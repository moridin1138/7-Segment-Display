#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FastLED.h>
#include <FS.h>                               // Please read the instructions on http://arduino.esp8266.com/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system
#define countof(a) (sizeof(a) / sizeof(a[0]))
#define NUM_LEDS 252                           // Total of 86 LED's     
#define DATA_PIN D6                           // Change this if you are using another type of ESP board than a WeMos D1 Mini
#define MILLI_AMPS 2400 
#define COUNTDOWN_OUTPUT D5

#define WIFIMODE 2                            // 0 = Only Soft Access Point, 1 = Only connect to local WiFi network with UN/PW, 2 = Both

#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2)
  const char* APssid = "7SEG_AP";        
  const char* APpassword = "1234567890";  
#endif
  
#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2)
  #include "credentials.h"                    // Create this file in the same directory as the .ino file and add your credentials (#define SID YOURSSID and on the second line #define PW YOURPASSWORD)
  const char *ssid = SID;
  const char *password = PW;
#endif

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdateServer;
CRGB LEDs[NUM_LEDS];

// Settings
String now = "0";
int numbVal = 100000;
unsigned long prevTime = 0;
byte r_val = 255;
byte g_val = 0;
byte b_val = 0;
bool dotsOn = true;
byte brightness = 255;
CRGB alternateColor = CRGB::Black; 
long numbers[] = {
  0b000111111111111111111,  // [0] 0
  0b000111000000000000111,  // [1] 1
  0b111111111000111111000,  // [2] 2
  0b111111111000000111111,  // [3] 3
  0b111111000111000000111,  // [4] 4
  0b111000111111000111111,  // [5] 5
  0b111000111111111111111,  // [6] 6
  0b000111111000000000111,  // [7] 7
  0b111111111111111111111,  // [8] 8
  0b111111111111000111111,  // [9] 9
  0b000000000000000000000,  // [10] off
  0b111111111111000000000,  // [11] degrees symbol
  0b000000111111111111000,  // [12] C(elsius)
  0b111000111111111000000,  // [13] F(ahrenheit)
};

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup() {
  pinMode(COUNTDOWN_OUTPUT, OUTPUT);
  Serial.begin(115200); 
  delay(200);

  timeClient.begin();

  WiFi.setSleepMode(WIFI_NONE_SLEEP);  

  delay(200);
  //Serial.setDebugOutput(true);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);  
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(LEDs, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // WiFi - AP Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2) 
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(APssid, APpassword);    // IP is usually 192.168.4.1
  Serial.println();
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
#endif

  // WiFi - Local network Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2) 
  byte count = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    // Stop if cannot connect
    if (count >= 60) {
      Serial.println("Could not connect to local WiFi.");      
      return;
    }
       
    delay(500);
    Serial.print(".");
    LEDs[count] = CRGB::Green;
    FastLED.show();
    count++;
  }
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  IPAddress ip = WiFi.localIP();
  Serial.println(ip[3]);
#endif   

  httpUpdateServer.setup(&server);

  // Handlers
  server.on("/color", HTTP_POST, []() {
    Serial.println(server.arg("r").toInt());    
    Serial.println(server.arg("g").toInt());    
    Serial.println(server.arg("b").toInt());    
    r_val = server.arg("r").toInt();
    g_val = server.arg("g").toInt();
    b_val = server.arg("b").toInt();
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/brightness", HTTP_POST, []() {    
    brightness = server.arg("brightness").toInt();    
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/number", HTTP_POST, []() {    
    numbVal = server.arg("numberVal").toInt(); 
    writeValToFile(numbVal);
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  // Before uploading the files with the "ESP8266 Sketch Data Upload" tool, zip the files with the command "gzip -r ./data/" (on Windows I do this with a Git Bash)
  // *.gz files are automatically unpacked and served from your ESP (so you don't need to create a handler for each file).
  server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  server.begin();     

  SPIFFS.begin();
  Serial.println("SPIFFS contents:");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
  }
  Serial.println(); 

  File f = SPIFFS.open("numval.txt", "r");
  if (!f) {
    Serial.println("Count file open failed on read.");
  } else {
    while(f.available()) {
      //Lets read line by line from the file
      String line = f.readStringUntil('\n');
      numbVal = line.toInt();
      Serial.print("Read ");
      Serial.print(numbVal);
      Serial.println(" fromfile");
      break; //if left in, we'll just read the first line then break out of the while.
    } 
    f.close();
  }
  
  
}

void loop(){

  server.handleClient(); 
  
  unsigned long currentMillis = millis();  
  if (currentMillis - prevTime >= 1000) {
    prevTime = currentMillis;

    updateNumber();
  
    CRGB color = CRGB(r_val, g_val, b_val);
    FastLED.setBrightness(brightness);
    FastLED.show();
  }   
}

void writeValToFile(int inputval){

  File f = SPIFFS.open("numval.txt", "w");
  if (!f) {
    Serial.println("Count file open failed on update.");
  } else {
    f.println(inputval); 
    f.close();
    Serial.print("Added: ");
    Serial.println(inputval);
  }
  f.close();
}

void displayNumber(byte number, byte segment, CRGB color) {
  /*
   * 
      __ __ __        __ __ __          __ __ __        12 13 14  
    __        __    __        __      __        __    11        15
    __        __    __        __      __        __    10        16
    __        __    __        __  42  __        __    _9        17
      __ __ __        __ __ __          __ __ __        20 19 18  
    __        65    __        44  43  __        21    _8        _0
    __        __    __        __      __        __    _7        _1
    __        __    __        __      __        __    _6        _2
      __ __ __       __ __ __           __ __ __       _5 _4 _3   

   */

 
  // segment from left to right: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = 42;
      break;
    case 2:
      startindex = 84;
      break;
    case 3:
      startindex = 126;
      break;   
    case 4:
      startindex = 168;
      break;     
    case 5:
      startindex = 210;
      break;  
  }

  for (byte i=0; i<42; i++){
    yield();
    LEDs[i + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? color : alternateColor;
  } 
}

void allBlank() {
  for (int i=0; i<NUM_LEDS; i++) {
    LEDs[i] = CRGB::Black;
  }
  FastLED.show();
}

//
void updateNumber() {  

  Serial.print("Number:");
  Serial.println(numbVal);
  CRGB color = CRGB(r_val, g_val, b_val);

  byte n1 = (numbVal / 100000) % 10;
  byte n2 = (numbVal / 10000) % 10;
  byte n3 = (numbVal / 1000) % 10;  
  byte n4 = (numbVal / 100) % 10;
  byte n5 = (numbVal / 10) % 10;
  byte n6 = numbVal % 10;

  Serial.print("Broken Number:");
  Serial.print(n1);
  Serial.print(" - ");
  Serial.print(n2);
  Serial.print(" - ");
  Serial.print(n3);
  Serial.print(" - ");
  Serial.print(n4);
  Serial.print(" - ");
  Serial.print(n5);
  Serial.print(" - ");
  Serial.println(n6);

  displayNumber(n1,5,color);
  displayNumber(n2,4,color);
  displayNumber(n3,3,color); 
  displayNumber(n4,2,color);
  displayNumber(n5,1,color);
  displayNumber(n6,0,color); 
  
}
