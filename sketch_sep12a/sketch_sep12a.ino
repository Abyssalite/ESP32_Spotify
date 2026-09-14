
#include <FS.h>
#include <SPIFFS.h>
//#include <U8g2lib.h>
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
#define IMAGE_SIZE 120

//unsigned long updateOledTimer = 0;
unsigned long updateTftTimer = 0;
unsigned long reconnectTimer = 0;
unsigned long apiTimer = 0;
unsigned long lastNotify = 0;
uint8_t count = 0;
uint16_t *imageBuffer = nullptr;

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

//U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
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

/*void drawOled () {
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
}*/

int savePixel(JPEGDRAW *pDraw) {
  for (int y = 0; y < pDraw->iHeight; y++) {
    int destY = pDraw->y + y;

    if (destY < 0 || destY >= IMAGE_SIZE)
      continue;

    int srcX = 0;
    int destX = pDraw->x;
    int width = pDraw->iWidth;

    // Clip left
    if (destX < 0) {
        srcX = -destX;
        width -= srcX;
        destX = 0;
    }

    // Clip right
    if (destX + width > IMAGE_SIZE) {
      width = IMAGE_SIZE - destX;
    }

    if (width <= 0)
        continue;

    memcpy(
      imageBuffer + destY * IMAGE_SIZE + destX,
      pDraw->pPixels + y * pDraw->iWidth + srcX,
      width * sizeof(uint16_t)
    );
  }
  return 1;
}

void drawTftJPEG() {
    const int center = IMAGE_SIZE / 2;
    const int radius = IMAGE_SIZE / 2;
    const int radius2 = radius * radius;

    for (int y = 0; y < IMAGE_SIZE; y++) {
        for (int x = 0; x < IMAGE_SIZE; x++) {

            int dx = x - center;
            int dy = y - center;

            if (dx * dx + dy * dy > radius2)
                continue;

            uint16_t pixel = imageBuffer[y * IMAGE_SIZE + x];

            tft.drawPixel(x * 2,     y * 2,     pixel);
            tft.drawPixel(x * 2 + 1, y * 2,     pixel);
            tft.drawPixel(x * 2,     y * 2 + 1, pixel);
            tft.drawPixel(x * 2 + 1, y * 2 + 1, pixel);
        }
    }
}

void drawCircleMask(int cx, int cy, int radius) {
  for (int y = 0; y < 240; y++) {
    for (int x = 0; x < 240; x++) {
      int dx = x - cx;
      int dy = y - cy;
      if (dx*dx + dy*dy > radius*radius) {
        tft.drawPixel(x, y, TFT_BLACK);
      }
    }
  }
}

bool downloadAndDrawImage(int x, int y) {
  if (imageUrl == "") {
    Serial.println("No Image url");
    return false;    
  }
  
  HTTPClient http;
  http.begin(imageUrl);
  http.setTimeout(15000);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.println("Download Image failed.");
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  // Get the image data
  int totalLen = http.getSize();
  if (totalLen <= 0) return false;

  uint8_t *buffer = (uint8_t *)malloc(totalLen);
  if (!buffer) {
    Serial.println("Not enough memory");
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  int bytesRead = 0;
  unsigned long timeout = millis() + 10000;

  while (http.connected() && (bytesRead < totalLen) && millis() < timeout) {
    size_t avail = stream->available();
    if (avail) {
      int toRead = min((int)avail, totalLen - bytesRead);
      int got = stream->readBytes(buffer + bytesRead, toRead);
      if (got > 0) bytesRead += got;
    } else {
      delay(2);
    }
  }

  http.end();

  if (bytesRead != totalLen) {
    Serial.println("Incomplete download - abort");
    free(buffer);
    return false;
  }
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  // ===== Decode with JPEGDEC =====
  if (jpeg.openRAM(buffer, totalLen, savePixel)) {
    //Serial.printf("JPEG size: %d x %d\n", jpeg.getWidth(), jpeg.getHeight());
      jpeg.setPixelType(RGB565_LITTLE_ENDIAN);

    if(jpeg.getWidth() != IMAGE_SIZE || jpeg.getHeight() != IMAGE_SIZE) {
      free(buffer);
      jpeg.close();
      return false;
    }
    if (!jpeg.decode(0, 0, 0)) {
      free(buffer);
      jpeg.close();
      return false;
    }
    jpeg.close();

  } else {
    Serial.println("JPEGDEC failed to open image");
    free(buffer);
    return false;
  }

  free(buffer);
  drawTftJPEG();
  return true;
}

void showNowPlaying() {
  // Draw album art (top of screen)
  if (!downloadAndDrawImage(0, 0))
    Serial.println("Failed to print image");

  tft.fillRect(0, 241, 240, 120, TFT_BLACK);
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

String getItunesArtwork(String artist, String song) {
  if (artist == "" || song == "") {
    Serial.println("Missing song datas.");
    return "";
  }

  HTTPClient http;
  String term = artist + " " + song;
  term.replace(" ", "+");

  String url = "https://itunes.apple.com/search?term=" + term + "&entity=song&limit=1";

  http.begin(url);
  int code = http.GET();

  if (code != 200) {
    http.end();
    return "";
  }

  String payload = http.getString();
  http.end();

  JsonDocument imgJson;
  deserializeJson(imgJson, payload);

  if (imgJson["resultCount"] == 0) return "";

  String art = imgJson["results"][0]["artworkUrl100"].as<String>(); 
  art.replace("100x100bb", "120x120bb");
  art.replace("http://", "https://");

  return art;
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
  imageUrl   = "";
  artistName = ""; 
  songName   = "";

  // Check if something is currently playing
  if (track["@attr"]["nowplaying"] | false) {
    return true;
  }
  songName   = track["name"].as<String>();
  artistName = track["artist"]["#text"].as<String>();
  imageUrl   = getItunesArtwork(artistName, songName);

  if (imageUrl == "")
    imageUrl = "https://shared.fastly.steamstatic.com/community_assets/images/apps/636270/f2bab463067e93719abcc2678101eaa58b8f2fd2.jpg";
  
  return true;
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(SSID, PASSWORD);
  tft.init();
  tft.fillScreen(TFT_BLACK);

  //u8g2.begin();
  //u8g2.enableUTF8Print();

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

  imageBuffer = (uint16_t*)malloc(IMAGE_SIZE * IMAGE_SIZE * sizeof(uint16_t));
  if (!imageBuffer) {
      Serial.println("Failed to allocate image buffer");
  }
}

void loop() {
  unsigned long now = millis();

  /*if (now - updateOledTimer >= UPDATE_OLED_PERIOD) {
    updateOledTimer = now;
    drawOled();
  }

  if (now - updateTftTimer >= UPDATE_TFT_PERIOD) {
    updateTftTimer = now;
    count = (count + 1) % 3;
    drawTft();
  }*/

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
      Serial.println("===");
      showNowPlaying();
      Serial.println();
    }
  }
  ws.cleanupClients();
}