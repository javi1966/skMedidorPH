#include <ESP8266WiFi.h>
//#include <TimerOne.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Adafruit_ADS1015.h"

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;

const int oneWireBus = 14;      //GPIO14-D5

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensorDS18B20(&oneWire);


#define AP_SSID "Wireless-N"
#define AP_PASSWORD "z123456z"

#define CH_TDS 3
#define CH_PH 0
#define DEBUG true
const int LED =  13;
const int RELE = 15;
#define SensorPin 0            //pH meter Analog output to Arduino Analog Input 0
#define Offset -1.75            //deviation compensate
int32_t avgValue;              //Store the average value of the sensor feedback
bool toggle = false;

float valorPH = 0.0;


#define SCOUNT  30           // sum of sample point
int32_t analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC

float averageVoltage = 0, tdsValue = 0, temperature = 25;



WiFiServer server(80);


//**********************************************************************************
void setup()
{

  pinMode(RELE, OUTPUT);
  pinMode(LED, OUTPUT);

  Serial.begin(115200);

  digitalWrite(RELE, true);

  ads.setGain(GAIN_TWOTHIRDS);

  ads.begin();

  sensorDS18B20.begin();  //DS18b20


  //Timer1.initialize(500000);//timing for 500ms
  // Timer1.attachInterrupt(TimingISR);//declare the interrupt serve routine:TimingISR

  wifiConnect();

  delay(1000);
  Serial.println("LISTO!!!");    //Test the serial monitor*/
}



//*****************************************************************************
void loop()
{

     


      WiFiClient client = server.available();

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

       String str = "HTTP/1.1 200 OK\r\nContent-Type: text/json\r\n\r\n{'PH':";
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

       String str = "HTTP/1.1 200 OK\r\nContent-Type: text/json\r\n\r\n{'TDS':";
       String valorTDS = String(mideTDS(), 2);
       String cierre = "}";

       Serial.println(str+valorTDS+cierre);
       client.print(str+valorTDS+cierre);

       delay(1);
       Serial.println("Client disonnected");

      }
      else if (req.indexOf("temperaturaTDS/") != -1)
      {
      

       delay(1000);

       String str = "HTTP/1.1 200 OK\r\nContent-Type: text/json\r\n\r\n{'TemperaturaTDS':";
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

  

}//loop

  //************************************************************************************************
  void TimingISR()
  {
    static byte cntTemp = 0;

    toggle = !toggle;
    digitalWrite(LED, toggle);

    cntTemp = 0;

  }
  //***********************************************************************************************
  int getMedianNum(int bArray[], int iFilterLen)
  {
    int bTab[iFilterLen];
    for (byte i = 0; i < iFilterLen; i++)
      bTab[i] = bArray[i];
    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++)
    {
      for (i = 0; i < iFilterLen - j - 1; i++)
      {
        if (bTab[i] > bTab[i + 1])
        {
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

    float Voltage = 0.0;
    float phValue=0.0;
      
    Voltage=mideADS115(CH_PH);
    Serial.print("Voltaje: ");
    Serial.println(Voltage);
    

    //phValue = -5.70 * Voltage + 21.34;
    phValue=7 - (2.5 - Voltage) * -3.8;
    Serial.print("pH:");
    Serial.print(phValue, 1);
    Serial.println(" ");

    return phValue;

  }

  //********************************************************************************************
  float mideTDS() {

    float temperature  = 0.0;
    float Voltage = 0.0;
  

    Voltage=mideADS115(CH_TDS);
    Serial.print("Voltaje: ");
    Serial.println(Voltage);

    temperature=mideTemperatura();


    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = Voltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
    Serial.println(tdsValue);

    return tdsValue;

  }
  //*********************************************************************************************
  float mideADS115(int ch){

    float Voltage = 0;
    const float multiplier = 0.1875F;  //ADS1115 ,GAIN_TWOTHIRDS  6,144V  0,1875mV
    int buf[SCOUNT];
    
    for(int i=0;i<SCOUNT;i++){
      buf[i] = ads.readADC_SingleEnded(ch);    //read the analog value and store into the buffer
      delay(10);
    }

    
    Voltage = (getMedianNum(buf,SCOUNT) * multiplier ) /1000;
        

    //sensorValue = ads.readADC_SingleEnded(ch);
   // Serial.print("Valor sensor: ");
   // Serial.println(sensorValue);

   
   
    return Voltage;
    
  }
  //*******************************************************************************************
  float mideTemperatura(){

      sensorDS18B20.requestTemperatures(); 
      float temperatureC = sensorDS18B20.getTempCByIndex(0);
     
      Serial.print(temperatureC);
      Serial.println("ºC");

      return temperatureC;


    
  }
  

  //******************************************************************************************
  void wifiConnect()
  {

    WiFi.mode(WIFI_STA);

    Serial.println("Connecting to AP");
    WiFi.begin(AP_SSID, AP_PASSWORD);

    WiFi.config(IPAddress(192, 168, 1, 252), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print("*");

      if (++timeout > 100)
      {
        Serial.println("Sin Conexion WIFI");
        while (1) {
          digitalWrite(LED, LOW);
          delay(100);
          digitalWrite(LED, HIGH);
          delay(100);
        }
      }

    }

    IPAddress http_server_ip = WiFi.localIP();

    server.begin();

    Serial.println("Nuestra server IP:");
    Serial.print(http_server_ip);
    Serial.print("\r\n");


  }
