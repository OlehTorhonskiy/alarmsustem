//Використані бібліотеки
#include <ESP8266WiFi.h>
#include <WiFiServerSecure.h>
#include <UniversalTelegramBot.h>

//Логін і пароль вашого Wi-Fi, до якого буде підключатися система
#define WIFI_SSID "Kan3965"
#define WIFI_PASSWORD "lalala123"
#define BOT_TOKEN "5616528407:AAFjljnSKtcOWbOGLgcw8ozHqHyUaIZY-Y4"
String chat_id = "5616528407";

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

//===== Налаштування ========================
int time_work_alarm = 10;  //Вказувати в секундах
//===========================================

#define motion_sensor_pin 5  //D1
#define button_on_off_pin 4  //D2
#define led_pin 0            //D3
#define led_alarm_pin 14     //D5
#define zoomer_pin 12        //D6

//Логічні змінні з початковими значеннями (В моєму випатку всі false)
bool button_on_off_flag = false;
bool flag_on_off = false;
bool flag_alarm_on_off = false;
bool flag_led_alarm = false;


unsigned long previous_millis = 0;           //змінна 0мс, яка зберігає значення часу
unsigned long timer_current_millis = 0;      //змінна 0мс, яка зберігає поточне значення часу
unsigned long blink_led_current_millis = 0;  //змінна 0мс, яка зберігає поточне значення часу для контролю лампи
const unsigned long interval = 150;          //стала, яка вказує через який час буде дуже виконуватись дія

const unsigned long BOT_MTBS = 1000;         // затримка для контролю за частотою відправлення
unsigned long bot_lasttime;                  // зміна, яка зберігає в собі час, коли було відправлено -> зніниться, коли буде нове повідомлення

int Sec = 0;
int motion_detected = 0;

void setup() {
  Serial.begin(115200);
  pinMode(motion_sensor_pin, INPUT);
  pinMode(button_on_off_pin, INPUT_PULLUP);  //підтягує внутрішній резистор до кнопки
  pinMode(led_pin, OUTPUT); 
  pinMode(led_alarm_pin, OUTPUT);
  pinMode(zoomer_pin, OUTPUT);

//Допоміжний код дря роботи з Wi-Fi бібліотекою
  configTime(0, 0, "pool.ntp.org");
  secured_client.setTrustAnchors(&cert);
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long previousMillis = 0;
  const long interval = 500;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  bot.sendMessage(chat_id, "ESP8266 WiFi Telegram Test is Started!", "");
}


void loop() {

  Read_btn_on_off();
  Alarm_sound();

  if (flag_on_off) {

    unsigned long current_millis = millis();

    if (current_millis - previous_millis >= interval) {
      previous_millis = current_millis;
      motion_detected = digitalRead(motion_sensor_pin);

      if (motion_detected == HIGH) {
        flag_alarm_on_off = true;
        //Serial.println("Руху виявлено!");
      } else {
        //Serial.println("Руху не виявлено.");
      }
    }
  }
  //Перевірка чи пройшов вказаний раніше час від моменту відправки
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}


void Read_btn_on_off() { //Перевірка натискання кнопки

  bool button_on_off = !digitalRead(button_on_off_pin);

  if (button_on_off == 1 && button_on_off_flag == false) {
    button_on_off_flag = true;
  }
  if (button_on_off == 0 && button_on_off_flag == true) {
    button_on_off_flag = false;
    flag_on_off = !flag_on_off;

    if (flag_on_off) {
      digitalWrite(led_pin, 1);
    } else {
      digitalWrite(led_pin, 0);
      digitalWrite(zoomer_pin, 0);
      digitalWrite(led_alarm_pin, 0);
      flag_alarm_on_off = false;
    }
  }
}

void Alarm_sound() {  //Сигнал динаміка з таймером
  if (flag_alarm_on_off) {
    TimeCount();
    Alarm_blink_led();
    digitalWrite(zoomer_pin, 1);
    if (Sec == time_work_alarm) {
      digitalWrite(zoomer_pin, 0);
      flag_alarm_on_off = false;
      flag_led_alarm = false;
      Sec = 0;
    }
  }
}

void TimeCount() {    //таймер
  if (millis() - timer_current_millis > 1000) {
    timer_current_millis = millis();
    Sec++;
  }
}

void Alarm_blink_led() {  //Мигання світлодіодом 
  if (millis() - blink_led_current_millis > 1000) {
    blink_led_current_millis = millis();
    flag_led_alarm = !flag_led_alarm;
    if (flag_led_alarm) digitalWrite(led_alarm_pin, 1);
    else digitalWrite(led_alarm_pin, 0);
  }
}

void handleNewMessages(int numNewMessages) { //Обробка повідмолень
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);
  String text = bot.messages[1].text;

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (text == "Увімкнути сигналізацію") {
      bot.sendMessage(chat_id, "Сигналізацію було увімкнуто.", "");
      flag_on_off = true;
      Serial.println("ON");
      digitalWrite(led_pin, 1);
    }

    if (text == "Вимкнути сигналізацію") {
      bot.sendMessage(chat_id, "Сигналізацію було вимкнуто.", "");
      flag_on_off = false;
      digitalWrite(led_pin, 0);
      digitalWrite(zoomer_pin, 0);
      digitalWrite(led_alarm_pin, 0);
      flag_alarm_on_off = false;
      Serial.println("OFF");
    }

    if (text == "готово?") {
      bot.sendMessage(chat_id, "Все працює на базі ESP8266", "");
    }

    if (text == "Увімкнути вуличне світло") {
      bot.sendMessage(chat_id, "Вибачте, ця фунція тимчасово не працює", "");
    }

    if (text == "Вимкнути вуличне світло") {
      bot.sendMessage(chat_id, "Вибачте, ця фунція тимчасово не працює", "");
    }
  }
}