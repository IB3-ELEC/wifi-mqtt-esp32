#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"

#define SSID          "IB3"
#define PWD           "odroidn2+"

#define MQTT_SERVER   "192.168.3.5"
#define MQTT_PORT     1883

#define LED_PIN       2

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

boolean kaart1 = false;
boolean kaart2 = false;
boolean kaart3 = false;
boolean kaart4 = false;
boolean kaart5 = false;

void callback(char *topic, byte *message, unsigned int length);

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

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  Serial.println(WiFi.localIP());
  pinMode(LED_PIN, OUTPUT);

}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  String topicString = String(topic);

  if (topicString == "kaart1"){
    kaart1 = true;
    Serial.println("kaart1 is ok");
    }
  else if (topicString == "kaart2"){
    kaart2 = true;
    Serial.println("kaart2 is ok");
    }
  else if (topicString == "kaart3"){
    kaart3 = true;
    Serial.println("kaart3 is ok");
    }
  else if (topicString == "kaart4"){
    kaart4 = true;
    Serial.println("kaart4 is ok");
    }
  else if (topicString == "kaart5"){
    kaart5 = true;
    Serial.println("kaart5 is ok");
    }

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "esp32/password")
  {
    Serial.print("Pass ");
    if (messageTemp == "on")
    {
      Serial.println("on");
      digitalWrite(LED_PIN, HIGH);
    }
    else if (messageTemp == "off")
    {
      Serial.println("off");
      digitalWrite(LED_PIN, LOW);
    }
  }
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
    if (client.connect("wachtwoord"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
      client.subscribe("#");
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
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (kaart1 and kaart2 and kaart3 and kaart4 and kaart5){
    client.publish("password", "dit is je super geheime wachtwoord sssht");
    kaart1 = false;
    kaart2 = false;
    kaart3 = false;
    kaart4 = false;
    kaart5 = false;
  }

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  }
}
