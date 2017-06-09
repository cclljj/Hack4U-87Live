#ifndef LIVETOOL
#define LIVETOOL

char ssid[] = "LASS";       // 你的WiFi網路名稱  例如是 Hack4u  就打 "Hack4u"
char pass[] = "LASS123456";     // 你的網路密碼 沒有密碼不要找我
char gps_lat[] = "25.0413";   // 你在Google Map查到的緯度(預設地址)
char gps_lon[] = "121.6144";  // 你在Google Map查到的經度(預設地址)
const int lass_period = 60;      // 幾秒想傳一次資料 建議實際上線是 60
char blynk_auth[] = "c2a0129372b74234ac8a358409a03425"; //你的 Blynk Token 密碼 

#define LANG 1
/*
 * LANG 語言設定
 * 1: 中文
 * 2: English
 */
#define TH_SENSOR 2
/*
 * TH_SENSOR 溫濕度感測器設定
 * 1: thp
 * 2: SHT31
 */

//對 相信你的眼睛 下面沒有了

/*
授權聲明

套件名稱 ： Hack4U 87Live Zero Diy kits(OLED講座推廣版本)

本套件是依據 219 反空污大遊行現場 Hacking 的Code 延伸而成

原創者：肉多多/阿海        (219 遊行參與)
       中研院/荊輔翔      (219 遊行參與)
       協明工業 Joe Lai  ( 桃園市青創會)

姓名標示-非商業性-相同方式分享 3.0 台灣 (CC BY-NC-SA 3.0 TW)

你可自由：

分享 — 以任何媒介或格式重製及散布本素材
修改 — 重混、轉換本素材、及依本素材建立新素材
只要你遵守授權條款規定，授權人不能撤回你使用本素材的自由。
惟需遵照下列條件:

姓名標示 — 你必須給予 適當表彰、提供指向本授權條款的連結，以及 指出（本作品的原始版本）是否已被變更。你可以任何合理方式為前述表彰，但不得以任何方式暗示授權人為你或你的使用方式背書。
非商業性 — 你不得將本素材進行商業目的之使用。
相同方式分享 — 若你重混、轉換本素材，或依本素材建立新素材，你必須依本素材的授權條款來散布你的貢獻物。
不得增加額外限制 — 你不能增設法律條款或科技措施，來限制別人依授權條款本已許可的作為。
聲明

當你使用本素材中屬於公眾領域的元素，或當法律有例外或限制條款允許你的使用，則你不需要遵守本授權條款。
未提供保證。本授權條款未必能完全提供，你預期用途所需要的所有許可。例如：形象權，隱私權、著作人格權等其他權利，可能限制你如何使用本素材。
*/


































//對 你猜對了  怎麼可能  只有87行  能超越 86  XDDDD
//==============以下你都不要去管他 你都用不到 //
char server[] = "gpssensor.ddns.net"; // the MQTT server of LASS
char clientId[32] = "";
char outTopic[32] = "";
bool hastime = false;
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#if TH_SENSOR==1
  #include <Adafruit_thp.h>
  Adafruit_thp th_sensor;// I2C
  #define TH_SENSOR_BEGIN th_sensor.begin();
#elif TH_SENSOR==2
  #include <Adafruit_SHT31.h>
  Adafruit_SHT31 th_sensor = Adafruit_SHT31();
  #define TH_SENSOR_BEGIN th_sensor.begin(0x45);    // or 0x44, 
#endif
#include "wiring_watchdog.h"
#include <PubSubClient.h>
#include <WiFiUdp.h>
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <BlynkSimpleWifi.h>
#include <SimpleTimer.h>
WiFiClient wifiClient;
PubSubClient client(wifiClient);


int status = WL_IDLE_STATUS;
WiFiUDP Udp;
const char ntpServer[] = "tw.pool.ntp.org";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
const byte nptSendPacket[ NTP_PACKET_SIZE] = {
  0xE3, 0x00, 0x06, 0xEC, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x31, 0x4E, 0x31, 0x34,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
byte ntpRecvBuffer[ NTP_PACKET_SIZE ];

#define LEAP_YEAR(Y)     ( ((Y)>0) && !((Y)%4) && ( ((Y)%100) || !((Y)%400) ) )
static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0
uint32_t epochSystem = 0; // timestamp of system boot up

// send an NTP request to the time server at the given address
void retrieveNtpTime() {
  int try_cnt = 0;
  do{
    if(!hastime){
        Udp.begin(2390);
        Serial.println("Send NTP packet");
        Udp.beginPacket(ntpServer, 123); //NTP requests are to port 123
        Udp.write(nptSendPacket, NTP_PACKET_SIZE);
        Udp.endPacket();
        if(Udp.parsePacket()) {
          hastime = true;
          Serial.println("NTP packet received");
          Udp.read(ntpRecvBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
          
          unsigned long highWord = word(ntpRecvBuffer[40], ntpRecvBuffer[41]);
          unsigned long lowWord = word(ntpRecvBuffer[42], ntpRecvBuffer[43]);
          unsigned long secsSince1900 = highWord << 16 | lowWord;
          const unsigned long seventyYears = 2208988800UL;
          unsigned long epoch = secsSince1900 - seventyYears;
      
          epochSystem = epoch - millis() / 1000;
        }else{
          delay(3000);
        }
        try_cnt ++;
      }
      else break;
  }while(try_cnt < 3);
}

void getCurrentTime(unsigned long epoch, int *year, int *month, int *day, int *hour, int *minute, int *second) {
  int tempDay = 0;
  *hour = (epoch  % 86400L) / 3600;
  *minute = (epoch  % 3600) / 60;
  *second = epoch % 60;

  *year = 1970; // epoch starts from 1970
  *month = 0;
  *day = epoch / 86400;

  for (*year = 1970; ; (*year)++) {
    tempDay += (LEAP_YEAR(*year) ? 366 : 365);
    if (tempDay > *day) {
      tempDay -= (LEAP_YEAR(*year) ? 366 : 365);
      break;
    }
  }
  tempDay = *day - tempDay; // the days left in a year
  for ((*month) = 0; (*month) < 12; (*month)++) {
    if ((*month) == 1) {
      tempDay -= (LEAP_YEAR(*year) ? 29 : 28);
      if (tempDay < 0) {
        tempDay += (LEAP_YEAR(*year) ? 29 : 28);
        break;
      }
    } else {
      tempDay -= monthDays[(*month)];
      if (tempDay < 0) {
        tempDay += monthDays[(*month)];
        break;
      }
    }
  }
  *day = tempDay+1; // one for base 1, one for current day
  (*month)++;
}
void initializeMQTT();
void initializeWiFi() {
  if (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    Blynk.begin(blynk_auth,ssid,pass);
    delay(5000);
    status = WiFi.status();
    if(status == WL_CONNECTED){
      byte mac[6];
      WiFi.macAddress(mac);
      memset(clientId, 0, sizeof(clientId));
      sprintf(clientId, "FT_LIVE%02X%02X%02X%02X",mac[2], mac[3], mac[4], mac[5]);
      sprintf(outTopic, "LASS/Test/PM25/live");
      Serial.println(clientId);
    }
  }

  if(status == WL_CONNECTED) retrieveNtpTime();
 
  if ((!client.connected()) && status == WL_CONNECTED) {
    initializeMQTT();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId)) {
      Serial.println("connected");
      //break;
    }
  }
  status = WiFi.status();
}

void callback(char* topic, byte* payload, unsigned int length) {

}

void initializeMQTT() {
  client.setServer(server, 1883);
  //client.setCallback(callback);
}


SimpleTimer timer;

WidgetLCD lcd(V6);
static int v_page=0;
static bool v_trackGPS = 0;
void showVLCD(){
  if(status == WL_CONNECTED){
    lcd.clear();
    if(v_page == 0){
      #if LANG==1
        lcd.print(0, 0, String("[GPS]追蹤關閉"));
      #elif LANG==2
        lcd.print(0, 0, String("[GPS] Tracking Off"));
      #endif
    }else if(v_page == 1){
      lcd.print(0, 0, String("PM 10:")+String(pm10) + " μg/m³");
      delay(10);
      lcd.print(0, 1, String("PM2.5:")+String(pm25) + " μg/m³");
      delay(10);
    }else if(v_page == 2){
      #if LANG==1
        lcd.print(0, 0, String("溫度:")+String(thp_t,1)+"°C");
        delay(10);
        lcd.print(0, 1, String("濕度:")+(int)(thp_h)+"% 氣壓:"+(int)(thp_p/100)+"帕");
        delay(10);
      #elif LANG==2
        lcd.print(0, 0, String("Temp:")+String(thp_t,1)+"°C");
        delay(10);
        lcd.print(0, 1, String("RH:")+(int)(thp_h)+"% Pressure:"+(int)(thp_p/100)+"帕");
        delay(10);
      #endif
    }else if(v_page == 3){
      #if LANG==1
        lcd.print(0, 0, String("裝置代號"));
        delay(10);
        lcd.print(0, 1, String(clientId));
        delay(10);
      #elif LANG==2
        lcd.print(0, 0, String("Device ID"));
        delay(10);
        lcd.print(0, 1, String(clientId));
        delay(10);
      #endif
    }else if(v_page == 4){
      uint32_t v_second = millis()/1000;
      uint32_t v_minute = v_second/60;
      uint32_t v_hour = v_minute/60;
      uint32_t v_day  = v_hour/24;
      #if LANG==1
        lcd.print(0, 0, String("Live上線時間" ));
        lcd.print(0, 1, String(v_day) + "天" + String(v_hour%24) + "時" + String(v_minute%60) + "分" + String(v_second%60) + "秒");
      #elif LANG==2
        lcd.print(0, 0, String("Live online time" ));
        lcd.print(0, 1, String(v_day) + "D " + String(v_hour%24) + "H " + String(v_minute%60) + "M " + String(v_second%60) + "S");
      #endif    
      delay(10);
    }else if(v_page == 5){
      #if LANG==1
        lcd.print(0, 0, String("[GPS]追蹤已啟動!!!"));
      #elif LANG==2
        lcd.print(0, 0, String("[GPS] Tracking On"));
      #endif 
      
    }
  }
}

void push_blynk(){
  Blynk.virtualWrite(V1, pm25);
  delay(10);
  Blynk.virtualWrite(V2, pm10);
  delay(10);
  /* 4/29 因為有的人預設Blynk Energy點數不足，所以這邊變成示範的 Code 隨你的點數能力參考應用 -- 阿海
  Blynk.virtualWrite(V3, (int)thp_t);
  delay(10);
  Blynk.virtualWrite(V4, (int)thp_h);
  delay(10);
  Blynk.virtualWrite(V5, (int)(thp_p/100));
  delay(10);
  Blynk.virtualWrite(V10, (int)(millis()/1000));
  delay(10);
  */
}

BLYNK_CONNECTED() {
  Blynk.virtualWrite(V8, (int)0); //預設關閉 GPS追蹤 ....
}

BLYNK_WRITE(V7) {
  if(v_trackGPS){
    GpsParam gps(param);
    String gps_str = String(gps.getLat(),6);
    memset(gps_lat,0,sizeof(gps_lat));
    gps_str.toCharArray(gps_lat,sizeof(gps_lat));
    delay(10);
    gps_str = String(gps.getLon(),6);
    memset(gps_lon,0,sizeof(gps_lon));
    gps_str.toCharArray(gps_lon,sizeof(gps_lon));
    Serial.println(String("[Blynk GPS] lat:") + gps_lat + " lon:" + gps_lon);
    delay(10);
  }
}

BLYNK_WRITE(V8) {
  v_page = param.asInt();
  if(v_page == 0) {
    v_trackGPS = 0;
    Serial.println("[GPS DISABLED]");
  }else if(v_page == 5){
    v_trackGPS = 1;
    Serial.println("[GPS ENABLED]");
  }
  showVLCD();
}

/*
BLYNK_WRITE(V9) {
  v_trackGPS = param.asInt();
  if(v_trackGPS) Serial.println("[GPS ENABLED]");
  else Serial.println("[GPS DISABLED]");
}
*/

#endif
