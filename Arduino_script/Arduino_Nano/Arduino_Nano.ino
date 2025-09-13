
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Servo.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include <LiquidCrystal_I2C.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)
#define sensore 7

//--- indirizzi I2C---------------------------------
#define TCAADDR 0x71  //switch I2C per lettori nfc
#define ht16k33 0x70  //multiplex per matrice led
#define schermo 0x27
//--------------------------------------------------

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Adafruit_8x16matrix matrix = Adafruit_8x16matrix();
LiquidCrystal_I2C lcd(schermo, 20, 4);

Servo servoI; //servo ingresso
Servo servoP; //servo parcheggio
int pos_servoI = 0;
int pos_servoP = 0;
const long interval_servoI = 4000;
const long interval_servoP = 20000;
unsigned long timer_servoI = 0;
unsigned long timer_servoP = 0;

unsigned long timer_display = 0;
const long interval_display = 10000;
bool display_in_use = false;

void nfcReader(int n){

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    String uidString = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      uidString += String(uid[i]);  
    }
    if(n == 1){
      Serial.println("POS:"+uidString);
    }else if(n ==2){
      Serial.println("PAR:"+uidString);
    }
  }
    
}


void channelSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << channel);  // attiva solo il canale scelto
  Wire.endTransmission();
}


void serialReader(){ //legge la seriale e interpreta il comando
  String input = Serial.readStringUntil('\n');
  input.trim();
  if(input[0]+input[1] == "SI"){
    servoI.write(90);
    pos_servoI = 90;
    timer_servoI = millis();
    return;
  }
  if(input[0]+input[1] == "SP"){
      servoP.write(90);
    pos_servoP = 90;
    timer_servoP = millis();
    return;
  }

  if(input[0]+input[1] == "M"){
    input.remove(0, 1);
    aggiornaMatrice(input);
    return;
  }

  if(input[0]+input[1] == "D:"){
    input.remove(0, 1);
    LCDdisplay(input);
    display_in_use = true;
    timer_display = millis();
  }

}


bool rilevaOstacolo(){
  for(int i = 0; i < 15; i++){
    if(digitalRead(sensore) == LOW){
      return true; //se almeno una volta viene trovato un ostacolo ritorna true
    }
  }
  return false;
}

void checkServo(){  
  //controlla in modo asincrono le posizioni dei servo, 
  //gestisce i timer e ripristina le posizioni iniziali
  unsigned long currentMillis = millis();

  if(pos_servoI =! 0 && currentMillis - timer_servoI >= interval_servoI){
    servoI.write(0);
    pos_servoI = 0;
  }

  currentMillis = millis();
   if(pos_servoP =! 0 && currentMillis - timer_servoP >= interval_servoP){
      if(!rilevaOstacolo()){
            servoP.write(0);
            pos_servoP = 0;
      }
  }

}

void checkLCDdisplay(){
  unsigned long currentMillis = millis();
  if(display_in_use && currentMillis - timer_display >= interval_display){
    resetLCDdisplay();
    display_in_use = false;
  }
}

void aggiornaMatrice(String coordinate){
  int x = 0;
  int y = 0;
  matrix.clear();
   for(int i = 0; i <coordinate.length(); i+=2){
    x =  (coordinate.charAt(i) - '0')*1;
    y =  (coordinate.charAt(i) - '0')*1;
      matrix.drawPixel(x, y, LED_ON);
   }
}


void LCDdisplay(String msg){
  display_in_use = true;
  lcd.clear(); 
  int index = 0;
  String temp = "";
  int cont_righe = 0;

  while(index > -1){

    if(cont_righe > 3){
      return;
    }

    int index = msg.indexOf('/');
    if(index == -1){
      return;
    }

    temp = msg.substring(0, index);
    msg.remove(0, index + 1);

    lcd.setCursor(0, cont_righe);
    cont_righe ++;
  }
  
}

void resetLCDdisplay(){
  lcd.clear(); 
  lcd.setCursor(2, 0);
  lcd.print("Benvenuto in");
  lcd.setCursor(2, 1);
  lcd.print("SMART Office");
  lcd.setCursor(0, 3);
  lcd.print("avvicina il badge");
}


void setup(void) {

  Serial.begin(9600);
  Wire.begin();
  while (!Serial) delay(10);
  pinMode(sensore, INPUT);
//---DISPLAY-LCD---------------------------------
  lcd.init();
  lcd.backlight();
  lcd.clear(); 
  lcd.setCursor(2, 0);
  lcd.print("Benvenuto in");
  lcd.setCursor(2, 1);
  lcd.print("SMART Office");
  lcd.setCursor(0, 3);
  lcd.print("avvicina il badge");
//---MATRICE-LED--------------------------------
  matrix.begin(ht16k33);
  matrix.clear();

//---SERVO--------------------------------------
  servoI.attach(9);
  servoP.attach(10);
  servoI.write(0);
  servoP.write(0); 


//----NFC----------------------------------------
  nfc.begin();
  nfc.SAMConfig();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); 
  }



}

void loop(void) {
  

  channelSelect(1);
  nfcReader(1);
  delay(30);

  channelSelect(2);
  nfcReader(2);
  delay(30);

  serialReader();

  checkServo();
  delay(30);
  
}


