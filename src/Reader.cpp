#include <Adafruit_PN532.h>
#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#define SSID          "IB3"
#define PWD           "odroidn2+"

#define MQTT_SERVER   "192.168.3.5"
#define MQTT_PORT     1883

#define PN532_IRQ   4
#define PN532_RESET 5 

const int DELAY_BETWEEN_CARDS = 500;
long timeLastCardRead = 0;
boolean readerDisabled = false;
int irqCurr;
int irqPrev;

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char *topic, byte *message, unsigned int length);
static void startListeningToNFC();
static void handleCardDetected();

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setup_np532(){
    Serial.println("Begin NFC532 Scanning Software.");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN532 board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();

  startListeningToNFC();
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  setup_np532();
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "password")
  {
    Serial.print("Password: "+ messageTemp);
  }
}

void startListeningToNFC() {
  // Reset our IRQ indicators
  irqPrev = irqCurr = HIGH;
  
  Serial.println("Present an ISO14443A Card ...");
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // creat unique client ID
    // in Mosquitto broker enable anom. access
    if (client.connect("READER"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("password");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void np532(void) {
  if (readerDisabled) {
    if (millis() - timeLastCardRead > DELAY_BETWEEN_CARDS) {
      readerDisabled = false;
      startListeningToNFC();
    }
  } else {
    irqCurr = digitalRead(PN532_IRQ);

    // When the IRQ is pulled low - the reader has got something for us.
    if (irqCurr == LOW && irqPrev == HIGH) {
       //Serial.println("Got NFC IRQ");  
       handleCardDetected(); 
    }
  
    irqPrev = irqCurr;
  }
}

void handleCardDetected() {
    uint8_t success = false;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    uint8_t auth_uids1[][7] = {
              {0x04 ,0x07 ,0xCC ,0x52 ,0xA8 ,0x58 ,0x81},
              {0x04, 0x5B, 0xB4, 0x7A, 0x66, 0x62, 0x81},  // TAG
    };
    uint8_t auth_uids2[][4] = {
              {0x49, 0x1E, 0x07, 0xC2},
              {0xC9, 0xFC, 0x04, 0xC2},
              {0x49, 0x2A, 0x06, 0xC2},
    };


    // read the NFC tag's info
    success = nfc.readDetectedPassiveTargetID(uid, &uidLength);
    Serial.println(success ? "Read successful" : "Read failed (not a card?)");

    if (success) {
      // Display some basic information about the card
      //Serial.println("Found an ISO14443A card");
      //Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
      //Serial.print("  UID Value: ");
      Serial.print("Card ID HEX Value: ");
      nfc.PrintHex(uid, uidLength);
      if (uidLength == 7){
        for (int b=0; b < 7; b++){
            if (uid[b] == auth_uids1[0][b]){
              if (b == 6){
              client.publish("kaart1","ok");
              Serial.println("Message sent to kaart1");
              }
            }
            else{
            b=uidLength;
          }
        }
        for (int b=0; b<7;b++){
          if (uid[b] == auth_uids1[1][b]){
              if (b == 6){
              client.publish("kaart2","ok");
              Serial.println("Message sent to kaart2");
              }
          }
          else{
            b=uidLength;
          }
        }
      }
      else if(uidLength == 4){
        for (int b=0; b<4; b++){
          if (uid[b]== auth_uids2[0][b]){
            if (b==3){
              client.publish("kaart3","ok");
              Serial.println("Message sent to kaart3");
            }
          }
          else{
            b=uidLength;
          }
        }
        for (int b=0; b<4; b++){
          if (uid[b]== auth_uids2[1][b]){
            if (b==3){
              client.publish("kaart4","ok");
              Serial.println("Message sent to kaart4");
            }
          }
                    else{
            b=uidLength;
          }
      }
      for (int b=0; b<4; b++){
          if (uid[b]== auth_uids2[2][b]){
            if (b==3){
              client.publish("kaart5","ok");
              Serial.println("Message sent to kaart5");
            }
          }
                    else{
            b=uidLength;
          }
      }
      } 
      Serial.println("");
      timeLastCardRead = millis();
    }

    // The reader will be enabled again after DELAY_BETWEEN_CARDS ms will pass.
    readerDisabled = true;
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  np532();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  }
}




// valid_tags = 
// 0x49 0x1E 0x07 0xC2
// 0x04 0x07 0xCC 0x52 0xA8 0x58 0x81 
// 0xC9 0xFC 0x04 0xC2
// 0x04 0x5B 0xB4 0x7A 0x66 0x62 0x81
// 0x49 0x2A 0x06 0xC2
//  
// 0x04 0x6B 0x0F 0xE2 0x50 0x5A 0x80
// 0xA9 0xAF 0xAE 0xC2 
// 0x04 0x07 0xCC 0x52 0xA8 0x58 0x81 


