volatile static int pm25,pm10,thp_p,thp_h;
volatile static float thp_t;
uint32_t sema;
#include "live.h";
#include "zoled.h"
#include <SoftwareSerial.h>
SoftwareSerial Serial1(0, 1); // RX, TX
void read_sensor(const void *argument){
  while(1){
      wdt_reset();
      delay(3000);
      os_semaphore_wait(sema,0xFFFFFFFF);
      thp_t=th_sensor.readTemperature();
      thp_h=th_sensor.readHumidity();
      //thp_p=th_sensor.readPressure();  //reserved for BME280
      unsigned long timeout = millis();
      int count=0;
      byte incomeByte[24],data;
      bool startcount=false;
      //byte data;
      while (1){
        if((millis() -timeout) > 1000) {    
          break;
        }
        if(Serial1.available()){
          data=Serial1.read();
        if(data==0x42 && !startcount){
          startcount = true;
          count++;
          incomeByte[0]=data;
        }else if(startcount){
          count++;
          incomeByte[count-1]=data;
          if(count>=24) {break;}
         }
        }
      }
      unsigned int calcsum=0,exptsum=0; // BM
      for(int i = 0; i < 22; i++) {
        calcsum += (unsigned int)incomeByte[i];
      }
      exptsum = ((unsigned int)incomeByte[22] << 8) + (unsigned int)incomeByte[23];
      if(calcsum == exptsum) {
        pm25 = ((unsigned int)incomeByte[12] << 8) + (unsigned int)incomeByte[13];
        //PM10
        pm10 = ((unsigned int)incomeByte[14] << 8) + (unsigned int)incomeByte[15];
      }
      showLCD();
      os_semaphore_release(sema);
   }
}
void sendMQTT() {
    Serial.println("[MQTT LOOPS]");
    initializeWiFi();
    char payload[300];
    unsigned long epoch = epochSystem + millis() / 1000;
    int year, month, day, hour, minute, second;
    getCurrentTime(epoch, &year, &month, &day, &hour, &minute, &second);
    //os_semaphore_wait(sema, 0xFFFFFFFF);
    if (client.connected() && hastime) {
        sprintf(payload, "|ver_format=3|FAKE_GPS=1|app=PM25|ver_app=%s|device_id=%s|date=%4d-%02d-%02d|time=%02d:%02d:%02d|s_d0=%d|s_d1=%d|s_t0=%d|s_h0=%d|gps_lon=%s|gps_lat=%s",
          "live",clientId,year, month, day,hour, minute, second,pm25,pm10,(int)thp_t,(int)thp_h,gps_lon, gps_lat);
        client.publish(outTopic, payload);
        client.publish("LASS/Test/PM25", payload);
    }
    push_blynk();
}
void setup() {
  Serial.begin(38400);
  Serial1.begin(9600);
  Wire.begin(); //sda scl
  initLCD();
  TH_SENSOR_BEGIN
  sema = os_semaphore_create(1);
  os_thread_create(read_sensor, NULL, OS_PRIORITY_REALTIME, 4096);
  wdt_enable(8000);
  initializeWiFi();
  timer.setInterval(lass_period*1000, sendMQTT);
  timer.setInterval(5000, showVLCD);
}
void loop() {
  timer.run();
  if(status == WL_CONNECTED) Blynk.run();
  if(status == WL_CONNECTED) client.loop();
}
// 你只能寫到 87行不能再多了 ....
//====No Code==== 



