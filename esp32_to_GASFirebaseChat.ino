#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define API_KEY ""
#define DATABASE_URL ""

// BOT アカウント
#define BOT_EMAIL ""
#define BOT_PASSWORD ""

// DM の roomId
String roomId = "";

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

String inputBuffer = "";

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // ★ NTPで時刻同期（1970年問題の解決）
  configTime(9 * 3600, 0, "ntp.nict.jp", "pool.ntp.org");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = BOT_EMAIL;
  auth.user.password = BOT_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("ESP32 BOT Ready. Type message:");

  // ★ チャットのメッセージをリアルタイム購読開始
  String streamPath = "/rooms/" + roomId + "/messages";
  if (!Firebase.RTDB.beginStream(&stream, streamPath.c_str())) {
    Serial.println("Stream開始失敗: " + stream.errorReason());
  }
  pinMode(15,OUTPUT);
  pinMode(5,OUTPUT);
}

void loop() {
  // ★ ストリームの更新をチェック
  if (Firebase.RTDB.readStream(&stream)) {
    if (stream.streamAvailable()) {

      FirebaseJson json = stream.to<FirebaseJson>();

      FirebaseJsonData msgData, userData, uidData;

      json.get(msgData, "message");
      json.get(userData, "username");
      json.get(uidData, "uid");

      String message = msgData.stringValue;
      String username = userData.stringValue;
      String uid = uidData.stringValue;

      // BOT自身のメッセージは無視
      if (uid != auth.token.uid.c_str()) {
        Serial.println("【チャットからの返信】");
        Serial.println(username + ": " + message);
        digitalWrite(5,HIGH);
        delay(300);
        digitalWrite(5,LOW);
      }
    }
  }

  // シリアル入力 → チャット送信
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      sendMessage(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }
}

void sendMessage(String text) {
  if (text.length() == 0) return;

  if (!Firebase.ready()) {
    Serial.println("Firebase not ready");
    return;
  }

  String path = "/rooms/" + roomId + "/messages/" + String(millis());

  FirebaseJson json;
  json.set("username", "ESP32-BOT");
  json.set("uid", auth.token.uid.c_str());
  json.set("timestamp/.sv", "timestamp");  // ★サーバー時刻
  json.set("type", "text");
  json.set("message", text);

  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("送信成功: " + text);
    digitalWrite(15,HIGH);
    delay(500);
    digitalWrite(15,LOW);
  } else {
    Serial.println("送信失敗: " + fbdo.errorReason());
  }
}
