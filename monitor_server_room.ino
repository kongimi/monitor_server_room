#include <TridentTD_LineNotify.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "esp_system.h"
#include "DHT.h"
#include <Time.h>

#define DHTTYPE DHT11
byte DHTPIN = 33;  // กำหนดขา DHT11 วัดอุณหภูมิ
byte ACPin = 13; //กำหนดขาที่เชื่อมต่อกับเซ็นเซอร์ AC Power

LiquidCrystal_I2C lcd(0x23, 20, 4);
const String month_name[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
const String day_name[7] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
const int wdtTimeout = 10000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;
//########## CONFIGURATION : MODIFY HERE ##########
char ssid[] = "kongimi-2.4G_plus"; // เครือข่าย Wi-Fi ที่ต้องการเชื่อมต่อ
char pass[] = "xxxx"; // รหัสผ่านเครือข่าย
//char ssid[] = "Kongimi3"; // เครือข่าย Wi-Fi ที่ต้องการเชื่อมต่อ
//char pass[] = "xxxx"; // รหัสผ่านเครือข่าย
//รหัสเชื่อมต่อ Line token
#define LINE_TOKEN "xxxx"
int timezone = 7 * 3600; //ตั้งค่า TimeZone ตามเวลาประเทศไทย
int dst = 0; //กำหนดค่า Date Swing Time

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

DHT dht(DHTPIN, DHTTYPE);


void setup() {
  // put your setup code here, to run once:
  lcd.begin();

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);    

  WiFi.begin(ssid, pass);
  Serial.begin(19200);
  //แสดง "WiFi Connecting" ในคอนโซล
  Serial.print("WiFi Connecting     ");
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi    ");
  //ตรวจเช็คสถานะการเขื่อมต่อวายฟาย
  //ถ้าไม่สำเร็จให้แสดง "." ในคอนโซลจนกว่าจะเขื่อมต่อได้
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  //แสดงสถานะการวายฟายเชื่อมต่อแล้ว
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(WiFi.localIP());
  String myIp = WiFi.localIP().toString().c_str();

  LINE.setToken(LINE_TOKEN);  // กำหนด Line Token
  LINE.notify(myIp);

  dht.begin();
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov"); //ดึงเวลาจาก Server

  pinMode(ACPin, INPUT);
}

void loop() {
  String power_string;
  int Hour, Min, Sec;
  
  // put your main code here, to run repeatedly:
  timerWrite(timer, 0); //reset timer (feed watchdog)
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  Hour = p_tm->tm_hour;
  Min = p_tm->tm_min;
  Sec = p_tm->tm_sec;
  if (Sec == 0 ){ // every minute
    //=================================== temperature sensor =====================================
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
  
    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
  
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C "));
  
    lcd.setCursor(0,3);
    lcd.print("Humi:");
    lcd.print(h);
    lcd.print("Temp:");
    lcd.print(t);

    String txt ="ความชื้น=" + String(h) + "% " + "อุณหภูมิ=" + String(t) + "°C ";
    //************************* End check humidity ***********************

    //************************* check AC Power ***********************
    int ACValue = digitalRead(ACPin);  // AC sensor
    Serial.print("AC : ");
    Serial.println(ACValue);
    lcd.setCursor(0,1); // บรรทัดที่ 1
    lcd.print("AC : ");
    if(ACValue==1){
      txt += "ไฟฟ้าปกติ";
      lcd.print("ON   ");
      lcd.setCursor(10,1); // บรรทัดที่ 1
      lcd.print("          ");
    }else{  // read Battery Level
      lcd.print("OFF  ");
      lcd.setCursor(10,1); // บรรทัดที่ 1
      LINE.notify("ไฟดับ!!!");
    }
    //************************* End check AC Power ***********************

    if( Min == 0 ) {  // every hour
      LINE.notify(txt);
    }
  }
  delay(1000);
}
