
#include <SPI.h>
#include <MFRC522.h>
//#include "melody.h"
#include <Ds1302.h>
//#include <LiquidCrystal.h>
#include "ShiftedLCD.h"

#define MFRC_RST_PIN         9           // Configurable, see typical pin layout above
#define MFRC_SS_PIN          10           // Configurable, see typical pin layout above

#define BEEPER			17//5 //--> a3
#define RED_DIO			5//4 //--> a5
#define GREEN_DIO		4//3 //--> a4(pc5)

#define CLOCK_CLK		16//6 //--> a2
#define CLOCK_DAT		15//7 //--> a1
#define CLOCK_RST		14//8 //--> a0

#define LCD_SIGNAL		8		

#define FOSC 16000000 // Clock Speed
#define BAUD 115200
#define MYUBRR (FOSC/8/BAUD)-1


void send_to_com(const char* data)
{

	int a = 0;
	while (data[a])
	{

		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = data[a++];
	}
}
static uint8_t last_sec = 0;
static uint8_t last_day = 0;
bool statesep = false;
Ds1302 rtc(CLOCK_RST, CLOCK_CLK, CLOCK_DAT);
//LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
LiquidCrystal lcd(LCD_SIGNAL);
MFRC522 mfrc522(MFRC_SS_PIN, MFRC_RST_PIN);
Ds1302::DateTime now;
Ds1302::DateTime last_check_out;


uint32_t get_diff_time(Ds1302::DateTime now, Ds1302::DateTime past)
{
	uint32_t now_sec = now.hour * 3600 + now.minute * 60 + now.second;
	uint32_t past_sec = past.hour * 3600 + past.minute * 60 + past.second;
	return  now_sec - past_sec;
}

byte print_date(char* answ)
{
	rtc.getDateTime(&now);
	if (last_sec != now.second)
	{
		last_sec = now.second;
		if (statesep)
			snprintf(answ, 17, "20%.2d.%.2d.%.2d %.2d:%.2d", now.year, now.month, now.day, now.hour, now.minute);
		else
			snprintf(answ, 17, "20%.2d.%.2d.%.2d %.2d %.2d", now.year, now.month, now.day, now.hour, now.minute);
		statesep = !statesep;
		return 1;
	}
	return 0;
}



char* lcdrow1;
char* lcdrow2;
enum FID : byte
{
	Dryuchkov,
	Antonov,
	Dudarenko,
	Maznuk,
	Habarov,
	Frolova,
	Ivanov,
	Tyutnev,
	Sinyukov,
	LAST
};
byte rfid_dic[LAST][7]
{
{0x04,0x74,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x70,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x6c,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x68,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x64,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x60,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x5c,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x58,0x47,0x32,0x1E,0x70,0x80},
{0x04,0x54,0x47,0x32,0x1E,0x70,0x80}
};
#pragma pack(push, 1)
struct InOutStruct
{
	Ds1302::DateTime time_in;
	Ds1302::DateTime time_out;
	InOutStruct()
	{
		memset(&time_in, 0, sizeof(Ds1302::DateTime));
		memset(&time_out, 0, sizeof(Ds1302::DateTime));
	}
};
#pragma pack(pop)
InOutStruct DayInfo[2][LAST];
void setup()
{
	lcdrow1 = (char*)malloc(17);
	lcdrow2 = (char*)malloc(17);
	memset(lcdrow2, 0, 17);
	memset(lcdrow1, 0, 17);
	memset(&last_check_out, 0, sizeof(Ds1302::DateTime));
	//Serial.begin(115200);
	//while (!Serial);

	////Serial.print(sizeof(InOutStruct));
	//DDRD |= 0x1 << DDD2;
	//DDRD |= 0x1 << DDD3;
	//DDRD |= 0x1 << DDD4;

	//DDRB |= 0x1 << DDB5;

	//UCSR0A = (1 << U2X0);
	//UBRR0H = (unsigned char)(MYUBRR >> 8);
	//UBRR0L = (unsigned char)MYUBRR;
	//UCSR0C = (0 << USBS0) | (3 << UCSZ00);
	//UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	SPI.begin();


	mfrc522.PCD_Init(); // Init MFRC522 card
	rtc.init();
	lcd.begin(16, 2);  // Инициализирует LCD 16x2
	lcd.clear();
	bitSet(DDRC, RED_DIO); //red diod
	bitClear(PORTC, RED_DIO);

	bitSet(DDRC, GREEN_DIO);
	bitClear(PORTC, GREEN_DIO); //green diod

	bitSet(DDRC, BEEPER);
	bitClear(PORTC, BEEPER); //beeper


	pinMode(LCD_SIGNAL, OUTPUT);
	pinMode(MFRC_SS_PIN, OUTPUT);
}




void yield()
{

}
void loop()
{
	//bitClear(PORTB, GREEN_DIO); //подать 0 для работы с ним (8 лсд) (10 рфайди)
	//bitClear(PORTC, GREEN_DIO);
	byte t = print_date(lcdrow1);
	uint32_t time_exp = 0;
	if (last_check_out.hour != 0)
		time_exp = get_diff_time(now, last_check_out);
	if (t != 0)
	{
		lcd.clear();
		lcd.print(lcdrow1);
		bitClear(PORTC, GREEN_DIO);
		bitClear(PORTC, RED_DIO);
		bitClear(PORTC, BEEPER);
	}
	else
		return;
	if (time_exp < 30)
	{
		lcd.setCursor(0, 1);
		lcd.print(lcdrow2);
	}

	if (now.day != last_day > 0)
	{
		last_day = now.day;
		memcpy(DayInfo[1], DayInfo[0], sizeof(DayInfo[0])); //бэкапим прошлый день
	}

	// разнести по времени!
	digitalWrite(MFRC_SS_PIN, LOW);
	digitalWrite(LCD_SIGNAL, HIGH);

	if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
	{
		digitalWrite(MFRC_SS_PIN, HIGH);
		digitalWrite(LCD_SIGNAL, LOW);
		return;
	}
	digitalWrite(MFRC_SS_PIN, HIGH);
	digitalWrite(LCD_SIGNAL, LOW);


	char FIO[16];
	bool fl = false;
	//ищем айди в базе
	FID CurrentUser;
	for (byte i = 0; i < LAST; i++)
	{
		byte counter = 0;
		for (byte j = 0; j < mfrc522.uid.size; j++)
		{
			if (rfid_dic[i][j] != mfrc522.uid.uidByte[j])
			{
				break;
			}
			else
				counter++;
		}
		if (counter == mfrc522.uid.size)
		{
			fl = true;
			CurrentUser = (FID)i;
			switch (CurrentUser)
			{
			case Dryuchkov:
				strcpy(FIO, "Dryuchkov Y.V.");
				break;
			case Antonov:
				strcpy(FIO, "Antonov A.A.");
				break;
			case Dudarenko:
				strcpy(FIO, "Dudarenko S.N.");
				break;
			case Maznuk:
				strcpy(FIO, "Mazynuk E.");
				break;
			case Habarov:
				strcpy(FIO, "Habarov L.");
				break;
			case Frolova:
				strcpy(FIO, "Frolova T.V.");
				break;
			case Ivanov:
				strcpy(FIO, "Ivanov O.V.");
				break;
			case Tyutnev:
				strcpy(FIO, "Tyutnev V.A.");
				break;
			case Sinyukov:
				strcpy(FIO, "Sinyukov A.S.");
				break;
			default:
				fl = false;
				break;
			}
			break;
		}
	}

	if (fl)
	{
		bitSet(PORTC, GREEN_DIO);
		lcd.clear();
		lcd.print(lcdrow1);
		lcd.setCursor(0, 1);           // Установить курсор на вторую строку
		uint32_t tm = 0;
		
		if (DayInfo[0][CurrentUser].time_in.year != 0) //вход был
		{
			tm = get_diff_time(now, DayInfo[0][CurrentUser].time_in);
			if (tm <= 60) //если вход был раньше 60 секунд назад 
			{
				bitSet(PORTC, RED_DIO);
				snprintf(lcdrow2, 17, "TIMEOUT %ds", 60 - tm);
				tone(BEEPER, 1760, 500);
			}
			else
			{
				DayInfo[0][CurrentUser].time_out = now;
				snprintf(lcdrow2, 17, "BYE %s", FIO);
				tone(BEEPER, 2368, 500);
			}
		}
		else
		{
			DayInfo[0][CurrentUser].time_in = now;
			snprintf(lcdrow2, 17, "WELCOME %s", FIO);
			tone(BEEPER, 2368, 500);
		}
		lcd.print(lcdrow2);
		//play_music(BEEPER);
	}
	else
	{
		bitSet(PORTC, RED_DIO);
		lcd.setCursor(0, 1);           // Установить курсор на вторую строку
		snprintf(lcdrow2, 17, "Not A User");
		lcd.print(lcdrow2);    // Вывести текст
		//play_music(BEEPER, 120, 1);
		tone(BEEPER, 200, 500);
	}
	//char asdf[256];
	//Ds1302::DateTime nw = DayInfo[0][CurrentUser].time_in;
	//sprintf(asdf, "Last login is 20%.2d.%.2d.%.2d %.2d:%.2d:%.2d", nw.year, nw.month, nw.day, nw.hour, nw.minute, nw.second);
	//Serial.println(asdf);
	//nw = DayInfo[0][CurrentUser].time_out;
	//sprintf(asdf, "Last logout is 20%.2d.%.2d.%.2d %.2d:%.2d:%.2d", nw.year, nw.month, nw.day, nw.hour, nw.minute, nw.second);
	//Serial.println(asdf);
	last_check_out = now;

}
