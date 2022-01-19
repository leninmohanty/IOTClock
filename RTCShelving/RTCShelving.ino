/*
 * 3D printed smart shelving with a giant hidden digital clock in the front edges of the shelves - DIY Machines

==========

More info and build instructions: https://www.youtube.com/watch?v=8E0SeycTzHw

3D printed parts can be downloaded from here: https://www.thingiverse.com/thing:4207524

You will need to install the Adafruit Neopixel library which can be found in the library manager.

This project also uses the handy DS3231 Simple library:- https://github.com/sleemanj/DS3231_Simple   Please follow the instruction on installing this provided on the libraries page

Before you install this code you need to set the time on your DS3231. Once you have connected it as shown in this project and have installed the DS3231_Simple library (see above) you
 to go to  'File' >> 'Examples' >> 'DS3231_Simple' >> 'Z1_TimeAndDate' >> 'SetDateTime' and follow the instructions in the example to set the date and time on your RTC

==========


 * SAY THANKS:

Buy me a coffee to say thanks: https://ko-fi.com/diymachines
Support us on Patreon: https://www.patreon.com/diymachines

SUBSCRIBE:
■ https://www.youtube.com/channel/UC3jc4X-kEq-dEDYhQ8QoYnQ?sub_confirmation=1

INSTAGRAM: https://www.instagram.com/diy_machines/?hl=en
FACEBOOK: https://www.facebook.com/diymachines/

*/




#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif





// Which pin on the Arduino is connected to the NeoPixels?
#define LEDCLOCK_PIN    D6
#define LEDDOWNLIGHT_PIN    D5

// How many NeoPixels are attached to the Arduino?
#define LEDCLOCK_COUNT 207
#define LEDDOWNLIGHT_COUNT 12
#define SENSORNAME "RTCShelving"
  //(red * 65536) + (green * 256) + blue ->for 32-bit merged colour value so 16777215 equals white
int clockMinuteColour = 51200; //1677
int clockHourColour = 140000000; //7712


int OTAport = 8266;

int clockFaceBrightness = 0;

// Declare our NeoPixel objects:
Adafruit_NeoPixel stripClock(LEDCLOCK_COUNT, LEDCLOCK_PIN, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel stripDownlighter(LEDDOWNLIGHT_COUNT, LEDDOWNLIGHT_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

const char* ssid = "*******";
const char* password = "********";

int mmin = 0;
int hour = 0;
int colr = 51200;
int temp = 0;
//Smoothing of the readings from the light sensor so it is not too twitchy
const int numReadings = 12;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
long total = 0;                  // the running total
long average = 0;                // the average



void setup() {

  Serial.begin(115200);
  //Clock.begin();
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting..");
  }
  setupOta();
  stripClock.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
  stripClock.show();            // Turn OFF all pixels ASAP
  stripClock.setBrightness(255); // Set inital BRIGHTNESS (max = 255)
 

  stripDownlighter.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
  stripDownlighter.show();            // Turn OFF all pixels ASAP
  stripDownlighter.setBrightness(50); // Set BRIGHTNESS (max = 255)

  //smoothing
    // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  
}

void loop() {
  
  //read the time
  callTimeApi();
  //change the color
  changeColor();
  
  if(temp != mmin){
    stripClock.clear();  
    stripClock.show();
    temp = mmin;  
    Serial.print("strip refreshed");
  }
  //display the time on the LEDs
  displayTheTime();
//stripClock.show();



  //Record a reading from the light sensor and add it to the array
  readings[readIndex] = analogRead(A0); //get an average light level from previouse set of samples
  Serial.print("Light sensor value added to array = ");
  Serial.println(readings[readIndex]);
  readIndex = readIndex + 1; // advance to the next position in the array:

  // if we're at the end of the array move the index back around...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  //now work out the sum of all the values in the array
  int sumBrightness = 0;
  for (int i=0; i < numReadings; i++)
    {
        sumBrightness += readings[i];
    }
  Serial.print("Sum of the brightness array = ");
  Serial.println(sumBrightness);

  // and calculate the average: 
  int lightSensorValue = sumBrightness / numReadings;
  Serial.print("Average light sensor value = ");
  Serial.println(lightSensorValue);


  //set the brightness based on ambiant light levels
  clockFaceBrightness = map(lightSensorValue,50, 1000, 200, 1); 
  stripClock.setBrightness(clockFaceBrightness); // Set brightness value of the LEDs
  Serial.print("Mapped brightness value = ");
  Serial.println(clockFaceBrightness);
  
  stripClock.show();

   //(red * 65536) + (green * 256) + blue ->for 32-bit merged colour value so 16777215 equals white
  stripDownlighter.fill(16777215, 0, LEDDOWNLIGHT_COUNT);
  stripDownlighter.show();

  delay(5000);   //this 5 second delay to slow things down during testing

}


void setupOta(){
   //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(SENSORNAME);

  // No authentication by default
  //ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}
void readTheTime(){
  // Ask the clock for the data.
 // MyDateAndTime = Clock.read();
  
  // And use it
  Serial.println("");
  Serial.print("Time is: ");   Serial.print(18);
  Serial.print(":"); Serial.print(25);
  //Serial.print(":"); Serial.println(MyDateAndTime.Second);
 // Serial.print("Date is: 20");   Serial.print(MyDateAndTime.Year);
  //Serial.print(":");  Serial.print(MyDateAndTime.Month);
  //Serial.print(":");    Serial.println(MyDateAndTime.Day);
}

void displayTheTime(){

  stripClock.clear(); //clear the clock face 

  
  int firstMinuteDigit = mmin % 10; //work out the value of the first digit and then display it
  displayNumber(firstMinuteDigit, 0, clockMinuteColour);

  
  int secondMinuteDigit = floor(mmin / 10); //work out the value for the second digit and then display it
  displayNumber(secondMinuteDigit, 63, clockMinuteColour);  


  int firstHourDigit = hour; //work out the value for the third digit and then display it
  if (firstHourDigit > 12){
    firstHourDigit = firstHourDigit - 12;
  }
 
 // Comment out the following three lines if you want midnight to be shown as 12:00 instead of 0:00
//  if (firstHourDigit == 0){
//    firstHourDigit = 12;
//  }
 
  firstHourDigit = firstHourDigit % 10;
  displayNumber(firstHourDigit, 126, clockHourColour);


  int secondHourDigit = hour; //work out the value for the fourth digit and then display it

// Comment out the following three lines if you want midnight to be shwon as 12:00 instead of 0:00
//  if (secondHourDigit == 0){
//    secondHourDigit = 12;
//  }
 
 if (secondHourDigit > 12){
    secondHourDigit = secondHourDigit - 12;
  }
    if (secondHourDigit > 9){
      stripClock.fill(clockHourColour,189, 18); 
    }

  }


void displayNumber(int digitToDisplay, int offsetBy, int colourToUse){
    switch (digitToDisplay){
    case 0:
    digitZero(offsetBy,colourToUse);
      break;
    case 1:
      digitOne(offsetBy,colourToUse);
      break;
    case 2:
    digitTwo(offsetBy,colourToUse);
      break;
    case 3:
    digitThree(offsetBy,colourToUse);
      break;
    case 4:
    digitFour(offsetBy,colourToUse);
      break;
    case 5:
    digitFive(offsetBy,colourToUse);
      break;
    case 6:
    digitSix(offsetBy,colourToUse);
      break;
    case 7:
    digitSeven(offsetBy,colourToUse);
      break;
    case 8:
    digitEight(offsetBy,colourToUse);
      break;
    case 9:
    digitNine(offsetBy,colourToUse);
      break;
    default:
     break;
  }
}

void changeColor(){
  
  if(mmin != temp){
      clockHourColour = colr;
      clockMinuteColour = getComplementaryColor(clockHourColour);
      Serial.print("color refreshed");
  }
  Serial.print("clockHourColour is : ");  Serial.println(clockHourColour);
  Serial.print("clockMinuteColour is : ");  Serial.println(clockMinuteColour);
  
}

void callTimeApi(){
if (WiFi.status() == WL_CONNECTED) { 
    HTTPClient http; 
    http.begin("http://192.168.x.x/wp-json/time/v1/times"); 
    int httpCode = http.GET();                                                                  
    if (httpCode > 0) { 
      const size_t capacity = JSON_OBJECT_SIZE(2) + 10;
      DynamicJsonBuffer doc(capacity);
      JsonObject& root = doc.parseObject(http.getString());
      hour = root["h"]; // 21
      mmin = root["m"]; // 51
      colr = root["c"];
     Serial.print("Hour is : ");  Serial.println(hour);    
     Serial.print("Min is: ");  Serial.print(mmin);                 
    }
    http.end(); 
  }
}

 int getComplementaryColor( int color) {
          int R = color & 255;
          int G = (color >> 8) & 255;
          int B = (color >> 16) & 255;
          int A = (color >> 24) & 255;
          R = 255 - R;
          G = 255 - G;
          B = 255 - B;
          return R + (G << 8) + ( B << 16) + ( A << 24);
      }
