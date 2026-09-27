#include <Wire.h>
#include <SparkFun_I2C_Mux_Arduino_Library.h>
#include "SparkFun_VL53L1X.h"
#include <SparkFun_KX13X.h>
#include <CS1237.h>
#include <RunningAverage.h>
#include <math.h>
#include <float.h>
#include <FastLED.h>

// LEDS
#define NUM_LEDS 0 //80
#define LED_GROUPS 4
#define LED_DATA_PIN 6 // esp32 data pin 6, nano pin 3
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 60
#define VOLTS 5
#define MAX_AMPS 500

#define WINDOW_SIZE 25
#define OUTLIER_THRESHOLD 2.0  // 2 standard deviations

#define NUMBER_OF_LOAD_SENSORS 4
#define NUMBER_OF_DISTANCE_SENSORS 4


int elastic= 1;

CRGB leds[NUM_LEDS];

// Define the MUX address and create the MUX object
const byte MUX_ADDRESS = 0x70; // Default I2C address for the MUX
QWIICMUX myMux;

// Create objects for the sensors
SFEVL53L1X* distanceSensor[NUMBER_OF_DISTANCE_SENSORS]; // Array of pointers to VL53L1X sensors
SparkFun_KX134 kx13x; // Assuming you're using the KX134 model

outputData myData; // Struct for the accelerometer's data


// Create instances for each CS1237
CS1237 adc1(4, 5);  // SCLK on pin 4, DOUT on pin 5
CS1237 adc2(6, 7);  
CS1237 adc3(8, 9);  
CS1237 adc4(10, 11);  



RunningAverage voltageAverages[NUMBER_OF_LOAD_SENSORS] = {RunningAverage(WINDOW_SIZE), RunningAverage(WINDOW_SIZE), RunningAverage(WINDOW_SIZE), RunningAverage(WINDOW_SIZE)};
RunningAverage distanceAverages[NUMBER_OF_DISTANCE_SENSORS] = {RunningAverage(WINDOW_SIZE), RunningAverage(WINDOW_SIZE), RunningAverage(WINDOW_SIZE), RunningAverage(WINDOW_SIZE)};


float minVoltage[NUMBER_OF_LOAD_SENSORS] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
float maxVoltage[NUMBER_OF_LOAD_SENSORS] = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};

float percentages[NUMBER_OF_LOAD_SENSORS] = {0.0,0.0,0.0,0.0};

float minDistance[NUMBER_OF_DISTANCE_SENSORS] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
float maxDistance[NUMBER_OF_DISTANCE_SENSORS] = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};

CRGB sectionColors[LED_GROUPS] = {CRGB::Blue, CRGB::Red, CRGB::Yellow, CRGB::Green};


float pga_divider = 1;

// Function to calculate standard deviation
float calculateStandardDeviation(RunningAverage& avg) {
    float mean = avg.getAverage();
    float sumSquares = 0.0;
    for (int i = 0; i < avg.getCount(); i++) {
        float value = avg.getElement(i);
        sumSquares += pow(value - mean, 2);
    }
    return sqrt(sumSquares / avg.getCount());
}

// Function to update min and max values
void updateMinMax(float value, float& minValue, float& maxValue, float mean, float stdDev) {
    if (abs(value - mean) <= OUTLIER_THRESHOLD * stdDev) {
        if (value < minValue) minValue = value;
        if (value > maxValue) maxValue = value;
    }
}

void setup() {

  if(NUM_LEDS>0)
  {
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);  // GRB ordering is typical
    FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS,MAX_AMPS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    for(int i =0; i <NUM_LEDS;i++)
    {
        leds[i] = CRGB::White;
    }
    
    FastLED.show();
  }

  Serial.begin(115200);

  delay(100);

  if(elastic==1)
  {
    Wire.begin();
    delay(100);

    // Initialize the MUX
    if (!myMux.begin()) {
      Serial.println("Mux not detected. Check wiring.");
      while (1);
    }
    Serial.println("Mux detected!");

    // Initialize the VL53L1X sensors on channels 0 to 3
    bool initSuccess = true;
    for (int i = 0; i < NUMBER_OF_DISTANCE_SENSORS; i++) {
      myMux.setPort(i);
      distanceSensor[i] = new SFEVL53L1X(Wire);
      if (distanceSensor[i]->begin() != 0) {
        Serial.print("VL53L1X not detected on channel ");
        Serial.println(i);
        initSuccess = false;
      } else {
        distanceSensor[i]->setIntermeasurementPeriod(180);
        distanceSensor[i]->setDistanceModeLong();
        distanceSensor[i]->startRanging();
        Serial.print("VL53L1X detected on channel ");
        Serial.println(i);
      }
      myMux.disablePort(i);
    }

    if (!initSuccess) {
      while (1);
    }

    // Initialize KX13X on channel 4
    myMux.setPort(4);
    if (!kx13x.begin()) {
      Serial.println("KX13X not detected on channel 4. Check wiring.");
      while (1);
    }
    Serial.println("KX13X detected on channel 4!");
    kx13x.enableAccel(false);

    kx13x.setRange(SFE_KX132_RANGE16G); // 16g Range
    // kxAccel.setRange(SFE_KX134_RANGE16G);         // 16g for the KX134

    kx13x.enableDataEngine(); // Enables the bit that indicates data is ready.
    // kxAccel.setOutputDataRate(); // Default is 50Hz
    kx13x.enableAccel();
    myMux.disablePort(4);
  }




  // while (!Serial) {} //Wait for serial
  
  delay(3000); //Delay a bit, so the terminal can catch up with the next message:
  adc1.begin();
  adc2.begin();
  adc3.begin();
  adc4.begin();

  adc1.setRegister(0, 0); //CH 0 input
  delay(100);
  adc1.setRegister(1, 0); //PGA = 1
  delay(100);
  adc1.setRegister(2, 1); //DRATE = 10 Hz
  delay(100);
  adc1.setRegister(3, 1); //VREF = DISABLED
  delay(100);

  adc2.setRegister(0, 0); //CH 0 input
  delay(100);
  adc2.setRegister(1, 0); //PGA = 1
  delay(100);
  adc2.setRegister(2, 1); //DRATE = 10 Hz
  delay(100);
  adc2.setRegister(3, 1); //VREF = DISABLED
  delay(100);

  adc3.setRegister(0, 0); //CH 0 input
  delay(100);
  adc3.setRegister(1, 0); //PGA = 1
  delay(100);
  adc3.setRegister(2, 1); //DRATE = 10 Hz
  delay(100);
  adc3.setRegister(3, 1); //VREF = DISABLED
  delay(100);

  adc4.setRegister(0, 0); //CH 0 input
  delay(100);
  adc4.setRegister(1, 0); //PGA = 1
  delay(100);
  adc4.setRegister(2, 1); //DRATE = 10 Hz
  delay(100);
  adc4.setRegister(3, 1); //VREF = DISABLED
  delay(100);

  
  
  
}

void loop() {

  
  long data1 = adc1.readData();
  long data2 = adc2.readData();
  long data3 = adc3.readData();
  long data4 = adc4.readData();

  float voltageValues[NUMBER_OF_LOAD_SENSORS] = {
    (1250.0 / 1.0) * ((float)data1 / (8388607.0)),
    (1250.0 / 1.0) * ((float)data2 / (8388607.0)),
    (1250.0 / 1.0) * ((float)data3 / (8388607.0)),
    (1250.0 / 1.0) * ((float)data4 / (8388607.0))
  };
  String buffer;
  // Serial.print(elastic);
  // Serial.print(",");
  buffer += String(elastic) +",";

  

  // for (int i = 0; i < NUMBER_OF_LOAD_SENSORS; i++) {
  //   voltageAverages[i].addValue(voltageValues[i]);

  //   float mean = voltageAverages[i].getAverage();
  //   float stdDev = calculateStandardDeviation(voltageAverages[i]);

  //   // Update min and max values excluding outliers
  //   updateMinMax(voltageValues[i], minVoltage[i], maxVoltage[i], mean, stdDev);

  //   // Calculate the percentage of the voltage values relative to the all-time min and max values
  //   float percentage = (voltageValues[i] - minVoltage[i]) / (maxVoltage[i] - minVoltage[i]) * 100.0;
  //   float perLED = (voltageValues[i] - minVoltage[i]) / (maxVoltage[i] - minVoltage[i]) * 255.0;
  //   percentages[i] = perLED;
    
  //   Serial.print(constrain(percentage,0,100), 2);
  //   Serial.print(",");
  // }
  

    for (int i = 0; i < NUMBER_OF_LOAD_SENSORS; i++) {
      // Add voltage value to the running average
      voltageAverages[i].addValue(voltageValues[i]);

      // Calculate mean and standard deviation
      float mean = voltageAverages[i].getAverage();
      float stdDev = calculateStandardDeviation(voltageAverages[i]);

      // Update min and max values excluding outliers
      updateMinMax(voltageValues[i], minVoltage[i], maxVoltage[i], mean, stdDev);

      // Pre-compute range for percentage and perLED calculations
      float range = maxVoltage[i] - minVoltage[i];
      float relativeValue = voltageValues[i] - minVoltage[i];

      // Calculate percentage and perLED values
      float percentage = (relativeValue / range) * 100.0;
      percentages[i] = (relativeValue / range) * 255.0;

      // Store the constrained percentage value in the buffer
      buffer += String(constrain(percentage, 0, 100), 2) + ",";
    }

    // Remove the trailing comma from the buffer
    buffer.remove(buffer.length() - 1);

    // Send the entire buffer in one Serial print call
    Serial.print(buffer);


    

  if(elastic==1)
  {
    
     // Read data from the four VL53L1X sensors
    for (int i = 0; i < NUMBER_OF_DISTANCE_SENSORS; i++) {
      myMux.setPort(i);
      uint16_t distance = distanceSensor[i]->getDistance();
      // Serial.print(distance);
      // Serial.print(",");
      distanceAverages[i].addValue(distance); // Add distance to running average
      float mean = distanceAverages[i].getAverage();
      float stdDev = calculateStandardDeviation(distanceAverages[i]);
      updateMinMax(distance, minDistance[i], maxDistance[i], mean, stdDev);
      float percentageDistance = (distance - minDistance[i]) / (maxDistance[i] - minDistance[i]) * 100.0;

      Serial.print(constrain(percentageDistance,0,100), 2);
      Serial.print(",");
      
    }

    // Read data from the KX13X on channel 4
    myMux.setPort(4);
    kx13x.getAccelData(&myData); // Refresh the accelerometer readings
    float accelX = myData.xData;
    float accelY = myData.yData;
    float accelZ = myData.zData;
    
    Serial.print(accelX, 2);
    Serial.print(",");
    Serial.print(accelY, 2);
    Serial.print(",");
    Serial.println(accelZ, 2);
    myMux.disablePort(4);
  }
  
  else
  {
    Serial.println();
  }
 
  
  loopLED();

  
  //delay(1000);
  
  
}


void loopLED()
{
  int ledsPerSection = NUM_LEDS / LED_GROUPS;

  // Loop through each section and set the color
  for(int section = 0; section < LED_GROUPS; section++) {
    for(int i = 0; i < ledsPerSection; i++) {
      int ledIndex = section * ledsPerSection + i;
      // float percentage = (voltageValues[i] - minVoltage[i]) / (maxVoltage[i] - minVoltage[i]) * 255.0;
      leds[ledIndex] = sectionColors[section];
      float brightness = 255 -percentages[section];
      if(brightness <40)
      {
        brightness =0.0;
      }
      if(brightness >200)
      {
        brightness = 255;
      }
      leds[ledIndex].nscale8(brightness);
      
    }
  }
  
  FastLED.show();
}