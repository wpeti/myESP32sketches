#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_MCP3008.h>

const int JSON_BUFF_SIZE = 600;

char *ssid = "UPC879DEA5";
char *password = "cjhz5jjBhmbp";
char *mqttServer = "192.168.0.115";
int mqttPort = 1883;
char *mqttTopic = "home/env/sensorreadings";
char *MQTT_MEASUREMENT_NAME = "ESP32_001";

Adafruit_MCP3008 adc;

// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3435
// the value of the 'other' resistor
#define SERIESRESISTOR 10000

int samples[NUMSAMPLES];

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
  uint8_t i;
  float average;

  // take N samples in a row, with a slight delay
  for (i = 0; i < NUMSAMPLES; i++) {
    samples[i] = adc.readADC(0);
    delay(100);
  }

  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  Serial.print("Average analog reading: ");
  Serial.println(average);

  // convert the value to resistance
  average = 511 / average - 1;
  average = SERIESRESISTOR / average;
  Serial.print("Thermistor resistance: ");
  Serial.println(average);

  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  Serial.print("Temperature ");
  Serial.print(steinhart);
  Serial.println(" *C");
  
  if (isnan(steinhart))
  {
    Serial.println("Failed to read temperature from sensor!");
    return -9999.9999;
  }
  else
  {
    return steinhart;
  }
}

float readDHTHumidity()
{
    return -9999.9999;
}

void initializeSensor()
{
  Serial.println("Initializing sensor");
  //(clock, din, dout, cs);
  adc.begin(32, 27, 25, 12);
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
