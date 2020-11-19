// Thermistor Example #3 from the Adafruit Learning System guide on Thermistors
// https://learn.adafruit.com/thermistor/overview by Limor Fried, Adafruit Industries
// MIT License - please keep attribution and consider buying parts from Adafruit

#include <Adafruit_MCP3008.h>
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
int counter = 0;

void setup(void) {
  Serial.begin(9600);

  //(clock, din, dout, cs);
  adc.begin(32, 27, 25, 12);
}

void loop(void) {
  Serial.print(counter++);
  Serial.println(". ============================");
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
  
  delay(5000);
}
