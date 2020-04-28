#include <IotWebConf.h>
#include <MQTT.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "ESP32_001";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "Password123";

#define STRING_LEN 128
#define NUMBER_LEN 32

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "ESP32_WebConfScheme_v1"

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
//#define STATUS_PIN 1

// -- Callback method declarations.
void configSaved();
boolean formValidator();
void wifiConnected();
HTTPUpdateServer httpUpdater; //TODO test if this is neccessary for MQTT
WiFiClient wifiClientForMqtt;
MQTTClient mqttClient;

DNSServer dnsServer;
WebServer server(80);

char mqttIPParamValue [STRING_LEN];
char mqttTopicParamValue [STRING_LEN];
char mqttPortParamValue [NUMBER_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- We can add a legend to the separator
IotWebConfSeparator mqttSeparator = IotWebConfSeparator("MQTT settings");
IotWebConfParameter mqttIPParam = IotWebConfParameter("MQTT server IP addr", "mqttIPParam", mqttIPParamValue, STRING_LEN, "string", "e.g. 192.168.0.115");
IotWebConfParameter mqttTopicParam = IotWebConfParameter("MQTT topic", "mqttTopicParam", mqttTopicParamValue, STRING_LEN, "string", "e.g. home/env/sensorreadings");
IotWebConfParameter mqttPortParam = IotWebConfParameter("MQTT port", "mqttPortParam", mqttPortParamValue, NUMBER_LEN, "number", "1..65535, e.g. 1883", NULL, "min='1' max='65535' step='1'");

boolean needMqttConnect = false;
boolean needReset = false;

void setup() 
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...");

  //iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.addParameter(&mqttSeparator);
  iotWebConf.addParameter(&mqttIPParam);
  iotWebConf.addParameter(&mqttTopicParam);
  iotWebConf.addParameter(&mqttPortParam);
  
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.setupUpdateServer(&httpUpdater);

  // -- Initializing the configuration.
  boolean validConfig = iotWebConf.init();
  if (!validConfig)
  {
    mqttIPParamValue[0] = '\0';
    mqttPortParamValue[0] = '\0';
    mqttTopicParamValue[0] = '\0';
  }

  // -- Set up required URL handlers on the web server.
  server.on("/", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  mqttClient.begin(mqttIPParamValue, wifiClientForMqtt);
  mqttClient.onMessage(mqttMessageReceived);

  Serial.println("Ready.");
}

void loop() 
{
  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();
  mqttClient.loop();
}

void configSaved()
{
  Serial.println("Configuration was updated.");
}

boolean formValidator()
{
  Serial.println("Validating form.");
  boolean valid = true;
  
  if (server.arg(mqttIPParam.getId()).length() < 7)
  {
    mqttIPParam.errorMessage = "Provide a proper MQTT IP!";
    valid &= false;
  }

  if (server.arg(mqttTopicParam.getId()).length() < 3)
  {
    mqttTopicParam.errorMessage = "At least 3 characters as MQTT Topic!";
    valid &= false;
  }

  return valid;
}
