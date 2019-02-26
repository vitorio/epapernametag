#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSans12pt7b.h>

#include "waveshare-demo-epd42.h"        // combined e-Paper driver and 4.2" display code

// ***** for mapping of Waveshare e-Paper ESP8266 Driver Board *****
// select one , can use full buffer size (full HEIGHT)
//GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(/*CS=15*/ SS, /*DC=4*/ 4, /*RST=5*/ 5, /*BUSY=16*/ 16));
// can use only half buffer size
GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT / 2> display(GxEPD2_420c(/*CS=15*/ SS, /*DC=4*/ 4, /*RST=5*/ 5, /*BUSY=16*/ 16));

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
int prev_stations;      // Connected wifi clients

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)

void setup(void) {
  Serial.begin(115200);
  Serial.println();

  // e-paper display SPI initialization has to come before SPIFFS or it'll reset
  pinMode(CS_PIN  , OUTPUT);
  pinMode(RST_PIN , OUTPUT);
  pinMode(DC_PIN  , OUTPUT);
  pinMode(BUSY_PIN,  INPUT);
  SPI.begin();

  // then GxEPD
  display.init();

  SPIFFS.begin();                           // Start the SPI Flash Files System

  pinMode(10, INPUT_PULLUP); //SD3 / GPIO10 is safe to use because our flash is DIO
  if (digitalRead(10) == LOW)
  {
    SPIFFS.remove("/wifiap.txt");
  }

  Serial.print("Checking for wifi credentials file ... ");
  if (!SPIFFS.exists("/wifiap.txt")) {
    Serial.println("NOT FOUND!");
    long randSSID = secureRandom(1000, 10000); // four digit HW PRNG random number for a semi-unique SSID
    long randPW = secureRandom(10000000L, 100000000L); // eight digit HW PRNG random number password
    File f = SPIFFS.open("/wifiap.txt", "w");
    if (!f) {
      Serial.println("Failed to create wifi credential file!");
    }
    f.print("epapernametag-");
    f.println(randSSID);
    f.println(randPW);
    f.close();

    Serial.println("epaper credentials start");
    display.setRotation(0);
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(0, 0);
      display.println("");
      display.println("To upload a name tag, connect to:");
      display.println("");
      display.print("Wifi SSID: epapernametag-");
      display.println(randSSID);
      display.println("");
      display.print("Wifi password: ");
      display.println(randPW);
      display.println("");
      display.println("http://epapernametag.local (iOS/Mac)");
      display.println("http://192.168.4.1 (Android/Win)");
      display.println("");
      display.println("Jumper SD3 to GND to reset wifi");
    }
    while (display.nextPage());
    display.powerOff();
    Serial.println("epaper credentials end");
  } else {
    Serial.println("FOUND!");
  }
  
  File f = SPIFFS.open("/wifiap.txt", "r");
  if (!f) {
    Serial.println("Failed to open wifi credential file!");
  }
  String ssid = f.readStringUntil('\n');
  String password = f.readStringUntil('\n');
  f.close();
  
  ssid.trim();
  password.trim();
  char __ssid[ssid.length() + 1];
  char __password[password.length() + 1];
  ssid.toCharArray(__ssid, sizeof(__ssid));
  password.toCharArray(__password, sizeof(__password));
  
  Serial.print("SSID = ");
  Serial.println(__ssid);
  Serial.print("Password = ");
  Serial.println(__password);

  Serial.println(WiFi.softAP(__ssid, __password) ? "Wifi AP ready" : "Wifi AP failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Stations connected = ");
  Serial.println(prev_stations = WiFi.softAPgetStationNum());

  if (MDNS.begin("epapernametag")) {
    Serial.println("MDNS responder started");
  }

  server.on("/LOAD", EPD_Load);
  server.on("/EPD", EPD_Init);
  server.on("/NEXT", EPD_Next);
  server.on("/SHOW", EPD_Show);
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void loop(void) {
  int curr_stations = WiFi.softAPgetStationNum();
  if (curr_stations != prev_stations) {
    Serial.printf("Stations connected = %d\n", curr_stations);
    prev_stations = curr_stations;
  }

  server.handleClient();
}

void EPD_Init()
{
  EPD_dispIndex = ((int)server.arg(0)[0] - 'a') +  (((int)server.arg(0)[1] - 'a') << 4);
  // Print log message: initialization of e-Paper (e-Paper's type)
  Serial.printf("EPD %s\r\n", EPD_dispMass[EPD_dispIndex].title);

  // Initialization
  EPD_dispInit();
  server.send(200, "text/plain", "Init ok\r\n");
}

void EPD_Load()
{
  //server.arg(0) = data+data.length+'LOAD'
  String p = server.arg(0);
  if (p.endsWith("LOAD")) {
    int index = p.length() - 8;
    int L = ((int)p[index] - 'a') + (((int)p[index + 1] - 'a') << 4) + (((int)p[index + 2] - 'a') << 8) + (((int)p[index + 3] - 'a') << 12);
    if (L == (p.length() - 8)) {
      Serial.println("LOAD");
      // if there is loading function for current channel (black or red)
      // Load data into the e-Paper
      if (EPD_dispLoad != 0) EPD_dispLoad();
    }
  }
  server.send(200, "text/plain", "Load ok\r\n");
}

void EPD_Next()
{
  Serial.println("NEXT");

  // Instruction code for for writting data into
  // e-Paper's memory
  int code = EPD_dispMass[EPD_dispIndex].next;

  // If the instruction code isn't '-1', then...
  if (code != -1)
  {
    // Do the selection of the next data channel
    EPD_SendCommand(code);
    delay(2);
  }
  // Setup the function for loading choosen channel's data
  EPD_dispLoad = EPD_dispMass[EPD_dispIndex].chRd;

  server.send(200, "text/plain", "Next ok\r\n");
}

void EPD_Show()
{
  Serial.println("SHOW");
  // Show results and Sleep
  EPD_dispMass[EPD_dispIndex].show();
  server.send(200, "text/plain", "Show ok\r\n");
}

String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}
