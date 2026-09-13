
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
#include <base64.h>
#include <Preferences.h>

#include "env.h"

#define UPDATE_OLED_PERIOD 2000
#define UPDATE_TFT_PERIOD 500
#define RECONNECT_PERIOD 1000
#define NOTIFY_PERIOD 1000
#define API_PERIOD 60000

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

String clientId     = "";
String clientSecret = "";
String refreshToken = "";
String accessToken  = "";

String imageUrl   = "";
String songName   = "";
String artistName = ""; 

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JsonDocument telemetryJson;
Preferences preferences;

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
TFT_eSPI tft = TFT_eSPI();

void saveTokens(String token) {
  preferences.begin("spotify", false); //read-write
  preferences.putString("refresh", token);
  preferences.end();
}

void saveUser(String id, String secret) {
  preferences.begin("spotify", false); //read-write
  preferences.putString("id", id);
  preferences.putString("secret", secret);
  preferences.end();
}

String loadRefreshToken() {
  preferences.begin("spotify", true); //read-only
  String token = preferences.getString("refresh", "");
  preferences.end();
  return token;
}

String loadClientID() {
  preferences.begin("spotify", true); //read-only
  String id = preferences.getString("id", "");
  preferences.end();
  return id;
}

String loadClientSecret() {
  preferences.begin("spotify", true); //read-only
  String secret = preferences.getString("secret", "");
  preferences.end();
  return secret;
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
  switch (count) {
    case 0: {
      tft.fillScreen(TFT_RED);
      break;
    }
    case 1: {
      tft.fillScreen(TFT_GREEN);
      break;
    }
    case 2: {
      tft.fillScreen(TFT_BLUE);
      break;
    }
  }
}

bool refreshAccessToken() {
  if (refreshToken == "" || clientSecret == "" || clientSecret == "") {
    Serial.println("Not enough authentication datas.");
    return false;
  }

  HTTPClient http;
  http.begin("https://accounts.spotify.com/api/token");
  
  // Basic Auth (Base64 of client_id:client_secret)
  String auth = "Basic " + base64::encode(clientId + ":" + clientSecret);
  http.addHeader("Authorization", auth);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "grant_type=refresh_token&refresh_token=" + refreshToken;
  Serial.println(auth);

  int httpCode = http.POST(body);
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    JsonDocument tokenJson;
    deserializeJson(tokenJson, payload);
    
    accessToken = tokenJson["access_token"].as<String>();
    Serial.println("Access token refreshed");
    http.end();
    return true;
  } else {
    Serial.printf("Token refresh failed: %d\n", httpCode);
    Serial.println(http.getString());
    http.end();
    return false;
  }
}

bool getCurrentSongData() {
  if (accessToken == "") {
    if (!refreshAccessToken()) return false;
  }

  HTTPClient http;
  http.begin("https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization", "Bearer " + accessToken);

  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    http.end();

    JsonDocument songDataJson;
    deserializeJson(songDataJson, payload);

    // Check if something is playing
    if (songDataJson["item"].isNull()) {
      imageUrl   = "";
      songName   = "Nothing is currently playing";
      artistName = ""; 
      return true;
    }

    imageUrl   = (String)songDataJson["item"]["album"]["images"][1]["url"];
    songName   = (String)songDataJson["item"]["name"];
    artistName = (String)songDataJson["item"]["artists"][0]["name"]; 

    return true;
  } 
  else if (httpCode == 401) {
    // Token expired
    http.end();
    if (refreshAccessToken()) {
      return getCurrentSongData();   // retry once
    }
  }
  else {
    Serial.printf("Error: %d\n", httpCode);
    Serial.println(http.getString());
  }
  
  http.end();
  return false;
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(SSID, PASSWORD);
  tft.init();
  u8g2.begin();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  delay(1000);

  // #define in env.h
  if (REFRESH_TOKEN == "")
    refreshToken = loadRefreshToken();
  else {
    refreshToken = REFRESH_TOKEN;   
    saveTokens(refreshToken);
  }
  // #define in env.h
  if (CLIENT_ID == "" || CLIENT_SECRET == "") {
    clientId = loadClientID();
    clientSecret = loadClientSecret();
  }
  else {
    clientId = CLIENT_ID;
    clientSecret = CLIENT_SECRET;
    saveUser(clientId, clientSecret);
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
    Serial.println(getCurrentSongData());
  }

  ws.cleanupClients();
}