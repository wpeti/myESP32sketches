//TODO make MQTT publishing secure
//every 55 sec  -> 2020-05-01T18:43:18 -> 2020-05-04T09:37:38 -> 2x24+14h 54m = 62h 54m
//every 595 sec ->

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
#define STATUS_PIN 12 //G12
#define CONFIG_PIN 14 //G14

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
char secondsToSleepParamValue[NUMBER_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- We can add a legend to the separator
IotWebConfSeparator mqttSeparator = IotWebConfSeparator("MQTT settings");
IotWebConfParameter mqttIPParam = IotWebConfParameter("MQTT server IP addr", "mqttIPParam", mqttIPParamValue, STRING_LEN, "string", "e.g. 192.168.0.115");
IotWebConfParameter mqttTopicParam = IotWebConfParameter("MQTT topic", "mqttTopicParam", mqttTopicParamValue, STRING_LEN, "string", "e.g. home/env/sensorreadings");
IotWebConfParameter mqttPortParam = IotWebConfParameter("MQTT port", "mqttPortParam", mqttPortParamValue, NUMBER_LEN, "number", "1..65535, e.g. 1883", NULL, "min='1' max='65535' step='1'");
IotWebConfParameter secondsToSleepParam = IotWebConfParameter("Seconds to sleep", "secondsToSleepParam", secondsToSleepParamValue, NUMBER_LEN, "number", "Seconds to sleep", NULL, "min='1' max='32767' step='1'");

void setup()
{
    Serial.begin(115200);
    delay(1000); //Take some time to open up the Serial Monitor
    Serial.println();
    Serial.println("Starting up...");

    print_wakeup_reason();
    
    Serial.println("Initializing sensor..");
    dht.begin();
    Serial.println("Doing sensor readings..");
    float humidity = readDHTHumidity();
    float temperature = readDHTTemperature();

    Serial.println("Initializing webConfig and Wifi connection..");
    initializeWebConfigurations();

    //Waiting for thing to start connecting to wifi
    //This is mainly necessary to avoid thing going to sleep while configuration changes are being applied in AP mode.
    //see IotWebConf states here: https://github.com/prampec/IotWebConf/blob/master/src/IotWebConf.h
    while(iotWebConf.getState() < 2)
    {
      Serial.print("Doing iotWebConf loops, while waiting to start conneting to wifi network.. IotWebConf status: ");
      Serial.println(iotWebConf.getState());
      iotWebConf.doLoop();
      delay(500);
    }

    int connectionTriesCount = 0;
    while(!isMyWifiConnected && connectionTriesCount < 30)
    {
      Serial.print("Doing iotWebConf loops, while waiting to be connected to wifi network.. IotWebConf status: ");
      Serial.println(iotWebConf.getState());
      iotWebConf.doLoop();
      delay(1000);
      connectionTriesCount++;
    }

    //we only try sending the measurements if wifi connection is available..
    if (isMyWifiConnected)
    {
        Serial.println("Sending measurements..");
        sendMeasurements(humidity, temperature);
    }
    else
    {
      Serial.println("Not sending measurements because wifi is not connected!");
    }

    esp_sleep_enable_timer_wakeup(atoi(secondsToSleepParamValue) * 1000000ULL);
    Serial.println("Setup ESP32 to sleep for every " + String(secondsToSleepParamValue) + " [second]");
  
    Serial.println("Going to sleep now..");
    Serial.flush(); 
    esp_deep_sleep_start();
}

void loop()
{
  iotWebConf.doLoop();
  Serial.println("Doing loop, but should be sleeping... I'M A ZOMBIE, KILL ME!");
  delay(500);
}

void configSaved()
{
    Serial.println("Configuration was updated.");
}

void initializeWebConfigurations()
{
    iotWebConf.setStatusPin(STATUS_PIN);
    iotWebConf.setConfigPin(CONFIG_PIN);
    iotWebConf.addParameter(&mqttSeparator);
    iotWebConf.addParameter(&mqttIPParam);
    iotWebConf.addParameter(&mqttTopicParam);
    iotWebConf.addParameter(&mqttPortParam);
    iotWebConf.addParameter(&secondsToSleepParam);


    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setFormValidator(&formValidator);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);
    
    // -- Initializing the configuration.
    iotWebConf.init();

    // on iotWebConf.init and on config saved, the ApTimeout is being overwritten, so need to set it after init.
    // this is necessary to not to waste time with waiting for AP connection. For that, there is the reset button (CONFIG_PIN).
    iotWebConf.setApTimeoutMs(100);

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

    if (server.arg(secondsToSleepParam.getId()).length() < 1)
    {
        secondsToSleepParam.errorMessage = "At least 1 characters as Seconds to sleep!";
        valid &= false;
    }

    if (server.arg(secondsToSleepParam.getId()).toInt() < 1 && server.arg(secondsToSleepParam.getId()).toInt() > 32767)
    {
        secondsToSleepParam.errorMessage = "Seconds to sleep value must be between 1 and 32767 !";
        valid &= false;
    }
    
    return valid;
}

void wifiConnected()
{
  Serial.println("WiFi was connected.");
  isMyWifiConnected = true;
}

void sendMeasurements(float humidity, float temperature)
{
    char *mqttPayload = composeJsonPayload(humidity, temperature);

    sendPayloadToMqtt(mqttPayload);
}

char *composeJsonPayload(float humidity, float temp)
{
    Serial.println("Composing MQTT Json payload...");

    char *buf = (char *)malloc(JSON_BUFF_SIZE);

    StaticJsonDocument<JSON_BUFF_SIZE> doc;

    doc["measurement"] = iotWebConf.getThingName();
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
        Serial.println(iotWebConf.getThingName());
        
        if (pubSubCli.connect(iotWebConf.getThingName()))
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

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
