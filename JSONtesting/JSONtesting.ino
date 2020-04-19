#include <ArduinoJson.h>
const int JSON_BUFF_SIZE = 600;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

char * composeJsonPayload(double humidity, double temp){
  char * buf = (char *) malloc (JSON_BUFF_SIZE);
  
  StaticJsonDocument<JSON_BUFF_SIZE> doc;

  Serial.print("received humidity and temp: ");
  Serial.print(humidity);
  Serial.println(temp);
  
  doc["measurement"]="esp32_001";
  doc["fields"]["humidity[%]"]=humidity;
  doc["fields"]["temperature[C]"]=temp;

  char JSONmessageBuffer[JSON_BUFF_SIZE];
  serializeJsonPretty(doc, JSONmessageBuffer);
  
  strcpy (buf, JSONmessageBuffer);

  Serial.println(JSONmessageBuffer);
  return buf;
}

void loop() {
  char * p = composeJsonPayload(12.1111111111111, 23.111111111111666);
  Serial.println(p);  free(p);

  delay(5000);
}
