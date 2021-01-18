#include <Arduino.h>
#include "device-config.h"
 
// // BME related header files
// #include <Wire.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME280.h>
#include "bmx280_i2c.h"
#include "pm2008m.h"

// increment on change
#define SOFTWARE_VERSION_STR "NRZ-2020-133"
String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);

/**********************************************/
/*                  WiFi declarations         */
/**********************************************/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* host = "api.luftdaten.info";
const int httpPort = 80;
int value = 0;


unsigned long starttime;
unsigned long sampletime_ms = 10000;


// bme sensor setup
//Adafruit_BME280 bme; // I2C default pins (SCL=IO22, SDA=IO21 on ESP32)
BMX280 bmx280;

PM2008M pm2008m;

/**********************************************/
/* WiFi connecting script                     */
/**********************************************/
void connectWifi() {
  WiFi.begin(ssid, password); // Start WiFI
  
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
}


/*****************************************************************
 * send data to rest api                                         *
 *****************************************************************/
static unsigned long sendData(const String& data, int pin) {
  // temporary removing data send
  //return 0;

	unsigned long start_send = millis();
	int result = 0;

	String s_Host("api.sensor.community");
	String s_url("/v1/push-sensor-data/");
  int s_Port = 80;

	String contentType = "application/json";

	String esp_chipid = String(ESP.getChipId());
	String esp_mac_id = String(WiFi.macAddress());

	WiFiClient client;

	HTTPClient http;
	http.setTimeout(20 * 1000);
  http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid + '/' + esp_mac_id);
  http.setReuse(false);
	
	if (http.begin(client, s_Host, s_Port, s_url)) 
  {
		http.addHeader(F("Content-Type"), contentType);
		http.addHeader(F("X-Sensor"), "esp8266-" + esp_chipid);
		http.addHeader(F("X-MAC-ID"), "esp8266-" + esp_mac_id);
		http.addHeader(F("X-PIN"), String(pin));

		result = http.POST(data);

		if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) 
    {
			Serial.println("Succeeded!");
		} 
    else if (result >= HTTP_CODE_BAD_REQUEST) {
			Serial.print("Request failed with error: ");
      Serial.println(String(result));
			Serial.println(http.getString());
		}
		http.end();
	} 
  else 
  {
		Serial.println("Failed connecting to ");
	}

	return millis() - start_send;
}


String Float2String(float value)
{
  // Convert a float to String with two decimals.
  char temp[15];
  String s;

  dtostrf(value,13, 2, temp);
  s = String(temp);
  s.trim();
  return s;
}

String getJsonValue(String valueType, int value)
{
  return "{\"value_type\":\"" + valueType + "\",\"value\":\""+ String(value) + "\"}";
}

String getJsonValue(String valueType, float value)
{
  return "{\"value_type\":\"" + valueType + "\",\"value\":\""+ Float2String(value) + "\"}";
}

// DHT22 Sensor
void sensorBME() {
  String data;

	bmx280.takeForcedMeasurement();

	float t = bmx280.readTemperature();
	float p = bmx280.readPressure();
	float h = bmx280.readHumidity();

	if (isnan(t) || isnan(p)) {
		t = -128.0;
		p = -1.0;
		h = -1.0;
		Serial.println("BMP/BME280 read failed");
	}

  // json for push to api: h t
  data = "{\"sensor\": \"" + String(SENSOR_ID) + "\","
       + "\"sensordatavalues\":["
       + getJsonValue("temperature", t) + ","
       + getJsonValue("humidity", h) + ","
       + getJsonValue("pressure", p)
       + "]}";

  Serial.println(data);
  sendData(data, 7); 
}


void sensorPMS()
{
  String data;
  
  auto pmsData = pm2008m.startAndRead();

  data = "{\"sensor\": \"" + String(SENSOR_ID) + "\","
        + "\"sensordatavalues\":["
        + getJsonValue("P1", pmsData.PM10) + ","
        + getJsonValue("P2", pmsData.PM25)
        + "]}";

  Serial.println(data);
  sendData(data, 1);
}

static bool initBMX280() {
	Serial.println("Init BMx280 sensor.");
  
  if (bmx280.begin(0x76)) {
		bmx280.setSampling(
			BMX280::MODE_FORCED,
			BMX280::SAMPLING_X1,
			BMX280::SAMPLING_X1,
			BMX280::SAMPLING_X1);
		return true;
	} else {
		Serial.println("BMx280 sensor is not found.");
    return false;
	}
}

void setup() 
{
  Serial.begin(9600); //Output to Serial at 9600 baud
  
  delay(10);
  starttime = millis(); // store the start time
  
  delay(1000);
  connectWifi(); // Start ConnecWifi 
  Serial.print("\n"); 
  Serial.println("ChipId: ");
  Serial.println(ESP.getChipId());

  initBMX280();

  //Init pm2008m
  Serial.println("pm2008m sensor init.");
  pm2008m.init(D6, D5);
}

void loop() 
{
  // Checking if it is time to sample
  if ((millis()-starttime) > sampletime_ms)
  {
    starttime = millis(); // store the start time
    sensorBME();  // getting temperature and humidity
    sensorPMS();
    Serial.println("------------------------------");
  }
}

