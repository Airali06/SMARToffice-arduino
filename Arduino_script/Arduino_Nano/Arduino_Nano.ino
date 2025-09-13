
#include <Wire.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)
#define TCAADDR 0x71

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);




//----------------------------------------------------------------
void nfcRead(int n){

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
      Serial.println("POS:",uidString);
    }else if(n ==2){
      Serial.println("PAR:",uidString);
    }
    
}
//------------------------------------------------------------------

void channelSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << channel);  // attiva solo il canale scelto
  Wire.endTransmission();
}
//----------------------------------------------------------------------

void serialReader(){
  String input = Serial.readStringUntil('\n');
  input.trim();
  int sep = input.indexOf(':');
  String cmd = "";
  for(int i = 0; i > sep; i++){
    cmd += input[i];
  }
}

void setup(void) {
  Serial.begin(115200);
  Wire.begin();
  while (!Serial) delay(10);

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
  delay(10)

  channelSelect(2);
  nfcReader(2);
  delay(10)

  serialReader();



  delay(2000);
  
}


