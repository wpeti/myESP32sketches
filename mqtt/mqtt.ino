#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

const int JSON_BUFF_SIZE = 600;

char *ssid = "UPC879DEA5";
char *password = "cjhz5jjBhmbp";
char *mqttServer = "192.168.0.115";
int mqttPort = 1883;
char *mqttTopic = "home/env/sensorreadings";
char *MQTT_MEASUREMENT_NAME = "esp32_001";

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient pubSubCli(espClient);

void setup()
{
  Serial.begin(115200);

  initializeSensor();
  initializeWifiAndMqttConnections();
}

void loop()
{
  measureAndSend();

  Serial.println("------= end of loop =-------");

  delay(10000);
}

void measureAndSend()
{
  float humidity = readDHTHumidity();

  float temperature = readDHTTemperature();

  char *mqttPayload = composeJsonPayload(humidity, temperature);

  sendPayloadToMqtt(mqttPayload);
}

void sendPayloadToMqtt(char *mqttPayload)
{
  Serial.println("Sending below message to MQTT broker..");
  Serial.println(mqttPayload);
  free(mqttPayload);

  if (pubSubCli.publish(mqttTopic, mqttPayload) == true)
  {
    Serial.println("Successfuly sent MQTT message!");
  }
  else
  {
    Serial.print("Error sending message with state: ");
    Serial.println(pubSubCli.state());
  }

  pubSubCli.loop();
}

char *composeJsonPayload(float humidity, float temp)
{
  Serial.println("Composing MQTT Json payload...");

  char *buf = (char *)malloc(JSON_BUFF_SIZE);

  StaticJsonDocument<JSON_BUFF_SIZE> doc;

  doc["measurement"] = MQTT_MEASUREMENT_NAME;
  doc["fields"]["humidity[%]"] = humidity;
  doc["fields"]["temperature[C]"] = temp;

  char JSONmessageBuffer[JSON_BUFF_SIZE];
  serializeJson(doc, JSONmessageBuffer);

  strcpy(buf, JSONmessageBuffer);

  return buf;
}

float readDHTTemperature()
{
  float t = dht.readTemperature();
  if (isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return -9999.9999;
  }
  else
  {
    return t;
  }
}

float readDHTHumidity()
{
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h))
  {
    Serial.println("Failed to read from DHT sensor!");
    return -9999.9999;
  }
  else
  {
    return h;
  }
}

void initializeSensor()
{
  Serial.println("Initializing sensor");
  dht.begin();
}

void initializeWifiAndMqttConnections()
{
  Serial.println("Initializing wifi and MQTT connections..");

  Serial.println("Starting wifi connection..");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {

    for (int i = 0; i < 5; i++)
    {
      if (WiFi.status() == WL_CONNECTED)
        break;
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }

    if (WiFi.status() == WL_CONNECTED)
      break;

    Serial.println("Restarting wifi connection..");
    WiFi.disconnect();
    delay(10000);
    WiFi.begin(ssid, password);
    delay(500);
  }

  Serial.println("Connected to the WiFi network!");

  pubSubCli.setServer(mqttServer, mqttPort);

  while (!pubSubCli.connected())
  {
    Serial.println("Connecting to MQTT...");

    if (pubSubCli.connect(MQTT_MEASUREMENT_NAME))
    {

      Serial.println("Connected to MQTT!");
    }
    else
    {

      Serial.print("Failed to connect to MQTT server with state: ");
      Serial.println(pubSubCli.state());
      Serial.println("Retrying shortly..");
      delay(2000);
    }
  }
}
