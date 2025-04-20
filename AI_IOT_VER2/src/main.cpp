#include <WiFi.h>
#include <Wire.h>
#include <AHT20.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <U8g2lib.h>
#include <time.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
// CẤU HÌNH CHÂN
#define BOM_DC 19
#define DEN_STATUS 18
#define WATER_SENSOR_PIN 35
#define WATER_THRESHOLD 2200
#define ONE_WIRE_BUS 16

// CẤU HÌNH WIFI
const char *apSSID = "ESP32_Config";
const char *ssid = "Villa_3lau";
const char *password = "23456778";
// THÔNG TIN FIREBASE
#define API_KEY "AIzaSyDHtnjos6QcmQrNpFMi8JpKVSP3ojZnIPI"
#define DATABASE_URL "https://aquariumsmart-4d96d-default-rtdb.europe-west1.firebasedatabase.app/"
#define USER_EMAIL "vythaidung2003@gmail.com"
#define USER_PASSWORD "dung1802@"
// / KHỞI TẠO CÁC ĐỐI TƯỢNG
Servo myServo;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
AHT20 aht20;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Preferences preferences;
AsyncWebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);
// BIẾN TOÀN CỤC
unsigned long previousMillis = 0;
unsigned long interval = 30000;
unsigned long sendDataPrevMillis = 0;
unsigned long GetDataPrevMillis = 0;
unsigned long timerDelay = 10000;
unsigned long timerDelayGet = 5000;
String uid;
String databasePath;
String databasePathController;
String tempPath = "/temperature";
String humPath = "/humidity";
String waterTempPath = "/water_temperature";
String timePath = "/timestamp";
String parentPath;
int timestamp;
FirebaseJson json;
int feed;
int bom;
int den;
int servoFeed[1][2];
int timeRangeDen[2];

// KHỞI TẠO AHT20
void initAHT20()
{
  Wire.begin();
  delay(100);
  Serial.println("Initializing AHT20...");
  if (!aht20.begin())
  {
    Serial.println(" AHT20 sensor not found! Check wiring.");
    while (1)
    {
      delay(1000);
    }
    
  }
  Serial.println(" AHT20 initialized!");
}
// KHỞI TẠO OLED
void initOLED()
{
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.setCursor(10, 20);
  u8g2.print("SMART AQUARIUM!");
  u8g2.sendBuffer();
  delay(2000);
}
// CỔNG CẤU HÌNH WIFI
void startWiFiConfigPortal()
{
  WiFi.softAP(apSSID);
  Serial.println("\n ESP32 ở chế độ AP: " + String(apSSID));
  Serial.println("IP: " + WiFi.softAPIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String html = "<h2>Cấu hình WiFi</h2>"
                  "<form action='/save' method='POST'>"
                  "SSID: <input type='text' name='ssid'><br>"
                  "Password: <input type='password' name='password'><br>"
                  "<input type='submit' value='Kết nối'></form>";
    request->send(200, "text/html", html); });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      request->send(200, "text/html", "<h2> WiFi đã lưu! ESP32 đang khởi động lại...</h2>");
      delay(2000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", " Thiếu thông tin WiFi!");
    } });

  server.begin();
}

// TRANG THAY ĐỔI WIFI
void startWiFiChangePage()
{
  server.on("/change_wifi", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String html = "<!DOCTYPE html><html lang='vi'><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Cấu Hình WiFi ESP32</title>";
    html += "<style>body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f4; }";
    html += ".container { background: white; padding: 20px; margin: 50px auto; width: 300px; ";
    html += "border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.2); }";
    html += "h2 { color: #333; }";
    html += "input { width: 100%; padding: 8px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; }";
    html += "button { background: #28a745; color: white; padding: 10px; border: none; ";
    html += "border-radius: 5px; cursor: pointer; width: 100%; }";
    html += "button:hover { background: #218838; }</style></head>";
    html += "<body><div class='container'>";
    html += "<h2>Cấu Hình WiFi ESP32</h2>";
    html += "<form action='/save_wifi' method='POST'>";
    html += "<label>SSID:</label><br><input type='text' name='ssid' required><br>";
    html += "<label>Mật khẩu:</label><br><input type='password' name='password' required><br>";
    html += "<button type='submit'>Lưu & Kết Nối</button></form></div></body></html>";
    request->send(200, "text/html", html); });

  server.on("/save_wifi", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      request->send(200, "text/html", "<h2> WiFi đã lưu! ESP32 đang khởi động lại...</h2>");
      delay(2000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", " Thiếu thông tin WiFi!");
    } });

  server.begin();
}
// KHỞI TẠO WIFI
void initWiFi()
{
  preferences.begin("wifi", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");

  if (savedSSID != "" && savedPassword != "")
  {
    Serial.println("Đang kết nối WiFi...");
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    int timeout = 20;
    while (WiFi.status() != WL_CONNECTED && timeout > 0)
    {
      delay(500);
      Serial.print(".");
      timeout--;
    }
  }
  else
  {
    WiFi.begin(ssid, password);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n Kết nối thành công!");
    Serial.println(" IP: " + WiFi.localIP().toString());
  }
  else
  {
    startWiFiConfigPortal();
  }
  startWiFiChangePage();
}

// LẤY THỜI GIAN THỰC
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println(" Failed to get time");
    return 0;
  }
  time(&now);
  return now;
}

// GỬI DỮ LIỆU LÊN FIREBASE
void sendDataToFirebase(float temperature, float humidity, float waterTemp)
{
  Serial.println(" Sending data to Firebase...");
  json.set(tempPath, String(temperature));
  json.set(humPath, String(humidity));
  json.set(waterTempPath, String(waterTemp));
  json.set(timePath, String(timestamp));
  Serial.printf(" Sending... %s\n",
                Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? " Success" : fbdo.errorReason().c_str());
}

// CẬP NHẬT MÀN HÌNH OLED
void updateOLED(float temperature, float humidity, float waterTemp)
{
  u8g2.clearBuffer();
  u8g2.setCursor(10, 15);
  u8g2.print("Temp: ");
  u8g2.print(temperature);
  u8g2.print(" C");
  u8g2.setCursor(10, 35);
  u8g2.print("Humi: ");
  u8g2.print(humidity);
  u8g2.print(" %");
  u8g2.setCursor(10, 55);
  u8g2.print("Water Temp: ");
  u8g2.print(waterTemp);
  u8g2.print(" C");
  u8g2.sendBuffer();
}
void setup()
{
  Serial.begin(9600);
  initOLED();
  initWiFi();
  initAHT20();

  pinMode(WATER_SENSOR_PIN, OUTPUT);
  pinMode(BOM_DC, OUTPUT);
  pinMode(DEN_STATUS, OUTPUT);
  digitalWrite(WATER_SENSOR_PIN, LOW);
  digitalWrite(BOM_DC, HIGH);
  digitalWrite(DEN_STATUS, HIGH);

  myServo.setPeriodHertz(50);
  sensors.begin();
  configTime(0, 0, "pool.ntp.org");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println(" Getting User UID...");
  while (auth.token.uid.length() == 0)
  {
    Serial.print('.');
    delay(1000);
  }
  uid = auth.token.uid.c_str();
  Serial.println("\n UID: " + uid);
  databasePath = "/UsersData/" + uid + "/readings";
  databasePathController = "/UsersData/" + uid + "/controller";
}

// ĐỌC DỮ LIỆU TỪ FIREBASE
void read()
{
  if (Firebase.ready() && (millis() - GetDataPrevMillis > timerDelayGet || GetDataPrevMillis == 0))
  {
    GetDataPrevMillis = millis();
    if (Firebase.RTDB.getJSON(&fbdo, databasePathController))
    {
      FirebaseJsonData jsondata;
      FirebaseJson json = fbdo.to<FirebaseJson>();
      if (json.get(jsondata, "Feed"))
        feed = jsondata.to<int>();
      if (json.get(jsondata, "bom"))
        bom = jsondata.to<int>();
      if (json.get(jsondata, "den"))
        den = jsondata.to<int>();
      if (json.get(jsondata, "ServoFeed/AtTime1"))
        servoFeed[0][0] = jsondata.to<int>();
      if (json.get(jsondata, "ServoFeed/Times1"))
        servoFeed[0][1] = jsondata.to<int>();
      if (json.get(jsondata, "ServoFeed/AtTime2"))
        servoFeed[0][0] = jsondata.to<int>();
      if (json.get(jsondata, "ServoFeed/Times2"))
        servoFeed[0][1] = jsondata.to<int>();
      if (json.get(jsondata, "timeRangeDen/StartTime"))
        timeRangeDen[0] = jsondata.to<int>();
      if (json.get(jsondata, "timeRangeDen/EndTime"))
        timeRangeDen[1] = jsondata.to<int>();
      
    }
  }
}

// ĐỌC DỮ LIỆU NHIỆT ĐỘ, ĐỘ ẨM
void humi_temp_data()
{
  float temperature = aht20.getTemperature();
  float humidity = aht20.getHumidity();
  sensors.requestTemperatures();
  float waterTemp = sensors.getTempCByIndex(0);

  if (temperature < -40 || temperature > 85)
  {
    Serial.println(" Invalid air temperature!");
    return;
  }
  if (humidity < 0 || humidity > 100)
  {
    Serial.println(" Invalid humidity!");
    return;
  }
  if (waterTemp < -10 || waterTemp > 125)
  {
    Serial.println(" Invalid water temperature!");
    return;
  }

  Serial.print(" Temp: ");
  Serial.print(temperature);
  Serial.print("°C,  Humi: ");
  Serial.print(humidity);
  Serial.print("%,  Water Temp: ");
  Serial.print(waterTemp);
  Serial.println("°C");

  updateOLED(temperature, humidity, waterTemp);

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    timestamp = getTime();
    parentPath = databasePath;
    sendDataToFirebase(temperature, humidity, waterTemp);
  }
  timeClient.begin();
}

// ĐIỀU KHIỂN BƠM
bool autoPump = true;
bool overridePump = false;
bool isPumpOn = false;
void controlPump()
{
  String path = "/UsersData/" + uid + "/controller/bom";
  if (Firebase.RTDB.getBool(&fbdo, path.c_str()))
  {
    if (fbdo.dataType() == "boolean")
    {
      bool pumpState = fbdo.boolData();
      Serial.print(" Trạng thái bơm từ Firebase: ");
      Serial.println(pumpState ? "BẬT" : "TẮT");
      if (pumpState != isPumpOn)
      {
        isPumpOn = pumpState;
        digitalWrite(BOM_DC, isPumpOn ? LOW : HIGH);
        Serial.println(isPumpOn ? " Bơm đã BẬT" : " Bơm đã TẮT");
        overridePump = true;
      }
    }
  }
  else
  {
    Serial.print(" Lỗi đọc Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

// ĐIỀU KHIỂN BƠM TỰ ĐỘNG
void control_water_sensor()
{
  int waterLevel = analogRead(WATER_SENSOR_PIN);
  if (waterLevel < WATER_THRESHOLD)
  {
    Serial.println(" Không có nước! Bật bơm.");
    digitalWrite(BOM_DC, LOW);
  }
  else if (waterLevel >= WATER_THRESHOLD)
  {
    Serial.println(" Có nước! Tắt bơm.");
    digitalWrite(BOM_DC, HIGH);
  }
}

// ĐIỀU KHIỂN ĐÈN
bool autoLight = true;
bool overrideLight = false;
bool isLightOn = false;
void controlLight()
{
  String path = "/UsersData/" + uid + "/controller/den";
  if (Firebase.RTDB.getBool(&fbdo, path.c_str()))
  {
    if (fbdo.dataType() == "boolean")
    {
      bool lightState = fbdo.boolData();
      Serial.print(" Trạng thái đèn từ Firebase: ");
      Serial.println(lightState ? "BẬT" : "TẮT");
      if (lightState != isLightOn)
      {
        isLightOn = lightState;
        digitalWrite(DEN_STATUS, isLightOn ? LOW : HIGH);
        Serial.println(isLightOn ? " Đèn đã BẬT" : " Đèn đã TẮT");
        overrideLight = true;
      }
    }
  }
  else
  {
    Serial.print(" Lỗi đọc Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

// ĐIỀU KHIỂN ĐÈN TỰ ĐỘNG
bool previousLightState = false;
bool Auto_Control = true;
void controlLightAuto()
{
  String path = "/UsersData/" + uid + "/controller/AutoLED";
  if (Firebase.RTDB.getBool(&fbdo, path.c_str()))
  {
    if (fbdo.dataType() == "boolean")
    {
      bool lightState = fbdo.boolData();
      Serial.print(" Trạng thái đèn từ Firebase: ");
      Serial.println(lightState ? "BẬT AUTO" : "TẮT AUTO");
    }
  }

  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes(); 
  int currentTime = currentHour * 60 + currentMinute;

  String startTimeStr, endTimeStr;
  if (Firebase.RTDB.getJSON(&fbdo, "/UsersData/" + uid + "/controller/timeRangeDen"))
  {
    FirebaseJson json = fbdo.to<FirebaseJson>();
    FirebaseJsonData jsonData;
    if (json.get(jsonData, "StartTime"))
      startTimeStr = jsonData.to<String>();
    else
    {
      Serial.println(" Lỗi đọc StartTime!");
      return;
    }
    if (json.get(jsonData, "EndTime"))
      endTimeStr = jsonData.to<String>();
    else
    {
      Serial.println(" Lỗi đọc EndTime!");
      return;
    }
  }
  else
  {
    Serial.print(" Lỗi đọc Firebase: ");
    Serial.println(fbdo.errorReason());
    return;
  }

  int startHour = startTimeStr.substring(0, 2).toInt();
  int startMin = startTimeStr.substring(3, 5).toInt();
  int endHour = endTimeStr.substring(0, 2).toInt();
  int endMin = endTimeStr.substring(3, 5).toInt();
  int startTime = startHour * 60 + startMin;
  int endTime = endHour * 60 + endMin;

  Serial.printf(" Giờ hiện tại: %02d:%02d\n", currentHour, currentMinute);
  Serial.printf(" Đèn tự động: Bật %02d:%02d - Tắt %02d:%02d\n", startHour, startMin, endHour, endMin);

  bool newLightState = false;
  if (!overrideLight)
  {
    if (currentTime >= startTime && currentTime < endTime)
      newLightState = true;
    else
      newLightState = false;
  }
  else
  {
    Serial.println(" Đèn đang ở chế độ ghi đè! Không tự động điều khiển.");
    return;
  }

  if (newLightState != previousLightState)
  {
    digitalWrite(DEN_STATUS, newLightState ? LOW : HIGH);
    previousLightState = newLightState;
    Serial.printf(" Trạng thái đèn thay đổi: %s\n", newLightState ? "BẬT" : "TẮT");
  }
}

// ĐIỀU KHIỂN CHO CÁ ĂN
bool isFeedOn = false;
void control_feed()
{
  String path = "/UsersData/" + uid + "/controller/feed";
  timestamp = getTime();
  String path1 = "/UsersData/" + uid + "/controller/Hisfeed/" + String(timestamp);
  if (Firebase.RTDB.getBool(&fbdo, path.c_str()))
  {
    if (fbdo.dataType() == "boolean")
    {
      bool feedState = fbdo.boolData();
      Serial.print("🍽 Trạng thái từ Firebase cho cá ăn : ");
      Serial.println(feedState ? "BẬT" : "TẮT");
      if (feedState && !isFeedOn)
      {
        isFeedOn = true;
        myServo.attach(17, 500, 2400);
        myServo.write(40);
        Serial.println(" Đang cho cá ăn...");
        delay(2000);
        myServo.write(55);
        Serial.println(" Đã đóng nắp thức ăn");
        delay(1000);
        myServo.detach();
        Firebase.RTDB.setBool(&fbdo, path.c_str(), false);
        isFeedOn = false;
        Firebase.RTDB.setInt(&fbdo, path1.c_str(), (int)timestamp);
        
      }
    }
  }
  else
  {
    Serial.print(" Lỗi đọc Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

// ĐIỀU KHIỂN CHO CÁ ĂN TỰ ĐỘNG LẦN 1
bool autoFeed1 = true;
bool overrideFeed1 = false;
bool isFeedOn1 = false;
void control_feed1()
{
  String path = "/UsersData/" + uid + "/controller/autoFeeding1";
  if (Firebase.RTDB.getBool(&fbdo, path.c_str()))
  {
    if (fbdo.dataType() == "boolean")
    {
      bool feedState = fbdo.boolData();
      Serial.print("🍽 Trạng thái từ Firebase cho cá ăn : ");
      Serial.println(feedState ? "BẬT" : "TẮT");
    }
  }

  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentTime = currentHour * 60 + currentMinute;

  String startTimeStr, endTimeStr;
  if (Firebase.RTDB.getJSON(&fbdo, "/UsersData/" + uid + "/controller/ServoFeed"))
  {
    FirebaseJson json = fbdo.to<FirebaseJson>();
    FirebaseJsonData jsonData;
    if (json.get(jsonData, "AtTime1"))
      startTimeStr = jsonData.to<String>();
    else
    {
      Serial.println(" Lỗi đọc AtTime1!");
      return;
    }
    if (json.get(jsonData, "Times1"))
      endTimeStr = jsonData.to<String>();
    else
    {
      Serial.println(" Lỗi đọc Time1!");
      return;
    }
  }
  else
  {
    Serial.print(" Lỗi đọc Firebase: ");
    Serial.println(fbdo.errorReason());
    return;
  }

  int startHour = startTimeStr.substring(0, 2).toInt();
  int startMin = startTimeStr.substring(3, 5).toInt();
  int endHour = endTimeStr.substring(0, 2).toInt();
  int endMin = endTimeStr.substring(3, 5).toInt();
  int startTime = startHour * 60 + startMin;
  int endTime = endHour * 60 + endMin;

  Serial.printf(" Giờ hiện tại: %02d:%02d\n", currentHour, currentMinute);
  Serial.printf(" Cho cá ăn tự động: Bật %02d:%02d ", startHour, startMin);

  bool newFeedState = false;
  if (!overrideFeed1)
  {
    if (currentTime >= startTime)
      newFeedState = true;
    else
      newFeedState = false;
  }
  else
  {
    Serial.println(" Cho cá ăn đang ở chế độ ghi đè! Không tự động điều khiển.");
    return;
  }

  if (newFeedState != isFeedOn1)
  {
    if (newFeedState)
    {
      isFeedOn1 = true;
      myServo.attach(17, 500, 2400);
      myServo.write(40);
      Serial.println(" Đang cho cá ăn...");
      delay(2000);
      myServo.write(55);
      Serial.println(" Đã đóng nắp thức ăn");
      delay(1000);
      myServo.detach();
    }
    else
    {
      isFeedOn1 = false;
      Serial.println(" Đã tắt chế độ cho cá ăn tự động");
    }
  }

  if (isFeedOn1)
    Firebase.RTDB.setBool(&fbdo, path.c_str(), false);
}

// ĐIỀU KHIỂN CHO CÁ ĂN TỰ ĐỘNG LẦN 2
bool autoFeed2 = true;
bool overrideFeed2 = false;
bool isFeedOn2 = false;
void control_feed2()
{
  String path = "/UsersData/" + uid + "/controller/autoFeeding2";
  if (Firebase.RTDB.getBool(&fbdo, path.c_str()))
  {
    if (fbdo.dataType() == "boolean")
    {
      bool feedState = fbdo.boolData();
      Serial.print(" Trạng thái từ Firebase cho cá ăn : ");
      Serial.println(feedState ? "BẬT" : "TẮT");
    }
  }

  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentTime = currentHour * 60 + currentMinute;

  String startTimeStr, endTimeStr;
  if (Firebase.RTDB.getJSON(&fbdo, "/UsersData/" + uid + "/controller/ServoFeed"))
  {
    FirebaseJson json = fbdo.to<FirebaseJson>();
    FirebaseJsonData jsonData;
    if (json.get(jsonData, "AtTime2"))
      startTimeStr = jsonData.to<String>();
    else
    {
      Serial.println(" Lỗi đọc AtTime2!");
      return;
    }
    if (json.get(jsonData, "Times2"))
      endTimeStr = jsonData.to<String>();
    else
    {
      Serial.println(" Lỗi đọc Time2!");
      return;
    }
  }
  else
  {
    Serial.print(" Lỗi đọc Firebase: ");
    Serial.println(fbdo.errorReason());
    return;
  }

  int startHour = startTimeStr.substring(0, 2).toInt();
  int startMin = startTimeStr.substring(3, 5).toInt();
  int endHour = endTimeStr.substring(0, 2).toInt();
  int endMin = endTimeStr.substring(3, 5).toInt();
  int startTime = startHour * 60 + startMin;
  int endTime = endHour * 60 + endMin;

  Serial.printf(" Giờ hiện tại: %02d:%02d\n", currentHour, currentMinute);
  Serial.printf(" Cho cá ăn tự động: Bật %02d:%02d ", startHour, startMin);

  bool newFeedState = false;
  if (!overrideFeed2)
  {
    if (currentTime >= startTime)
      newFeedState = true;
    else
      newFeedState = false;
  }
  else
  {
    Serial.println(" Cho cá ăn đang ở chế độ ghi đè! Không tự động điều khiển.");
    return;
  }

  if (newFeedState != isFeedOn2)
  {
    if (newFeedState)
    {
      isFeedOn2 = true;
      myServo.attach(17, 500, 2400);
      myServo.write(40);
      Serial.println(" Đang cho cá ăn...");
      delay(1000);
      myServo.write(55);
      Serial.println(" Đã đóng nắp thức ăn");
      delay(1000);
      myServo.detach();
    }
    else
    {
      isFeedOn2 = false;
      Serial.println(" Đã tắt chế độ cho cá ăn tự động");
    }
  }

  if (isFeedOn2)
    Firebase.RTDB.setBool(&fbdo, path.c_str(), false);
}


void loop()
{
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval))
  {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }

  humi_temp_data();
  read();
  control_water_sensor();
  controlPump();
  controlLight();
  controlLightAuto();
  control_feed();
  control_feed1();
  control_feed2();
}