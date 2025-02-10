#include <Arduino.h>
#include <ESP32Servo.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Pin sensor
#define SERVO_PIN 14
#define SENSOR_HUJAN 34
#define SENSOR_CAHAYA 35

const char *ssid = "DLabs";
const char *password = "baliteam88";

// Telegram Credentials
#define BOTtoken "7249977403:AAFQDmS6vXFLegRJCoWI_8CCTErSshG44vE"
#define CHAT_ID "6868155856"

float nilaiHujan;
float nilaiCahaya;
Servo servo;

int lastAngle = 90;
int currentAngle = 0;
int targetAngle = 0;

bool notifhujan = false;
bool notifpanas = false;
bool notifmendung = false;
bool helloMessageSent = false;
bool waitForCommand = false; // Added missing semicolon
unsigned long lastTimeBotRan = 0;
const unsigned long BOT_CHECK_INTERVAL = 1000; // Added constant for bot check interval

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Error handling function
void handleError(const char *errorMessage)
{
  Serial.println(errorMessage);
  // Add any additional error handling here
}

// Fungsi membership untuk hujan ringan
float hujanRingan(float x)
{
  if (x <= 800)
    return 0;
  if (x >= 1200)
    return 1;
  return (x - 800) / 400.0; // Transisi dari 800 ke 1200
}

// Fungsi membership untuk hujan sedang
float hujanSedang(float x)
{
  if (x <= 1200 || x >= 3000)
    return 0;
  if (x > 1200 && x < 2000)
    return (x - 1200) / 800.0; // Naik dari 1200 ke 2000
  if (x >= 2000 && x < 3000)
    return (3000 - x) / 1000.0; // Turun dari 2000 ke 3000
  return 0;
}

// Fungsi membership untuk hujan tinggi
float hujanTinggi(float x)
{
  if (x >= 4095)
    return 1;
  if (x <= 3000)
    return 0;
  return (x - 3000) / 1095.0; // Transisi dari 3000 ke 4095
}

// Fungsi membership cahaya
float cahayaRendah(float x)
{
  if (x <= 1000)
    return 1;
  if (x >= 2000)
    return 0;
  return (2000 - x) / 1000.0;
}

float cahayaTinggi(float x)
{
  if (x <= 2000)
    return 0;
  if (x >= 3000)
    return 1;
  return (x - 2000) / 1000.0;
}

void moveServoSmoothly(int newAngle)
{
  targetAngle = constrain(newAngle, 0, 90); // Added angle constraints

  while (currentAngle != targetAngle)
  {
    if (currentAngle < targetAngle)
    {
      currentAngle = min(currentAngle + 2, targetAngle);
    }
    else
    {
      currentAngle = max(currentAngle - 2, targetAngle);
    }

    servo.write(currentAngle);
    delay(60);
  }
}

void handleNewMessages(int numNewMessages) {
  Serial.println("Handling new messages");
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    Serial.print("Pesan diterima: "); Serial.println(text);
    Serial.print("Status waitForCommand: "); Serial.println(waitForCommand);

    if (text == "/start") {
      String welcome = "Selamat datang di Sistem Kontrol Jemuran Otomatis!\n";
      welcome += "Gunakan perintah berikut:\n";
      welcome += "/status - Cek status jemuran\n";
      welcome += "angkat - Mengangkat jemuran\n";
      bot.sendMessage(chat_id, welcome, "");
    }
    else if (text == "/status") {
      String status = "Status Jemuran:\n";
      status += "Nilai Hujan: " + String(nilaiHujan) + "\n";
      status += "Nilai Cahaya: " + String(nilaiCahaya) + "\n";
      status += "Posisi Servo: " + String(currentAngle) + "°";
      bot.sendMessage(chat_id, status, "");
    }
    else if (text == "angkat") {
      if (waitForCommand) {
        Serial.println("Perintah 'angkat' diterima, menggerakkan servo...");
        moveServoSmoothly(90);
        bot.sendMessage(chat_id, "Jemuran sudah diangkat! ✅", "");
        waitForCommand = false;
        notifmendung = false;
      } else {
        Serial.println("Perintah 'angkat' diabaikan karena waitForCommand = false.");
        bot.sendMessage(chat_id, "Tidak ada perintah menunggu. Jemuran tidak dalam kondisi mendung.", "");
      }
    }
  }
}


void connectWiFi()
{
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  { // Added connection timeout
    Serial.print(".");
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    handleError("WiFi connection failed!");
  }
}

void setup()
{
  Serial.begin(115200);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  analogSetAttenuation(ADC_11db);

  pinMode(SENSOR_HUJAN, INPUT);
  pinMode(SENSOR_CAHAYA, INPUT);

  servo.setPeriodHertz(50);

  servo.attach(SERVO_PIN, 500, 2400);
  if (!servo.attached())
  {
    handleError("Servo attachment failed!");
  }
  else
  {
    Serial.println("Servo attached successfully!");
  }
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  moveServoSmoothly(90);

  connectWiFi();

  if (!helloMessageSent)
  {
    bot.sendMessage(CHAT_ID, "Sistem Jemuran Otomatis telah aktif! 🚀", "");
    helloMessageSent = true;
  }
}

void loop()
{
  nilaiHujan = 4095 - analogRead(SENSOR_HUJAN);
  nilaiCahaya = 4095 - analogRead(SENSOR_CAHAYA);

  // Hitung derajat keanggotaan fuzzy
  float hujan_tinggi = hujanTinggi(nilaiHujan);
  float hujan_sedang = hujanSedang(nilaiHujan);
  float cahaya_rendah = cahayaRendah(nilaiCahaya);
  float cahaya_tinggi = cahayaTinggi(nilaiCahaya);

  Serial.printf("Nilai Sensor Hujan: %.1f\nNilai Sensor Cahaya: %.1f\n", nilaiHujan, nilaiCahaya);
  Serial.print("Kondisi Cuaca: ");
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectWiFi();
  }

  // Cek kondisi hujan terlebih dahulu
  if (hujan_tinggi >= 0.5 || hujan_sedang >= 0.7)
  {
    Serial.println("Hujan");
    moveServoSmoothly(90);
    if (!notifhujan)
    {
      bot.sendMessage(CHAT_ID, "Hujan terdeteksi ⛈️, jemuran otomatis diangkat!", "");
      notifhujan = true;
      notifmendung = false;
      notifpanas = false;
      waitForCommand = false;
    }
  }
  // Jika tidak hujan, cek kondisi cahaya
  else if (cahaya_rendah < cahaya_tinggi)
  {
    Serial.println("Cerah");
    moveServoSmoothly(0);
    if (!notifpanas)
    {
      bot.sendMessage(CHAT_ID, "☀️ Cuaca Cerah, Jemuran akan diturunkan!", "");
      notifpanas = true;
      notifhujan = false;
      notifmendung = false;
      waitForCommand = false;
    }
  }
  else
  {
    Serial.println("Mendung");
    if (!notifmendung)
    {
      bot.sendMessage(CHAT_ID, "Cuaca mendung 🌥️, ketik 'angkat' untuk mengangkat jemuran", "");
      notifmendung = true;
      notifhujan = false;
      notifpanas = false;
      waitForCommand = true;
    }
  }

  if (millis() - lastTimeBotRan > BOT_CHECK_INTERVAL)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0)
    {
      handleNewMessages(numNewMessages);
    }
    lastTimeBotRan = millis();
  }

  vTaskDelay(pdMS_TO_TICKS(500));
}
