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
#define TCAADDR 0x72  //switch I2C per lettori nfc
#define ht16k33 0x70  //multiplex per matrice led
#define schermo 0x27
//--------------------------------------------------

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Adafruit_8x16matrix matrix = Adafruit_8x16matrix();
LiquidCrystal_I2C lcd(schermo, 20, 4);

Servo servoI; //servo ingresso
Servo servoP; //servo parcheggio
int pos_servoI = 1;
int pos_servoP = 1;
const long interval_servoI = 4000;
const long interval_servoP = 20000;
unsigned long timer_servoI = 0;
unsigned long timer_servoP = 0;

unsigned long timer_display = 0;
const long interval_display = 10000;
bool display_in_use = false;

// --- helper: converte uint64_t in String decimale (senza usare sprintf che su AVR può essere limitato)
String uint64ToString(uint64_t v) {
  if (v == 0) return "0";
  char buf[32];
  int i = 0;
  while (v > 0 && i < (int)sizeof(buf)-1) {
    uint8_t digit = v % 10;
    buf[i++] = '0' + digit;
    v /= 10;
  }
  buf[i] = '\0';
  // inverti
  for (int j = 0; j < i/2; j++) {
    char t = buf[j];
    buf[j] = buf[i-1-j];
    buf[i-1-j] = t;
  }
  return String(buf);
}

void nfcReader(int n){
  uint8_t success;
  uint8_t uid[7];      // max lunghezza UID è 7 byte
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength,300);

  if (success) {
    // usa uint64_t per evitare overflow con UID fino a 7 byte
    uint64_t uidValue = 0;
    for (uint8_t i = 0; i < uidLength; i++) {
      uidValue <<= 8;
      uidValue |= uid[i];
    }
    String uidString = uint64ToString(uidValue);

    if (n == 1) {
      Serial.print("POS:");
      Serial.println(uidString);
    } else if (n == 2) {
      Serial.print("PAR:");
      Serial.println(uidString);
    }
  }
}

void channelSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << (channel - 1));  // attiva solo il canale scelto
  Wire.endTransmission();
  delay(5); // piccolo ritardo perché il TCA possa commutare
}

void serialReader(){ //legge la seriale e interpreta il comando
  if (!Serial.available()) return;
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;
  Serial.println("ARDUINO: "+input);

  if(input == "SI"){
    servoI.write(90);
    delay(100);
    pos_servoI = 90;
    timer_servoI = millis();
    return;
  }
  if(input=="SP"){
    servoP.write(90);
    delay(100);
    pos_servoP = 90;
    timer_servoP = millis();
    return;
  }

  if(input.startsWith("M:")){
    input.remove(0, 2); // rimuove "M:"
    aggiornaMatrice(input);
    return;
  }

  if(input.startsWith("D:")){
    input.remove(0, 2); // rimuove "D:"
    LCDdisplay(input);
    display_in_use = true;
    timer_display = millis();
    return;
  }
}

bool rilevaOstacolo(){
  for(int i = 0; i < 15; i++){
    if(digitalRead(sensore) == LOW){
      return true; //se almeno una volta viene trovato un ostacolo ritorna true
    }
    delay(2); // piccolo debounce/tempo tra letture
  }
  return false;
}

void checkServo(){  
  //controlla in modo asincrono le posizioni dei servo, 
  //gestisce i timer e ripristina le posizioni iniziali
  unsigned long currentMillis = millis();

  if(pos_servoI != 1 && currentMillis - timer_servoI >= interval_servoI){
    servoI.write(1);
    pos_servoI = 1;
  }

  if(pos_servoP != 1 && currentMillis - timer_servoP >= interval_servoP){
    if(!rilevaOstacolo()){
      servoP.write(1);
      pos_servoP = 1;
    } else {
      // se ostacolo ancora presente, rimetti timer per ritentare dopo un altro intervallo
      timer_servoP = currentMillis;
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
  matrix.clear();
  // attendiamo coppie di cifre x,y (es: "0314" -> (0,3) e (1,4))
  for(int i = 0; i + 1 < coordinate.length(); i += 2){
    char cx = coordinate.charAt(i);
    char cy = coordinate.charAt(i+1);
    if (isDigit(cx) && isDigit(cy)) {
      int x = cx - '0';
      int y = cy - '0';
      if (x >= 0 && x < 16 && y >= 0 && y < 8) { // bounds per matrice 8x16: x:0..15, y:0..7
        matrix.drawPixel(x, y, LED_ON);
      }
    }
  }
  matrix.writeDisplay();
}

void LCDdisplay(String msg){
  display_in_use = true;
  lcd.clear(); 
  int cont_righe = 0;

  // dividiamo su '/' e stampiamo fino a 4 righe
  while (msg.length() > 0 && cont_righe < 4) {
    int idx = msg.indexOf('/');
    String temp;
    if (idx == -1) {
      temp = msg;
      msg = "";
    } else {
      temp = msg.substring(0, idx);
      msg.remove(0, idx + 1);
    }
    lcd.setCursor(0, cont_righe);
    lcd.print(temp);
    cont_righe++;
  }
}

void resetLCDdisplay(){
  display_in_use = false;
  lcd.clear(); 
  lcd.setCursor(2, 0);
  lcd.print("Benvenuto in");
  lcd.setCursor(2, 1);
  lcd.print("SMART Office");
  lcd.setCursor(0, 3);
  lcd.print("avvicina il badge");
}

void setup(void) {

  Wire.begin();
  Serial.begin(9600);
  
  //while (!Serial) { delay(10); } // opzionale: se la board non ha seriale nativa potrebbe bloccare; se vuoi, togli questa riga
 
  pinMode(sensore, INPUT);

  // Seleziona canale del TCA **dopo** Wire.begin()
  channelSelect(1);

//---DISPLAY-LCD---------------------------------
  lcd.init();                      // initialize the lcd 
  lcd.init();
  lcd.backlight();
    display_in_use = false;
  lcd.clear(); 
  lcd.setCursor(2, 0);
  lcd.print("Benvenuto in");
  lcd.setCursor(2, 1);
  lcd.print("SMART Office");
  lcd.setCursor(0, 3);
  lcd.print("avvicina il badge");
  Serial.println("ciao");
//---MATRICE-LED--------------------------------
  matrix.begin(ht16k33);
  matrix.clear();
  matrix.writeDisplay();

//---SERVO--------------------------------------
Serial.println("ciao");
 servoI.attach(5);
 servoP.attach(6);
  servoI.write(1);
  servoP.write(1); 

//----NFC----------------------------------------

  // assicurati che il canale TCA sia quello giusto (1 o 2) prima di inizializzare il nfc
  channelSelect(1); // lettore 1
  nfc.begin();
  nfc.SAMConfig();
  
    uint32_t versiondata = nfc.getFirmwareVersion();
    Serial.println("ciao");
  if (! versiondata) {
    Serial.println("Didn't find PN53x board (canale 0) - verifica collegamenti e canale TCA");
    // non bloccare eternamente, puoi anche provare canale 2 o uscire
  } else {
    Serial.println("PN532 (canale 0) firmware: 0x");
    Serial.println(versiondata, HEX);
  }

  channelSelect(2); // lettore 2
  nfc.begin();
  nfc.SAMConfig();

  versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.println("Didn't find PN53x board (canale 1) - verifica collegamenti e canale TCA");
    // non bloccare eternamente, puoi anche provare canale 2 o uscire
  } else {
    Serial.print("PN532 (canale 1) firmware: 0x");
    Serial.println(versiondata, HEX);
  }
}

void loop(void) {
  // lettore 1
  
  channelSelect(1);
  nfcReader(1);
  delay(100);
  serialReader();

  // lettore 2
  channelSelect(2);
  nfcReader(2);
  delay(100);

  serialReader();
  delay(50);
  checkLCDdisplay();
  checkServo();
  delay(100);
}
