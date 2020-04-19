/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

// Import required libraries
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 4     
#define DHTTYPE    DHT22     

DHT dht(DHTPIN, DHTTYPE);

String readDHTTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  dht.begin();
}
 
void loop(){
  Serial.print("Humidity reading: ");
  Serial.println(readDHTHumidity());
  Serial.print("Temperature reading: ");
  Serial.println(readDHTTemperature());
  delay(10000);
}
