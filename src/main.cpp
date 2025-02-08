
#include <Arduino.h>
#include <ESP32Servo.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "Library/LibAir/rain.h"
#include "Library/LibCahaya/cahaya.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define LED 4
#define SERVO_PIN 14
#define SENSOR_PIN_RAIN 34
#define SENSOR_PIN_LIGHT 35

#define DELAY 50
#define MotorInterfaceType 4
#define BOTtoken "7249977403:AAFQDmS6vXFLegRJCoWI_8CCTErSshG44vE"
#define CHAT_ID "6868155856"

Rain sensorair(SENSOR_PIN_RAIN);
Cahaya sensorcahaya(SENSOR_PIN_LIGHT);
Servo servo;
int lastAngle = 0;
int currentAngle = 0;
int targetAngle = 0;
const int STEP_DELAY = 50;
const int ANGLE_STEP = 4;
unsigned long lastTimeBotRan = 0;

bool notifhujan = false;
bool notifpanas = false;
bool notifmendung = false;
bool helloMessageSent = false;
const char *ssid = "DLabs";
const char *password = "baliteam88";

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void safePinWrite(uint8_t pin, uint8_t val)
{
    if (pin < NUM_DIGITAL_PINS)
    {
        digitalWrite(pin, val);
    }
}
// Fungsi keanggotaan untuk sensor cahaya
float keanggotaanCahaya(int value)
{
    // Gelap: 0-255, Terang: 255-511
    if (value <= 0)
        return 0;
    if (value >= 511)
        return 1;
    return value / 511.0;
}

// Fungsi keanggotaan untuk sensor hujan
float keanggotaanAir(int value)
{
    // ringan: 0-480, Basah: 480-960
    if (value <= 0)
        return 0;
    if (value >= 960)
        return 1;
    return value / 960.0;
}
void moveServoSmoothly(int newAngle)
{
    targetAngle = newAngle;

    while (currentAngle != targetAngle)
    {
        if (currentAngle < targetAngle)
        {
            currentAngle = min(currentAngle + ANGLE_STEP, targetAngle);
        }
        else
        {
            currentAngle = max(currentAngle - ANGLE_STEP, targetAngle);
        }

        servo.write(currentAngle);
        delay(STEP_DELAY);
    }
}
// Fungsi defuzzifikasi Tsukamoto
int getServoAngle(float light, float rain)
{
    // Fuzzifikasi
    float CahayaTerang = keanggotaanCahaya(light);
    float CahayaGelap = 1 - CahayaTerang;
    float Hujan = keanggotaanAir(rain);
    float TidakHujan = 1 - Hujan;

    // Rule 1: IF Cahaya Terang AND TidakHujan THEN Sudut Besar (90°)
    // Rule 2: IF Hujan THEN Sudut Kecil (0°)
    // Rule 3: IF Cahaya Terang AND Hujan THEN Tunggu
    // Rule 4: IF Cahaya Gelap AND TidakHujan THEN Pertahankan posisi terakhir

    // Jika hujan, pindah ke 90°
    if (Hujan > 0.5)
    {
        lastAngle = 90;
        if (!notifhujan)
        {
            bot.sendMessage(CHAT_ID, "Cuaca Hujan, Jemuran akan di angkat..!", "");
            notifhujan = true;
            notifpanas = false;
        }
        return 90;
    }

    // Jika cahaya terang dan tidak hujan, pindah ke 90°
    if (CahayaTerang > 0.5 && TidakHujan > 0.5)
    {
        lastAngle = 0;

        if (!notifpanas)
        {
            bot.sendMessage(CHAT_ID, "Cuaca Panas, Jemur lagi ya..!", "");
            notifpanas = true;
            notifhujan = false;
        }
        return 0;
    }

    // Jika tidak hujan tapi cahaya gelap, pertahankan posisi terakhir
    if (TidakHujan > 0.5 && CahayaGelap > 0.5)
    {
        if (!notifmendung)
        {

            bot.sendMessage(CHAT_ID, "Cuaca mendung, Apa mau di angkat jemurannya ..?", "");
            notifmendung = true;
            notifpanas = false;
            notifhujan = false;
        }

        return lastAngle;
    }

    // Untuk kondisi lainnya, pertahankan posisi terakhir
    return lastAngle;
}
void handleTelegramMessages()
{
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
        
        for (int i = 0; i < numNewMessages; i++)
        {
            String chat_id = String(bot.messages[i].chat_id);
            String text = bot.messages[i].text;
            if (chat_id == CHAT_ID)
            {
                if (text == "angkat")
                {
                    lastAngle = 90;
                    bot.sendMessage(CHAT_ID, "Ok, Jemuran Sudah di angkat...!", "");
                }
               
            }
        }
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
}

void setup()
{
    Serial.begin(115200);
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    analogSetAttenuation(ADC_11db);

    pinMode(LED, OUTPUT);
    safePinWrite(LED, 0);
    pinMode(SENSOR_PIN_RAIN, INPUT);
    pinMode(SENSOR_PIN_LIGHT, INPUT);
    servo.setPeriodHertz(100);
    servo.attach(SERVO_PIN);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    servo.write(90);

    // Attempt to connect to Wifi network:
    Serial.print("Connecting Wifi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    if (!helloMessageSent)
    {
        bot.sendMessage(CHAT_ID, "Hallo I'M Rain Assistant 😊", "");
        helloMessageSent = true;
    }
}

void loop()
{

    int data_air = sensorair.readSensor();
    int data_cahaya = sensorcahaya.readSensor();
    data_air = data_air >> 1;
    data_cahaya = data_cahaya >> 1;

    // Hitung sudut servo menggunakan fuzzy
    int servoAngle = getServoAngle(data_cahaya, data_air);
    moveServoSmoothly(servoAngle);
    Serial.println("Air: " + String(data_air) +
                   " Cahaya: " + String(data_cahaya) +
                   " Sudut Target: " + String(servoAngle) +
                   " Sudut Sekarang: " + String(currentAngle));
    if (millis() - lastTimeBotRan > 1000)
    {
        handleTelegramMessages();
        lastTimeBotRan = millis();
    }
    delay(100);
}
