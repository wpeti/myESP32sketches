#include <SmoothThermistor.h>
SmoothThermistor smoothThermistor(12,              // the analog pin to read from
                              ADC_SIZE_12_BIT, // the ADC size
                              10000,           // the nominal resistance
                              10000,           // the series resistance
                              3435,            // the beta coefficient of the thermistor
                              25,              // the temperature for nominal resistance
                              10);             // the number of samples to take for each measurement

void setup(void) {
  Serial.begin(112500);

                                  
}

void loop(void) {

  // print the temperature
  Serial.print("Temperature = ");
  Serial.println(smoothThermistor.temperature());
}
