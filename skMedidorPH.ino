#include <ESP8266WiFi.h>
//#include <TimerOne.h>
#include "Adafruit_ADS1015.h"

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;

#define AP_SSID "Wireless-N"
#define AP_PASSWORD "z123456z"


#define DEBUG true
const int LED = 13;
#define SensorPin 0            //pH meter Analog output to Arduino Analog Input 0
#define Offset -1.75            //deviation compensate
int32_t avgValue;              //Store the average value of the sensor feedback
bool toggle = false;

float valorPH = 0.0;
WiFiServer server(80);


//**********************************************************************************
void setup()
{
  pinMode(13, OUTPUT);

  Serial.begin(115200);

  ads.begin();

 
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

    String str = "HTTP/1.1 200 OK\r\nContent-Type: text/json\r\n\r\n{'TDS':";
    String valorTDS = String(mideTDS(), 2);
    String cierre = "}";

    Serial.println(str+valorTDS+cierre);
    client.print(str+valorTDS+cierre);
    
    delay(1);
    Serial.println("Client disonnected");
   
  }

  else
  {

    Serial.println("invalid request");
    client.stop();
    return;
  }




 /*int16_t adc0, adc1, adc2, adc3;

  adc0 = ads.readADC_SingleEnded(0);

 Serial.print("AIN0: "); Serial.println(midePH());

  delay(1000);*/

 

}
//************************************************************************************************
void TimingISR()
{
  static byte cntTemp = 0;

  toggle = !toggle;
  digitalWrite(LED, toggle);

  cntTemp = 0;

}



//*******************************************************************************************
float midePH() {

  int16_t buf[20];                //buffer for read analog
  for (int i = 0; i < 10; i++) //Get 10 sample value from the sensor for smooth the value
  {
    buf[i] = ads.readADC_SingleEnded(0);
    delay(10);
  }

  for (int i = 0; i < 9; i++) //sort the analog from small to large
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        int16_t temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  //for (int i = 0; i < 10; i++)
  //  Serial.println(buf[i]);

  avgValue = 0L;
  for (int i = 2; i < 8; i++)               //take the average value of 6 center sample
    avgValue += buf[i];
  // Serial.println(avgValue);
  // float phValue = (float)avgValue * 5.0 / 1024 / 6; //convert the analog into millivolt
  float phValue = (float)avgValue / 6.0 * 0.1875;

  //phValue = 3.5 * phValue/1000 + Offset;                //convert the millivolt into pH value

  phValue = -5.70 * phValue / 1000 + 21.34;
  Serial.print("    pH:");
  Serial.print(phValue, 1);
  Serial.println(" ");

  return phValue;

}

//********************************************************************************************
float mideTDS() {

  float Voltage =0;
  float tdsValue =0;
  int16_t sensorValue = 0;
  const float multiplier = 0.1875F;
  float temperature  =25;

  int16_t buf[20];                //buffer for read analog
  for (int i = 0; i < 10; i++) //Get 10 sample value from the sensor for smooth the value
  {
    buf[i] = ads.readADC_SingleEnded(1) ;
    delay(10);
  }

  for (int i = 0; i < 9; i++) //sort the analog from small to large
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        int16_t temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  //for (int i = 0; i < 10; i++)
  //  Serial.println(buf[i]);

  avgValue = 0L;
  for (int i = 2; i < 8; i++)               //take the average value of 6 center sample
    avgValue += buf[i];

    
    sensorValue=ads.readADC_SingleEnded(1);
    Voltage= sensorValue * multiplier;
    Voltage=Voltage/1000;
    float compensationCoefficient=1.0+0.02*(temperature-25.0); 
    float compensationVoltage=Voltage/compensationCoefficient;
    tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
   // Serial.println(tdsValue);
  
  
  return tdsValue;

}

//******************************************************************************************
void wifiConnect()
{

  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to AP");
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
