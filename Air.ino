#include <SoftwareSerial.h>
#include <DHT.h>
#include <MHZ19.h>

#define MHZ19B_RX A0
#define MHZ19B_TX A1

#define DHT22_DPIN 4

MHZ19 mhz;
SoftwareSerial swSerial(MHZ19B_RX, MHZ19B_TX); // RX, TX
DHT dht(DHT22_DPIN, DHT22);
// misc
#define INTERVAL_MSEC 5000
unsigned long prev_msec = 0;

struct DataMHZ19B
{
	int	nCO2 = 0;
};

struct DataDHT22
{
	float	fTemp = 0.0;
	float	fHum = 0.0;
};

void setup()
{
	Serial.begin(9600);
	swSerial.begin(9600);

	setupMHZ19B();
	setupDHT22();
}

void setupMHZ19B()
{
	mhz.begin(swSerial);
	//mhz.setRange();
	//mhz.setSpan();
	mhz.autoCalibration();
}


void setupDHT22()
{
	// Init DHT22
	dht.begin();
}

void loop() 
{
	delayAdjusted();

	DataMHZ19B dataMHZ19B;
	processMHZ19B(dataMHZ19B);

	DataDHT22 dataDHT22;
	processDHT22(dataDHT22);

	printTimeStamp();
	printSerial(dataMHZ19B);
	printSerial(dataDHT22);
	Serial.println();

	printLED(dataMHZ19B);
}

void delayAdjusted()
{
	unsigned long msec = millis();
	unsigned long adjust_msec = msec - prev_msec;
	if (adjust_msec > INTERVAL_MSEC)
	{
		adjust_msec -= INTERVAL_MSEC;
	}
	else
	{
		adjust_msec = 0;
	}

	prev_msec = msec;

	delay(INTERVAL_MSEC - adjust_msec);
}

void printTimeStamp()
{
	Serial.print(millis() / 1000);
	Serial.print(" sec, ");
}

void processMHZ19B(struct DataMHZ19B& data)
{
	data.nCO2 = mhz.getCO2();
}

void printSerial(const struct DataMHZ19B& data)
{
	Serial.print("CO2: ");
	Serial.print(data.nCO2);
	Serial.print(" ppm, ");
}

void processDHT22(struct DataDHT22& data)
{
	// Temperature, humidity
	data.fTemp = dht.readTemperature();
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

void printLED(const struct DataMHZ19B& data)
{
//TODO: implement
}