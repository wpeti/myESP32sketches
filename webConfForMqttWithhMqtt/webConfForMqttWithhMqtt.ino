/*
04.27.20:09 - 04.29.05:53 -> 33h 44m
04.29.13:58 - 05.01.03:59 -> 71h 57m ???
*/

#include <IotWebConf.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

const int JSON_BUFF_SIZE = 600;

DHT dht(DHTPIN, DHTTYPE);

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "esp32_001";

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

boolean isMyWifiConnected = false;

// -- Callback method declarations.
void configSaved();
boolean formValidator();
void wifiConnected();

DNSServer dnsServer;
WebServer server(80);

char mqttIPParamValue[STRING_LEN];
char mqttTopicParamValue[STRING_LEN];
char mqttPortParamValue[NUMBER_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- We can add a legend to the separator
IotWebConfSeparator mqttSeparator = IotWebConfSeparator("MQTT settings");
IotWebConfParameter mqttIPParam = IotWebConfParameter("MQTT server IP addr", "mqttIPParam", mqttIPParamValue, STRING_LEN, "string", "e.g. 192.168.0.115");
IotWebConfParameter mqttTopicParam = IotWebConfParameter("MQTT topic", "mqttTopicParam", mqttTopicParamValue, STRING_LEN, "string", "e.g. home/env/sensorreadings");
IotWebConfParameter mqttPortParam = IotWebConfParameter("MQTT port", "mqttPortParam", mqttPortParamValue, NUMBER_LEN, "number", "1..65535, e.g. 1883", NULL, "min='1' max='65535' step='1'");

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Starting up...");

    initializeWebConfigurations();

    Serial.println("Ready.");
}

void loop()
{
    if (isMyWifiConnected)
    {
        initializeSensor();
        measureAndSend();
    }
    
    // -- doLoop should be called as frequently as possible.
    iotWebConf.doLoop();
    delay(10000);
}

void configSaved()
{
    Serial.println("Configuration was updated.");
}

void initializeWebConfigurations()
{
    //iotWebConf.setStatusPin(STATUS_PIN);
    iotWebConf.addParameter(&mqttSeparator);
    iotWebConf.addParameter(&mqttIPParam);
    iotWebConf.addParameter(&mqttTopicParam);
    iotWebConf.addParameter(&mqttPortParam);

    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setFormValidator(&formValidator);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);
    //iotWebConf.getApTimeoutParameter()->visible = true;

    // -- Initializing the configuration.
    iotWebConf.init();

    // -- Set up required URL handlers on the web server.
    server.on("/", [] { iotWebConf.handleConfig(); });
    server.onNotFound([]() { iotWebConf.handleNotFound(); });
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

void wifiConnected()
{
  Serial.println("WiFi was connected.");
  isMyWifiConnected = true;
}

void measureAndSend()
{
    float humidity = readDHTHumidity();

    float temperature = readDHTTemperature();
    
    char *mqttPayload = composeJsonPayload(humidity, temperature);

    sendPayloadToMqtt(mqttPayload);
}

char *composeJsonPayload(float humidity, float temp)
{
    Serial.println("Composing MQTT Json payload...");

    char *buf = (char *)malloc(JSON_BUFF_SIZE);

    StaticJsonDocument<JSON_BUFF_SIZE> doc;

    doc["measurement"] = thingName;
    doc["fields"]["humidity[%]"] = humidity;
    doc["fields"]["temperature[C]"] = temp;

    char JSONmessageBuffer[JSON_BUFF_SIZE];
    serializeJson(doc, JSONmessageBuffer);

    strcpy(buf, JSONmessageBuffer);

    return buf;
}

void sendPayloadToMqtt(char *mqttPayload)
{
    Serial.println("Sending below message to MQTT broker..");
    Serial.println(mqttPayload);

    Serial.println("Initializing MQTT connection..");

    WiFiClient espClient;
    PubSubClient pubSubCli(espClient);

    pubSubCli.setServer(mqttIPParamValue, atoi(mqttPortParamValue));

    while (!pubSubCli.connected())
    {
        Serial.print("Connecting to MQTT using name: ");
        Serial.println(thingName);
        
        if (pubSubCli.connect(thingName))
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

    if (pubSubCli.publish(mqttTopicParamValue, mqttPayload) == true)
    {
        Serial.println("Successfuly sent MQTT message!");
    }
    else
    {
        Serial.print("Error sending message with state: ");
        Serial.println(pubSubCli.state());
    }

    free(mqttPayload);
    pubSubCli.loop();
}

float readDHTTemperature()
{
    float t = dht.readTemperature();
    if (isnan(t))
    {
        Serial.println("Failed to read temp from DHT sensor!");
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
        Serial.println("Failed to read humidity from DHT sensor!");
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
