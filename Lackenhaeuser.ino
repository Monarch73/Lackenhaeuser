/*
    Name:       Lackenhaeuser.ino
    Created:	21.11.2019 18:50:21
    Author:     MONARCH\niels
*/

#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>
#include <DHT_U.h>
#include <DHT.h>
#include <Wire.h> 
#include <RCSwitch.h>

#define CODEHOUSE	"01111"
#define CODEHEATER	"10000"
#define CODEDRYER	"01000"

DHT_Unified g_dht(PORTD2, DHT22);
DHT_Unified g_dht2(PORTD3, DHT22);
uint32_t delayMS = 500;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
RCSwitch rcSwitch_r = RCSwitch();
RCSwitch rcSwitch_s = RCSwitch();
uint8_t onoff=0;
uint8_t switchHeizung = 0;
uint8_t switchTrockner = 0;

sensors_event_t event;

void setupSensor(DHT_Unified *p_dht)
{
	p_dht->begin();
	Serial.println(F("DHTxx Unified Sensor Example"));
	// Print temperature sensor details.
	sensor_t sensor;
	p_dht->temperature().getSensor(&sensor);
	Serial.println(F("------------------------------------"));
	Serial.println(F("Temperature Sensor"));
	Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
	Serial.print(F("Driver Ver:  ")); Serial.println(sensor.version);
	Serial.print(F("Unique ID:   ")); Serial.println(sensor.sensor_id);
	Serial.print(F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
	Serial.print(F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
	Serial.print(F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
	Serial.println(F("------------------------------------"));
	// Print humidity sensor details.
	p_dht->humidity().getSensor(&sensor);
	Serial.println(F("Humidity Sensor"));
	Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
	Serial.print(F("Driver Ver:  ")); Serial.println(sensor.version);
	Serial.print(F("Unique ID:   ")); Serial.println(sensor.sensor_id);
	Serial.print(F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
	Serial.print(F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
	Serial.print(F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
	Serial.println(F("------------------------------------"));
	// Set delay between sensor readings based on sensor details.
	uint32_t ldelay = sensor.min_delay / 1000;
	if (ldelay > delayMS)  delayMS = ldelay;
}

void setup()
{
 Serial.begin(9600);
  // Initialize device.
 setupSensor(&g_dht);
 setupSensor(&g_dht2);
  lcd.init();                      // initialize the lcd 
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();

  //RCSwitch mySwitch = RCSwitch();
  rcSwitch_r.enableReceive(PORTD4);
  rcSwitch_s.enableTransmit(PORTD5);

  rcSwitch_s.switchOn(CODEHOUSE, CODEDRYER);
  rcSwitch_s.switchOn(CODEHOUSE, CODEHEATER);
  delay(5000);
  rcSwitch_s.switchOff(CODEHOUSE, CODEDRYER);
  rcSwitch_s.switchOff(CODEHOUSE, CODEHEATER);

}


void readValue(DHT_Unified *p_dht, int column)
{
	p_dht->temperature().getEvent(&event);
	if (isnan(event.temperature)) {
		Serial.println(F("Error reading temperature!"));
	}
	else {
		Serial.print(F("Temperature: "));
		Serial.print(event.temperature);
		Serial.println(F("°C"));
		lcd.setCursor(column, 1);
		lcd.print((double)event.temperature, 1);
		lcd.print(F(" ")); lcd.print((char)223); lcd.print(F("C   "));

	}
	// Get humidity event and print its value.
	p_dht->humidity().getEvent(&event);
	if (isnan(event.relative_humidity)) {
		Serial.println(F("Error reading humidity!"));
	}
	else {
		Serial.print(F("Humidity: "));
		Serial.print(event.relative_humidity);
		Serial.println(F("%"));
		lcd.setCursor(column, 0);
		lcd.print((double)event.relative_humidity, 1);
		lcd.print(F(" %    "));
	}
}

void loop()
{
 // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  readValue(&g_dht, 0);
  float indoorTemp = isnan(event.temperature) ? 1001.0 : event.temperature;
  float indoorHumidy = isnan(event.relative_humidity) ? 1001.0 : event.relative_humidity;
  readValue(&g_dht2, 8);
  float outdoorTemp = isnan(event.temperature) ? 1001.0 : event.temperature;

  if (indoorHumidy == 1001.0 || indoorTemp == 1001.0)
  {
	  Serial.println("skipping indoor");
	  goto skipIndoor;
  }

  if (switchTrockner == 0)
  {
	  // indoor trockner ist aus
	  if (indoorTemp > 5 && indoorHumidy > 55)
	  {
		  Serial.println("Trockner an");
		  switchTrockner = 1;
		 
		  rcSwitch_s.switchOn(CODEHOUSE, CODEDRYER);
	  }
  }
  else
  {
	  if (indoorTemp < 4 || indoorHumidy < 50)
	  {
		  Serial.println("Trockner aus");
		  rcSwitch_s.switchOff(CODEHOUSE, CODEDRYER);
		  switchTrockner = 0;
	  }
  }

skipIndoor:

  if (outdoorTemp == 1001.0)
  {
	  Serial.println("Skiping Outdoor");
	  switchHeizung = 1;
	  goto skipOutdoor;
  }

  if (switchHeizung == 0)
  {
	  if (outdoorTemp < 1)
	  {
		  Serial.println("Heizung an.");
		  switchHeizung = 1;
		  rcSwitch_s.switchOn(CODEHOUSE, CODEHEATER);
	  }
  }
  else
  {
	  if (outdoorTemp > 3)
	  {
		  Serial.println("Heizung aus");
		  switchHeizung = 0;
		  rcSwitch_s.switchOff(CODEHOUSE, CODEHEATER);
	  }
  }

skipOutdoor:
  onoff++;

  if (onoff > 15)
  {
	  onoff = 0;
	  lcd.setCursor(15, 0);
	  if (switchHeizung != 0)
	  {
		  lcd.print(F("1"));
		  rcSwitch_s.switchOn(CODEHOUSE, CODEHEATER);
	  }
	  else
	  {
		  lcd.print(F("0"));
		  rcSwitch_s.switchOff(CODEHOUSE, CODEHEATER);
	  }

	  lcd.setCursor(15, 1);
	  if (switchTrockner != 0)
	  {
		  lcd.print(F("1"));
		  rcSwitch_s.switchOn(CODEHOUSE, CODEDRYER);
	  }
	  else
	  {
		  lcd.print(F("0"));
		  rcSwitch_s.switchOff(CODEHOUSE, CODEDRYER);
	  }
  }

  delay(1000);
}
