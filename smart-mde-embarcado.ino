/*
  @title (SMART MDE - PROJECT OF EMBEDDED SYSTEM)
  @authors (Caleb A.; Sidney R.; Inácior H.)
  @date (18/09/2021)
*/

//FIREBASE
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>

#define FIREBASE_HOST "smart-mde-default-rtdb.firebaseio.com"   
#define FIREBASE_AUTH "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define WIFI_SSID "(9x^3+5x^2+9)' = ?" 
#define WIFI_PASSWORD "só derivar haha" 

//RTC
#include <Wire.h>
#include <RtcDS3231.h>

RtcDS3231<TwoWire> Rtc(Wire);

//SCT 013
#include "EmonLib.h"

EnergyMonitor sctMonitor; 
float pinSCT = A0;

//LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

//VARIABLES
double meanCurrent = 0.0, mean[5];
int count = 0, voltage = 220;
                                                              
void setup(){
  
  Serial.begin(9600);
  delay(500);

  //set configs
  configLCD();
  configRTC();
  configWIFI();
  configFirebaseAndSCT();

}
 
void loop() { 

  //RTC
  if (!Rtc.IsDateTimeValid()) { //battery is empty
    Serial.println("Battery is Empty!");
  }
  
  RtcDateTime now = Rtc.GetDateTime();  

  Serial.println();
  Serial.print(formatDate(now, "d/m/y", true));
  Serial.print(" - ");
  Serial.print(formatTime(now, "h:m:s", true));
  Serial.println();
  Serial.print("Current: ");
  Serial.print(meanCurrent);
  Serial.print("A");
  Serial.println();
  Serial.print("Potency: ");
  Serial.print(meanCurrent * voltage);
  Serial.print("W");
  Serial.println();
  
  String dateTime = formatDate(now, "d/m/y", false) +" - "+ formatTime(now, "h:m", false);
  String pot = "Pot.: " + String(meanC(now) * voltage) + "W";
  //String pot = "Curr.: " + String(meanC(now)) + "A";
  
  lcd.setCursor(0, 0);
  lcd.print(dateTime);
  
  lcd.setCursor(0, 1);
  lcd.print(pot);
  
  delay(1000);
  lcd.clear();

}

double meanC(RtcDateTime nowDateTime){ 

  meanCurrent = sctMonitor.calcIrms(1480);

  mean[count] = meanCurrent;
  count++;
  
  if(count == 5){
    saveDataFirebase(nowDateTime);//save mean in firabase
    count = 0;
  } 
  return meanCurrent;
}

void saveDataFirebase(RtcDateTime nowDateTime){
      
  int x;
  double meanSave = 0.0, potency = 0.0;
      
  for(x=0;x<5;x++){
    meanSave += mean[x];
  }
  
  meanSave /= 5.0;
  potency = meanSave * voltage;
  
  Serial.println();
  Serial.print("Mean Current: ");
  Serial.print(meanSave);
  Serial.print("A");
  Serial.println();

  Serial.print("Mean Potency: ");
  Serial.print(potency);
  Serial.print("W");
  Serial.println();
  
  String dateTime = formatDate(nowDateTime, "y/m/d", true);
  String path = dateTime.substring(0,8);
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &data = jsonBuffer.createObject();

  data["date"] = dateTime +" "+ formatTime(nowDateTime, "h:m:s", true);
  data["current"] = meanSave;
  data["potency"] = potency;
  
  Firebase.push(path, data);
}

//functions of configuration
void configRTC(){
  
  Rtc.Begin();
  RtcDateTime dateOfCompiled = RtcDateTime(__DATE__,__TIME__); //get date of compilation
  //Rtc.SetDateTime(dataCompilacao);

  if(!Rtc.IsDateTimeValid()){ //checks date time 
    Rtc.SetDateTime(dateOfCompiled); //set time by date of compilation
  }
  
  if (!Rtc.GetIsRunning()){ //active RTC
    Rtc.SetIsRunning(true);
  }
  
  RtcDateTime now = Rtc.GetDateTime();
  
  if (now < dateOfCompiled) { //Checks if a date is consistent
    Rtc.SetDateTime(dateOfCompiled);
  }
  
  // reset
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  Serial.println("Relógio ativado\n");
}


void configLCD(){

  int x;
  
  lcd.begin(16,2);
  Wire.begin(); //start i2c communication

  lcd.init(); //start lcd
  lcd.backlight(); 

  //esthetics
  lcd.setCursor(0, 0);
  lcd.print("Starting");
  lcd.setCursor(0, 1);
  lcd.print("SMART-MDE");

  for(x = 8; x <= 16; x++){
    lcd.setCursor(x, 0);
    lcd.print(".");
    delay(300);
  }
  
  lcd.clear();
}

void configWIFI(){

  String ssid = String(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                               
  
  Serial.print("Connecting WIFI to ");
  Serial.print(WIFI_SSID);

  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print(ssid.substring(0, ssid.length() > 16 ? 16 : ssid.length()));

  int count = 13, x;
  
  while (WiFi.status() != WL_CONNECTED){
    
    if(count % 13 == 3){
      lcd.setCursor(13, 0);
      lcd.print("   ");
      count = 13;
    }
    Serial.print(".");
    
    lcd.setCursor(count, 0);
    lcd.print(".");
    
    delay(500);
    count++;
  }

  lcd.clear();
  
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);

  lcd.setCursor(0, 0);
  lcd.print("Connected to");
  
  lcd.setCursor(0, 1);
  lcd.print(ssid.substring(0, ssid.length() > 16 ? 16 : ssid.length()));
  
  delay(2000);
  
  lcd.clear();
}


void configFirebaseAndSCT(){
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  sctMonitor.current(pinSCT, 111.11);
}

String formatDate(const RtcDateTime& dt, String format, boolean complete) { //função para formatar data
    
  String d = dt.Day() < 10 ? "0" + String(dt.Day()) : String(dt.Day()); 
  String m = dt.Month() < 10 ? "0" + String(dt.Month()) : String(dt.Month());
  String y = String(dt.Year());
  
  format.replace("d",d);
  format.replace("m",m);
  complete ? format.replace("y",y) : format.replace("y",y.substring(2,4));
    
  return format;
}

String formatTime(const RtcDateTime& dt, String format, boolean complete) { //função para formatar hora
  
  String h = dt.Hour() < 10 ? "0" + String(dt.Hour()) : String(dt.Hour());
  String m = dt.Minute() < 10 ? "0" + String(dt.Minute()) : String(dt.Minute());
  String s = dt.Second() < 10 ? "0" + String(dt.Second()) : String(dt.Second());
  
  format.replace("h",h);
  format.replace("m",m);
  if(complete){
    format.replace("s",s);
  }
  
  return format;
}
