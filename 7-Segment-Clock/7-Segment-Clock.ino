#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
//#include <FastLED.h>
#include <FS.h>                               // Please read the instructions on http://arduino.esp8266.com/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system
#include "./Neo7Segment.h"

#define PIXELS_DIGITS       6   // Number of digits
#define PIXELS_PER_SEGMENT  6   // Pixels per segment - If you want more than 10 pixels per segment, modify the Neo7Segment_Var.cpp
#define PIXELS_PER_POINT    1   // Pixels per decimal point - CANNOT be higher than PIXELS_PER_SEGMENT
#define PIXELS_PIN          D6   // Pin number

// Settings
String now = "0";
int numbVal = 100000;
unsigned long prevTime = 0;
byte r_val = 255;
byte g_val = 0;
byte b_val = 0;

// Initalise the display with 5 Neo7Segment boards, 4 LEDs per segment, 1 decimal point LED, connected to GPIO 4
Neo7Segment disp(PIXELS_DIGITS, PIXELS_PER_SEGMENT, PIXELS_PER_POINT, PIXELS_PIN);

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

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup() {
  Serial.begin(115200); 
  delay(200);

  timeClient.begin();

  WiFi.setSleepMode(WIFI_NONE_SLEEP);  

  delay(200);
  //Serial.setDebugOutput(true);

  // Start the display with a brightness value of 20
  disp.Begin(20);

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
    Serial.print("Brightness set to: ");
    Serial.println(server.arg("brightness").toInt());    
    disp.SetBrightness(server.arg("brightness").toInt());    
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

    disp.Color(r_val, g_val, b_val);
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

//
void updateNumber() {  

  Serial.print("Number:");
  Serial.println(numbVal);
  disp.Color(r_val, g_val, b_val);

  String n1 = String((numbVal / 100000) % 10);
  String n2 = String((numbVal / 10000) % 10);
  String n3 = String((numbVal / 1000) % 10);  
  String n4 = String((numbVal / 100) % 10);
  String n5 = String((numbVal / 10) % 10);
  String n6 = String(numbVal % 10);

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

  disp.SetDigit(0, n6, disp.Color(r_val, g_val, b_val));
  disp.SetDigit(1, n5, disp.Color(r_val, g_val, b_val));
  disp.SetDigit(2, n4, disp.Color(r_val, g_val, b_val));
  disp.SetDigit(3, n3, disp.Color(r_val, g_val, b_val));
  disp.SetDigit(4, n2, disp.Color(r_val, g_val, b_val));
  disp.SetDigit(5, n1, disp.Color(r_val, g_val, b_val));

}
