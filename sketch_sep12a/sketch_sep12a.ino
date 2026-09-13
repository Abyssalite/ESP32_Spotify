
#include <FS.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <string>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <JPEGDEC.h>

#include "env.h"

#define UPDATE_OLED_PERIOD 2000
#define UPDATE_TFT_PERIOD 500
#define RECONNECT_PERIOD 1000
#define NOTIFY_PERIOD 1000
#define API_PERIOD 30000

unsigned long updateOledTimer = 0;
unsigned long updateTftTimer = 0;
unsigned long reconnectTimer = 0;
unsigned long apiTimer = 0;
unsigned long lastNotify = 0;
uint8_t count = 0;

char* ssid     = "";
char* password = "";
char* status   = "";
String message = "";

String apiKey     = "";
String userName = "";

String imageUrl   = "";
String songName   = "";
String artistName = ""; 

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JsonDocument telemetryJson;
Preferences preferences;

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;

void saveUser(String key, String user) {
  preferences.begin("last.fm", false); //read-write
  preferences.putString("key", key);
  preferences.putString("user", user);
  preferences.end();
}

String loadApiKey() {
  preferences.begin("last.fm", true); //read-only
  String key = preferences.getString("key", "");
  preferences.end();
  return key;
}

String loadUserName() {
  preferences.begin("last.fm", true); //read-only
  String user = preferences.getString("user", "");
  preferences.end();
  return user;
}

void notifyClients() {

  String jsonString;
  serializeJson(telemetryJson, jsonString);
  ws.textAll(jsonString);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) 
    status = "WS connected";
  
  else if (type == WS_EVT_DISCONNECT) 
    status = "WS disconnect";

  else if (type == WS_EVT_DATA) {
    // Handle commands from app
    message = String((char*)data).substring(0, len);

    }
    lastNotify = millis();
}

void drawOled () {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.setCursor(4, 8);
    u8g2.print("Connected to");

    u8g2.setCursor(4, 18);
    u8g2.print(WiFi.localIP());

    u8g2.setCursor(4, 28);
    u8g2.print(status);
  } while (u8g2.nextPage()); 
}

void drawTft () {
}

int drawTftJPEG(JPEGDRAW *pDraw) {
  Serial.printf("Draw block: x=%d y=%d  w=%d h=%d\n", 
                pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);

  tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  return 1;
}

bool downloadAndDrawImage(int x, int y) {
  if (imageUrl == "") {
    Serial.println("No Image url");
    return false;    
  }
  
  HTTPClient http;
  http.begin(imageUrl);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.println("Download Image failed.");
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  // Get the image data
  int len = http.getSize();
  uint8_t *buffer = (uint8_t *)malloc(len);
  if (!buffer) {
    Serial.println("Not enough memory");
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  stream->readBytes(buffer, len);
  http.end();

  // ===== Decode with JPEGDEC =====
  if (jpeg.openRAM(buffer, len, drawTftJPEG)) {
    Serial.printf("JPEG size: %d x %d\n", jpeg.getWidth(), jpeg.getHeight());

    jpeg.setPixelType(RGB565_BIG_ENDIAN);  // important for TFT_eSPI

    // Center the image
    //int x = (240 - jpeg.getWidth()) / 2;
    //if (x < 0) x = 0;
    jpeg.setMaxOutputSize(100);
    jpeg.decode(0, 0, 2);
    jpeg.close();

    Serial.println("Image drawn successfully");
  } else {
    Serial.println("JPEGDEC failed to open image");
  }

  free(buffer);
  return true;
}

void showNowPlaying() {
  tft.fillScreen(TFT_BLACK);

  // Draw album art (top of screen)
  if (!downloadAndDrawImage(0, 0))
    Serial.println("Failed to print image");

  // Song name
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString(songName.substring(0, 18), 120, 260);

  // Artist name
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(artistName.substring(0, 30), 120, 295);
}

bool getNowPlaying() {
  if (userName == "" || apiKey == "") {
    Serial.println("Missing login datas.");
    return false;
  }
  HTTPClient http;

  String url = "http://ws.audioscrobbler.com/2.0/?method=user.getrecenttracks";
  url += "&user=" + userName;
  url += "&api_key=" + apiKey;
  url += "&limit=1&format=json";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.println("Get Song Data failed.");
    Serial.printf("Last.fm error: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument songJson;
  deserializeJson(songJson, payload);

  JsonObject track = songJson["recenttracks"]["track"][0];
      Serial.println(track);

  // Check if something is currently playing
  if (track["@attr"]["nowplaying"] | false) {
    imageUrl   = "";
    songName   = "Nothing is currently playing";
    artistName = ""; 
    return true;
  }

  JsonArray images = track["image"];
  if (images.size() >= 4) {
    imageUrl = images[3]["#text"].as<String>();   // extralarge
  } else if (images.size() >= 3) {
    imageUrl = images[2]["#text"].as<String>();   // large
  }

  songName   = track["name"].as<String>();
  artistName = track["artist"]["#text"].as<String>();

  //if (imageUrl == "" || albumArtUrl.indexOf("2a96cbd8b46e442fc41c2b86b821562f") >= 0)
  //  No image or default placeholder
  //  imageUrl = "";

  return true;
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(SSID, PASSWORD);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  u8g2.begin();
  u8g2.enableUTF8Print();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  delay(1000);

  // #define in env.h
  if (LASTFM_API_KEY == "" || LASTFM_USERNAME == "") {
    apiKey = loadApiKey();
    userName = loadUserName();
  }
  else {
    apiKey = LASTFM_API_KEY;
    userName = LASTFM_USERNAME;
    saveUser(apiKey, userName);
  }
}

void loop() {
  unsigned long now = millis();

  if (now - updateOledTimer >= UPDATE_OLED_PERIOD) {
    updateOledTimer = now;
    drawOled();
  }

  if (now - updateTftTimer >= UPDATE_TFT_PERIOD) {
    updateTftTimer = now;
    count = (count + 1) % 3;
    drawTft();
  }

  if (now - lastNotify >= NOTIFY_PERIOD) {
    lastNotify = now;
    notifyClients();
  }

  if (now - apiTimer >= API_PERIOD && (WiFi.status() == WL_CONNECTED)) {
    apiTimer = now;
    if (getNowPlaying()) {
      Serial.println(imageUrl);
      Serial.println(songName);
      Serial.println(artistName);
      showNowPlaying();
    }
  }

  ws.cleanupClients();
}