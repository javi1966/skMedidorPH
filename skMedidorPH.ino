
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ElegantOTA.h>  //http://192.168.1.252/update
#include <Wire.h>
#include <TCS34725.h>
#include "credentials.h"


const int TDS = 0;
const int PH = 1;
const int RELE = 2;
const int DS18B20 = 3;
const int I2C_SDA = 21;
const int I2C_SCL = 20;
const int LED = 8;

boolean bLed = false;

TCS34725 tcs;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(DS18B20);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensorDS18B20(&oneWire);



int32_t avgValue;  //Store the average value of the sensor feedback
bool toggle = false;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


#define SCOUNT 30              // sum of sample point
int32_t analogBuffer[SCOUNT];  // store the analog value in the array, read from ADC

float averageVoltage = 0;

AsyncWebServer server(80);

//***********************************************************************************
void IRAM_ATTR onTimer() {

  bLed = !bLed;

  digitalWrite(LED, bLed);
}


//**********************************************************************************
void setup() {

  Serial.begin(115200);

  pinMode(RELE, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(DS18B20, INPUT);
  pinMode(PH, INPUT);
  pinMode(TDS, INPUT);

 

  digitalWrite(RELE, false);
  digitalWrite(LED, HIGH);

  
  Wire.begin(21,20);
  if (!tcs.attach(Wire))
        Serial.println("ERROR: TCS34725 NOT FOUND !!!");

  tcs.integrationTime(33); // ms
  tcs.gain(TCS34725::Gain::X01);

  sensorDS18B20.begin();  //DS18b20

  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000000, true, 0);

  WiFi.mode(WIFI_STA);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  WiFi.config(IPAddress(192, 168, 1, 252), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0), IPAddress(8, 8, 8, 8));

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("*");
  }

  Serial.println("");
  Serial.println("STAIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  //**********************************************************************************
  server.on("/ph", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELE, false);

    delay(1000);

    String salJSON = "{\"ph\":";
    salJSON += String(midePH());
    salJSON += "}";

    Serial.println(salJSON);

    AsyncWebServerResponse *response = request->beginResponse(200, "text/json", salJSON);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  //**********************************************************************************
  server.on("/tds", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELE, true);

    delay(1000);

    String salJSON = "{\"tds\":";
    salJSON += String(mideTDS());
    salJSON += "}";

    Serial.println(salJSON);

    AsyncWebServerResponse *response = request->beginResponse(200, "text/json", salJSON);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  //*********************************************************************************
  server.on("/temperaturaTDS", HTTP_GET, [](AsyncWebServerRequest *request) {
    String salJSON = "{\"temperatura\":";
    salJSON += String(mideTemperatura());
    salJSON += "}";

    Serial.println(salJSON);

    AsyncWebServerResponse *response = request->beginResponse(200, "text/json", salJSON);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  //**********************************************************************************
  server.on("/temp_cpu", HTTP_GET, [](AsyncWebServerRequest *request) {

    String salJSON = "{\"temp_CPU\":";
    salJSON += String(temperatureRead());
    salJSON += "}";

    Serial.println(salJSON);

    AsyncWebServerResponse *response = request->beginResponse(200, "text/json", salJSON);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  //*****************************************************************************************
  server.on("/luxometro", HTTP_GET, [](AsyncWebServerRequest *request) {

    /*uint16_t r, g, b, c, colorTemp, lux;

    if (tcs.available())
    {
        static uint32_t prev_ms = millis();

        TCS34725::Color color = tcs.color();
        Serial.print("Interval   : "); Serial.println(millis() - prev_ms);
        Serial.print("Color Temp : "); Serial.println(tcs.colorTemperature());
        Serial.print("Lux        : "); Serial.println(tcs.lux());
        Serial.print("R          : "); Serial.println(color.r);
        Serial.print("G          : "); Serial.println(color.g);
        Serial.print("B          : "); Serial.println(color.b);


        TCS34725::RawData raw = tcs.raw();
        Serial.print("Raw R      : "); Serial.println(raw.r);
        Serial.print("Raw G      : "); Serial.println(raw.g);
        Serial.print("Raw B      : "); Serial.println(raw.b);
        Serial.print("Raw C      : "); Serial.println(raw.c);

        prev_ms = millis();
    }

    Serial.printf("R: %d,G: %d,B: %d,Temp. Color: %d,lux: %d\n", r, g, b, colorTemp, lux);
    */ 
    String salJSON = "{\"temp_CPU\":";
    salJSON += String(temperatureRead());
    salJSON += "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/json", salJSON);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  //*****************************************************************************************

  ElegantOTA.begin(&server);  // Start ElegantOTA
  // ElegantOTA callbacks
  //ElegantOTA.onStart(onOTAStart);
  //ElegantOTA.onProgress(onOTAProgress);
  //ElegantOTA.onEnd(onOTAEnd);
  //server.onNotFound(notFound);
  server.begin();

  Serial.println("INICIO OTA");

  delay(1000);
  Serial.println("LISTO!!!");  //Test the serial monitor*/
}



//*****************************************************************************
void loop() {
  ElegantOTA.loop();

  /*WiFiClient client = server.available();

      if (!client) {
       return;
      }
      Serial.print("NUEVO CLIENTE\r\n");

      while (!client.connected()) {

       delay(1);
      }

      String req = client.readStringUntil('\r');
      Serial.print(req);
      Serial.print("\r\n");
      client.flush();




      if (req.indexOf("ph/") != -1)
      {

       digitalWrite(RELE,false);

       delay(1000);

       String str = "HTTP/1.1 200 OK\r\nContent-Type: text/json\r\n\r\n{\"PH\":";
       String valorPH = String(midePH(), 2);
       String cierre = "}";

       Serial.println(str+valorPH+cierre);
       client.print(str+valorPH+cierre);

       delay(1);
       Serial.println("Client disonnected");


      }

      else if (req.indexOf("tds/") != -1)
      {
       digitalWrite(RELE,true);

       delay(1000);

       float valueTDS=mideTDS();

       String str = "HTTP/1.1 200 OK\r\nContent-Type: text/json\r\n\r\n{\"ºd\":";
       String valorD=String(valueTDS*0.0556,1);
       String valorF=",\"ºf\":"+String(valueTDS*0.1,1);
       String valorTDS =",\"TDS\":"+ String((int)valueTDS);
       String cierre = "}";

       Serial.println(str+valorD+valorF+valorTDS+cierre);
       client.print(str+valorD+valorF+valorTDS+cierre);

       delay(1);
       Serial.println("Client disonnected");

      }
      else if (req.indexOf("temperaturaTDS/") != -1)
      {
      

       delay(1000);

       String str = "HTTP/1.1 200 OK\r\nContent-Type: text/json\r\n\r\n{\"TemperaturaTDS\":";
       String valorTemperatura = String(mideTemperatura(), 2);
       String cierre = "}";

       Serial.println(str+valorTemperatura+cierre);
       client.print(str+valorTemperatura+cierre);

       delay(10);
       Serial.println("Client disonnected");

      }

      else
      {

       Serial.println("invalid request");
       client.stop();
       return;
      }

      */



}  //loop


//***********************************************************************************************
int getMedianNum(int bArray[], int iFilterLen) {

  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}


//*******************************************************************************************
float midePH() {

  //Sensor PH4502C
  uint32_t measurings = 0;
  float phValue = 0.0;
  float coeffDivisor = 0.68;


  for (int i = 0; i < 10; i++) {
    measurings += analogRead(PH);
    delay(10);
  }


  Serial.printf("RAW : %d\n", measurings / 10);

  float voltage = (3.3 / 4095.0) * measurings / 10;

  Serial.printf("Voltaje PH: %2.2f V.\n", voltage);


  phValue = 7 + ((1.65 - voltage) / 0.18);


  Serial.printf("pH: %1.1f\n", phValue);

  return phValue + 1;
}

//********************************************************************************************
float mideTDS() {

  uint32_t measurings = 0;
  float temperature = 25.0;
  uint32_t valorTDS = 0;

  for (int i = 0; i < 20; i++) {
    measurings += analogRead(TDS);
    delay(10);
  }

  float voltage = 3.3 / 4095.0 * measurings / 20;

  Serial.printf("Voltaje TDS: %2.1f V.\n", voltage);

  temperature = mideTemperatura();

  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = voltage / compensationCoefficient;
  valorTDS = (133.42 * pow(compensationVoltage, 3) - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
  Serial.printf("TDS: %d\n", valorTDS);

  return valorTDS;
}

//*******************************************************************************************
float mideTemperatura() {

  sensorDS18B20.requestTemperatures();
  float temperatureC = sensorDS18B20.getTempCByIndex(0);

  Serial.printf("T: %2.1fºC\n", temperatureC);

  return temperatureC;
}

//***************************************************************************************
