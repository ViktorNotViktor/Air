#include <SoftwareSerial.h>
#include <DHT.h>
#include <MHZ19.h>
#include <SparkFun_TLC5940.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr unsigned long INTERVAL_MSEC = 10000;
unsigned long current_msec = 0;
unsigned long current_step = 0;

int blink_zero = 0;
unsigned long prev_zero_msec = 0;
int blink_display_error = 0;
unsigned long prev_display_error_msec = 0;
#define BLINK_INTERVAL 200

#define DHT22_DPIN 4
DHT dht(DHT22_DPIN, DHT22);
#define DHT22_TEMP_CORRECTION -2

#define MHZ19B_RX A0
#define MHZ19B_TX A1
MHZ19 mhz;
SoftwareSerial swSerial(MHZ19B_RX, MHZ19B_TX); // RX, TX

#define LED_LEVEL_MIN 0
#define LED_LEVEL_BLUE_MAX 80
#define LED_LEVEL_GREEN_MAX 80
#define LED_LEVEL_YELLOW_MAX 200
#define LED_LEVEL_RED_MAX 200

#define LED_CHANNEL_START_IDX 0
#define LED_CHANNEL_END_IDX 9
#define LED_CHANNEL_COUNT 10

// CO2 PPM levels
int arrLevel[LED_CHANNEL_COUNT][2] = 
	{ 	// blue, always on
		{0, LED_LEVEL_BLUE_MAX},
		// green
		{500, LED_LEVEL_GREEN_MAX},
		{600, LED_LEVEL_GREEN_MAX},
		{700, LED_LEVEL_GREEN_MAX},
		{800, LED_LEVEL_GREEN_MAX},
		// yellow
		{1000, LED_LEVEL_YELLOW_MAX},
		{1300, LED_LEVEL_YELLOW_MAX},
		{1600, LED_LEVEL_YELLOW_MAX},
		// red
		{2000, LED_LEVEL_RED_MAX},
		{3000, LED_LEVEL_RED_MAX},
	};
// TLC5940 pins
int arrChannel[LED_CHANNEL_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
#define BLINK_ZERO_LED_IDX 0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

//TODO: OLED reset?
#define OLED_RESET     -1 //4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#pragma region type
struct DataMHZ19B
{
	int	nCO2 = 0;
};

struct DataDHT22
{
	float	fTemp = 0.0;
	float	fHum = 0.0;
};
#pragma endregion

#pragma region setup
void setup()
{
	delay(1000);

	Serial.begin(9600);
	swSerial.begin(9600);

	setupLED();
	setupDisplay();
	setupMHZ19B();
	setupDHT22();
}

void setupMHZ19B()
{
	mhz.begin(swSerial);
	//mhz.setRange();
	//mhz.setSpan();
	mhz.autoCalibration(false);
}

void setupDHT22()
{
	// Init DHT22
	dht.begin();
}

void setupLED()
{
	Tlc.init();
}

void setupDisplay()
{
	if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) // Address 0x3C for 128x32
	{
		Serial.println(F("SSD1306 allocation failed"));
		blink_display_error = 1;
	}

	display.clearDisplay();
	display.cp437(true);
	display.display();
}

#pragma endregion

void loop() 
{
	processButtons();
	processBlink();

	if(isIntervalElapsed())
	{
		DataMHZ19B dataMHZ19B;
		processMHZ19B(dataMHZ19B);

		DataDHT22 dataDHT22;
		processDHT22(dataDHT22);

		printTimeStamp();
		printSerial(dataMHZ19B);
		printSerial(dataDHT22);
		Serial.println();

		printLED(dataMHZ19B);
		printDisplay(dataMHZ19B, dataDHT22);
	}
}

bool isIntervalElapsed()
{
	//TODO: overflow control
	current_msec = millis();
	bool bResult = (current_msec >= current_step * INTERVAL_MSEC);
	if(bResult)
	{
		++current_step;
	}
	return bResult;
}

void processButtons()
{
	//TODO: if "Calibrate" button pressed
	//mhz.calibrateZero();
}

void processBlink()
{
	if(blink_zero != 0 && (current_msec - prev_zero_msec) > BLINK_INTERVAL)
	{
		Tlc.set(arrChannel[BLINK_ZERO_LED_IDX], blink_zero == 1 ? arrLevel[BLINK_ZERO_LED_IDX][1] : LED_LEVEL_MIN);
		Tlc.update();

		prev_zero_msec = current_msec;
		blink_zero *= -1;
	}
	//TODO: check prev state of blink_zero to turn off LED
}

#pragma region time

void printTimeStamp()
{
	Serial.print(millis() / 1000);
	Serial.print(" sec, ");
}
#pragma endregion

#pragma region MHZ19B
void processMHZ19B(struct DataMHZ19B& data)
{
	data.nCO2 = mhz.getCO2();
	blink_zero = (data.nCO2 == 0) ? -1 : 0; 
}

void printSerial(const struct DataMHZ19B& data)
{
	Serial.print("CO2: ");
	Serial.print(data.nCO2);
	Serial.print(" ppm, ");
}
#pragma endregion

#pragma region DHT22
void processDHT22(struct DataDHT22& data)
{
	// Temperature, humidity
	data.fTemp = dht.readTemperature() + DHT22_TEMP_CORRECTION; // :D
	data.fHum = dht.readHumidity();
}

void printSerial(const struct DataDHT22& data)
{
	Serial.print("Temp: ");
	Serial.print(data.fTemp);
	Serial.print(" C, RH: ");
	Serial.print(data.fHum);
	Serial.print("%");
}
#pragma endregion

#pragma region LED
void printLED(const struct DataMHZ19B& data)
{
	for(int i = LED_CHANNEL_START_IDX; i <= LED_CHANNEL_END_IDX; ++i)
	{
		if(arrLevel[i][0] < data.nCO2)
		{
			Tlc.set(arrChannel[i], arrLevel[i][1]);
		}
		else
		{
			Tlc.set(arrChannel[i], LED_LEVEL_MIN);
		}
	}
	Tlc.update();
}
#pragma endregion

#pragma region display
void printDisplay(const struct DataMHZ19B& dataMHZ19B, const struct DataDHT22& dataDHT22)
{
	display.clearDisplay();

	display.setTextSize(2);
	display.setCursor(0, 0);
	display.setTextColor(WHITE);
	display.print(dataMHZ19B.nCO2);
	display.print(" ");

	int x = display.getCursorX();
	int y = display.getCursorY();
	display.setTextSize(1);
	display.print("CO2");
	display.setCursor(x, y + 8);
	display.println("PPM\n");

	display.print(dataDHT22.fTemp, 0);
	display.print("C ");
	display.print(dataDHT22.fHum, 0);
	display.print("% ");

	unsigned long lSeconds = (current_msec / 1000) % 60;
	unsigned long lMinutes = (current_msec / 1000 / 60) % 60;
	unsigned long lHours = current_msec / 1000 / 60 / 60;
	
	/*Serial.print("WTF");
	Serial.print(" current_msec=");
	Serial.print(current_msec);
	Serial.print(" /=");
	Serial.print(current_msec / 1000 / 60);
	Serial.print(" %=");
	Serial.println((current_msec / 1000 / 60) % 60);
*/
	constexpr int nBufferSize = 16;
	char szBuffer[nBufferSize];
	snprintf(szBuffer, nBufferSize, "%02lu:%02lu:%02lu", lHours, lMinutes, lSeconds);
	display.print(szBuffer);

	display.display();
}
#pragma endregion

/*
Basic Pin setup:
------------                       ---u----
ARDUINO
13|-> SCLK (pin 25)          OUT1 |1     28| OUT channel 0
12|                          OUT2 |2     27|-> GND (VPRG)
11|-> SIN (pin 26)           OUT3 |3     26|-> SIN (pin 11)
10|-> BLANK (pin 23)         OUT4 |4     25|-> SCLK (pin 13)
9|-> XLAT (pin 24)             .  |5     24|-> XLAT (pin 9)
8|                             .  |6     23|-> BLANK (pin 10)
7|                             .  |7     22|-> GND
6|                             .  |8     21|-> VCC (+5V)
5|                             .  |9     20|-> 2K Resistor -> GND
4|                             .  |10    19|-> +5V (DCPRG)
3|-> GSCLK (pin 18)            .  |11    18|-> GSCLK (pin 3)
2|                             .  |12    17|-> SOUT
1|                             .  |13    16|-> XERR
0|                           OUT14|14    15| OUT channel 15
------------                                  --------

-  Put the longer leg (anode) of the LEDs in the +5V and the shorter leg
(cathode) in OUT(0-15).
-  +5V from Arduino -> TLC pin 21 and 19     (VCC and DCPRG)
-  GND from Arduino -> TLC pin 22 and 27     (GND and VPRG)
-  digital 3        -> TLC pin 18            (GSCLK)
-  digital 9        -> TLC pin 24            (XLAT)
-  digital 10       -> TLC pin 23            (BLANK)
-  digital 11       -> TLC pin 26            (SIN)
-  digital 13       -> TLC pin 25            (SCLK)
-  The 2K resistor between TLC pin 20 and GND will let ~20mA through each
LED.  To be precise, it's I = 39.06 / R (in ohms).  This doesn't depend
on the LED driving voltage.
- (Optional): put a pull-up resistor (~10k) between +5V and BLANK so that
all the LEDs will turn off when the Arduino is reset.

If you are daisy-chaining more than one TLC, connect the SOUT of the first
TLC to the SIN of the next.  All the other pins should just be connected
together:
BLANK on Arduino -> BLANK of TLC1 -> BLANK of TLC2 -> ...
XLAT on Arduino  -> XLAT of TLC1  -> XLAT of TLC2  -> ...
The one exception is that each TLC needs it's own resistor between pin 20
and GND.

This library uses the PWM output ability of digital pins 3, 9, 10, and 11.
Do not use analogWrite(...) on these pins.

This sketch does the Knight Rider strobe across a line of LEDs.

Alex Leone <acleone ~AT~ gmail.com>, 2009-02-03 */