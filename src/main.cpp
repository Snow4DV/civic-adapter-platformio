#include <Arduino.h>
#include <ArduinoJson.h>

//const String fanDirections[] = {"NONE", "UP", "UP_DOWN", "DOWN", "DOWN_WINDSHIELD", "WINDSHIELD"};/
//const String fanLevels[] = {"LEVEL_0", "LEVEL_1", "LEVEL_2", "LEVEL_3", "LEVEL_4", "LEVEL_5", "LEVEL_6", "LEVEL_7"};

#define CLK 2 // Красный #4 провод в фишке климата (DISP CLK)
#define DAT 5 // Жёлтый #5 провод в фишке климата (AC SO)
#define IS_SLAVE false // Заменить false на true для переключения в режим slave (подключение в параллель с Connects 2 или штатным ГУ) 

StaticJsonDocument<128> jsonDoc;

volatile byte climat_data[7];
/* Формат пакеты климата
//volatile byte climat_data[] = {B00000000, B00000001, B10001001, B10000000, B10100001, B10100100, B10001100};
//                                Idle   p       Fanp    DirLFAp        ACp      Tmp2p      Tmp1p   XOR = 1p
//                                  frame                   huu
//                                                          elt
//                                                          alo
//  LSB first  ~(7 bin + parity(odd))                       dR
*/

void (*Read_Climate_Protocol) (uint8_t, uint8_t) = NULL;

void Climat_Read_Slave(uint8_t dataPin, uint8_t clockPin) {
  while (!digitalRead(clockPin)) {}
  delay(10);
  for (uint8_t b = 0; b < 7; b++ ) {
    for (uint8_t i = 0; i < 8; ++i) {
      while (!digitalRead(clockPin)) {}
      while (digitalRead(clockPin)) {}
      switch (digitalRead(dataPin)) {
        case 0:
          climat_data[b] |= 1 << i;
          break;
        case 1:
          climat_data[b] &= ~(1 << i);
          break;
      }
    }
  }
}

void Climat_Read_Master(uint8_t dataPin, uint8_t clockPin)
{
  for (uint8_t b = 0; b < 7; b++)
  {
    for (uint8_t i = 0; i < 8; ++i)
    {
      digitalWrite(clockPin, HIGH);
      delayMicroseconds(50);
      digitalWrite(clockPin, LOW);
      delayMicroseconds(50);
      switch (digitalRead(dataPin))
      {
      case 0:
        climat_data[b] |= 1 << i;
        break;
      case 1:
        climat_data[b] &= ~(1 << i);
        break;
      }
    }
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  
  if (IS_SLAVE) {
    pinMode(CLK, INPUT);
    pinMode(DAT, INPUT);
    Read_Climate_Protocol = &Climat_Read_Slave;
  }
  else {
    pinMode(CLK, OUTPUT);
    digitalWrite(CLK, LOW);
    pinMode(DAT, INPUT_PULLUP);
    Read_Climate_Protocol = &Climat_Read_Master;
  }
  jsonDoc["ac"] = false;
  jsonDoc["auto"] = false;
  jsonDoc["fanDirection"] = 0;
  jsonDoc["fanLevel"] = 0;
  jsonDoc["tempLeft"] = -1;
  jsonDoc["tempRight"] = -1;
  Serial.begin(57600);
}

boolean Check_CRC()
{
  byte CRC = 0;
  if (climat_data[0] != 0)
  {
    jsonDoc["tempLeft"] = 78;
    return 0;
  }
  for (byte i = 1; i < 7; i++)
    CRC ^= climat_data[i];
  if (CRC != B10000001)
  {
    jsonDoc["tempLeft"] = 79;
    return 0;
  }
  return 1;
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  Read_Climate_Protocol(DAT, CLK);

  if (Check_CRC())
  {
    jsonDoc["fanLevel"] = ((climat_data[1]) & B1110) ? ((climat_data[1] & B1110) >> 1): 0;
    jsonDoc["fanDirection"] = (int) ((climat_data[2]) & B1110000 ? ((climat_data[2] & B1110000) >> 4) : 0);
    jsonDoc["auto"] = ((climat_data[2]) & B10) == 2;
    jsonDoc["ac"] = ((climat_data[3]) & B110 ? ((climat_data[3] & B110) >> 1) : 0);
    
    if (char i = (((climat_data[5]) & B11110) >> 1) ) {
    
      if (i == 1) {
        jsonDoc["tempLeft"] = 0;
      }
      if ((i > 1) && (i < 0xF)) {
        jsonDoc["tempLeft"] = i + 14; // 1- LO; 2 - 16; 3 - 17 ..
      }
      if (i == 0xf) {
        jsonDoc["tempLeft"] = 99;
      }
    }
    else {
      jsonDoc["tempLeft"] = -1;
    }

    if (char i = (((climat_data[4]) & B11110) >> 1) ) {
      if (i == 1) {
        jsonDoc["tempRight"] = 0;
      }
      if ((i > 1) && (i < 0xF)) {
        jsonDoc["tempRight"] = i + 14; // 1- LO; 2 - 16; 3 - 17 ..
      }
      if (i == 0xf) {
        jsonDoc["tempRight"] = 99;
      }
    }
    else {
      jsonDoc["tempRight"] = -1;
    }
  }
  String jsonStr;
  serializeJson(jsonDoc, jsonStr);
  
  Serial.println("^" + jsonStr + "$");

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}