#include <Arduino.h>
#include <ArduinoJson.h>

#define CLK 2		   // Красный #4 провод в фишке климата (DISP CLK)
#define DAT 5		   // Жёлтый #5 провод в фишке климата (AC SO)
#define IS_SLAVE false // Заменить false на true для переключения в режим slave (подключение в параллель с Connects 2 или штатным ГУ)

#define EQUAL_COUNT 2 // Количество одинаковых состояний климата, которые должны быть получены для отправки нового состояния (для фильтрции). Рекомендуемый диапазон: 1-5 (1 - для отключения фильтрации)

volatile byte climateData[7];
/* Формат пакеты климата
//volatile byte climateData[] = {B00000000, B00000001, B10001001, B10000000, B10100001, B10100100, B10001100};
//                                Idle   p       Fanp    DirLFAp        ACp      Tmp2p      Tmp1p   XOR = 1p
//                                  frame                   huu
//                                                          elt
//                                                          alo
//  LSB first  ~(7 bin + parity(odd))                       dR
*/

StaticJsonDocument<128> *prevJsonDoc = NULL;
uint8_t equalityCheck = 0; // Переменная хранит количество одинаковых последовательно полученных состояний климата. Используется для фильтрации помех.

void (*climateReadProtocol)(uint8_t, uint8_t) = NULL;

void climateReadSlave(uint8_t dataPin, uint8_t clockPin)
{
	while (!digitalRead(clockPin))
	{
	}
	delay(10);
	for (uint8_t b = 0; b < 7; b++)
	{
		for (uint8_t i = 0; i < 8; ++i)
		{
			while (!digitalRead(clockPin))
			{
			}
			while (digitalRead(clockPin))
			{
			}
			switch (digitalRead(dataPin))
			{
			case 0:
				climateData[b] |= 1 << i;
				break;
			case 1:
				climateData[b] &= ~(1 << i);
				break;
			}
		}
	}
}


void climateReadMaster(uint8_t dataPin, uint8_t clockPin)
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
				climateData[b] |= 1 << i;
				break;
			case 1:
				climateData[b] &= ~(1 << i);
				break;
			}
		}
	}
}

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);

	if (IS_SLAVE)
	{
		pinMode(CLK, INPUT);
		pinMode(DAT, INPUT);
		climateReadProtocol = &climateReadSlave;
	}
	else
	{
		pinMode(CLK, OUTPUT);
		digitalWrite(CLK, LOW);
		pinMode(DAT, INPUT_PULLUP);
		climateReadProtocol = &climateReadMaster;
	}

	Serial.begin(57600);
}

bool Check_CRC()
{
	uint8_t crc = 0;
	if (climateData[0] != 0)
	{
		
		return false;
	}
	for (byte i = 1; i < 7; i++)
		crc ^= climateData[i];
	if (crc != B10000001)
	{
		return false;
	}
	return true;
}

bool compareJsonDocs(StaticJsonDocument<128>* doc1, StaticJsonDocument<128>* doc2) {
	bool result = doc1 != NULL && doc2 != NULL && (*doc1)["tempLeft"] == (*doc2)["tempLeft"] && (*doc1)["ac"] == (*doc2)["ac"] && (*doc1)["auto"] == (*doc2)["auto"]
	 && (*doc1)["fanDirection"] == (*doc2)["fanDirection"] && (*doc1)["fanLevel"] == (*doc2)["fanLevel"] && (*doc1)["tempRight"] == (*doc2)["tempRight"];
	return result;
}

void loop()
{
	digitalWrite(LED_BUILTIN, HIGH);
	climateReadProtocol(DAT, CLK);


	StaticJsonDocument<128> *jsonDoc = new StaticJsonDocument<128>();
	if (Check_CRC())
	{
		(*jsonDoc)["fanLevel"] = ((climateData[1]) & B1110) ? ((climateData[1] & B1110) >> 1) : 0;
		(*jsonDoc)["fanDirection"] = (int)((climateData[2]) & B1110000 ? ((climateData[2] & B1110000) >> 4) : 0);
		(*jsonDoc)["auto"] = ((climateData[2]) & B10) == 2;
		(*jsonDoc)["ac"] = ((climateData[3]) & B110 ? ((climateData[3] & B110) >> 1) : 0);

		if (char i = (((climateData[5]) & B11110) >> 1))
		{

			if (i == 1)
			{
				(*jsonDoc)["tempLeft"] = 0;
			}
			if ((i > 1) && (i < 0xF))
			{
				(*jsonDoc)["tempLeft"] = i + 14; // 1- LO; 2 - 16; 3 - 17 ..
			}
			if (i == 0xf)
			{
				(*jsonDoc)["tempLeft"] = 99;
			}
		}
		else
		{
			(*jsonDoc)["tempLeft"] = -1;
		}

		if (char i = (((climateData[4]) & B11110) >> 1))
		{
			if (i == 1)
			{
				(*jsonDoc)["tempRight"] = 0;
			}
			if ((i > 1) && (i < 0xF))
			{
				(*jsonDoc)["tempRight"] = i + 14; // 1- LO; 2 - 16; 3 - 17 ..
			}
			if (i == 0xf)
			{
				(*jsonDoc)["tempRight"] = 99;
			}
		}
		else
		{
			(*jsonDoc)["tempRight"] = -1;
		}
	} else {
		(*jsonDoc)["tempLeft"] = -1;
		(*jsonDoc)["ac"] = 0;
		(*jsonDoc)["auto"] = false;
		(*jsonDoc)["fanDirection"] = 0;
		(*jsonDoc)["fanLevel"] = 0;
		(*jsonDoc)["tempRight"] = -1;
	}


	if(compareJsonDocs(jsonDoc, prevJsonDoc)) {
		if (equalityCheck < EQUAL_COUNT) {
			equalityCheck++;
		} 
		delete jsonDoc;
	} else {
		if(prevJsonDoc != NULL) delete prevJsonDoc;
		prevJsonDoc = jsonDoc;
		equalityCheck = 1;
	}


	if(equalityCheck >= EQUAL_COUNT) {
		String jsonStr;
		serializeJson(*prevJsonDoc, jsonStr);
		Serial.println("^" + jsonStr + "$");
	}

	
	digitalWrite(LED_BUILTIN, LOW);
	//delay(100 / EQUAL_COUNT);
	delay(10);
}

