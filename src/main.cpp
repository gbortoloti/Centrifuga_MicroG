//Integração dos Codigos do Wagner (Relé e controle)

#include <Arduino.h>
#include <sensor_Temp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <IRremote.h>

const int LEDPIN = 15; //Porta D10 na placa
String  estadoLed = "OFF";

//Pinos para funcionamento do RHT03
const int RHT03_DATA_IN = 0; // Porta D8 na placa
const int RHT03_DATA_OUT = 16; //Porta D2 na placa
float latestHumidity = 0;
float latestTempC = 0;

// Variaveis para o funcionamento do sensor de luminosidade
#define LDR_PIN                   0 //Porta A0
#define MAX_ADC_READING           1023
#define ADC_REF_VOLTAGE           5.0
#define REF_RESISTANCE            5030  // measure this for best results
#define LUX_CALC_SCALAR           12518931
#define LUX_CALC_EXPONENT         -1.25
int ldrRawData;
float resistorVoltage, ldrVoltage;
float ldrLux;
float ldrResistance;


const char* rede = "BOLSONARO2018.";
const char* senha = "faymicrog";
String apiKey = "HIUE1DNTL6744VTI";
const char* server = "api.thingspeak.com";

WiFiClient client;
RHT03 rht;
ESP8266WebServer server2 ( 80 );

String getPage()
{
  String page = "<html lang=pt-BR><head><meta http-equiv='refresh' content='10'/>";
  page += "<title>ESP8266 Demo - www.projetsdiy.fr</title>";
  page += "<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>";
  page += "</head><body><h1>ESP8266 Demo</h1>";
  page += "<h3>RHT03</h3>";
  page += "<ul><li>Temperatura: ";
  page += latestTempC;
  page += " graus Celsius</li>";
  page += "<li>Umidade: ";
  page += latestHumidity;
  page += "%</li>";
  page +="<h3>LDR</h3>";
  page += "<li>Luminosidade: ";
  page += ldrLux;
  page += "Lux</li>";
  page += "<h3>Acionamento da Carga</h3>";
  page += "<form action='/' method='POST'>";
  page += "<ul><li>CARGA (Estado: ";
  page += estadoLed;
  page += ")";
  page += "<INPUT type='radio' name='LED' value='1'>ON";
  page += "<INPUT type='radio' name='LED' value='0'>OFF</li></ul>";
  page += "<INPUT type='submit' value='Atualizar'>";
  page += "</body></html>";
  return page;
}

void handleSubmit()
{
  // Update GPIO
  String LEDValue;
  LEDValue = server2.arg("LED");
  //Serial.println("Set GPIO "); Serial.print(LEDValue);

  if ( LEDValue == "1" )
  {
    //digitalWrite(LEDPIN, 1);
    digitalWrite(LEDPIN, HIGH);
    estadoLed = "On";
    server2.send ( 200, "text/html", getPage() );
  }

  else if ( LEDValue == "0" )
  {
    //digitalWrite(LEDPIN, 0);
    digitalWrite(LEDPIN, LOW);
    estadoLed = "Off";
    server2.send ( 200, "text/html", getPage());
  }

  else
  {
    Serial.println("Err Led Value");
  }
}

void handleRoot()
{
  if ( server2.hasArg("LED"))
  {
    handleSubmit();
  }
  else
  {
    server2.send ( 200, "text/html", getPage() );
  }
}


void setup()
 {
  Serial.begin(9600);
  delay(10);

  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  Serial.println();
  Serial.println();
  Serial.println("Conectando-se a");
  Serial.print(rede);
  Serial.println();

  WiFi.begin(rede, senha);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Conectado!");
  //Server.begin();

  Serial.print("Use esse URL para conectar: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/");

  server2.on ( "/", handleRoot );

  server2.begin();
  Serial.println ( "HTTP server started" );

  rht.begin(RHT03_DATA_IN, RHT03_DATA_OUT);
}

void loop()
{
  server2.handleClient();
  // Call rht.update() to get new humidity and temperature values from the sensor.
  int updateRet = rht.update();

  // If successful, the update() function will return 1.
  // If update fails, it will return a value <0
  if (updateRet == 1)
  {
    // The humidity(), tempC(), and tempF() functions can be called -- after
    // a successful update() -- to get the last humidity and temperature
    // value
    latestHumidity = rht.humidity();
    latestTempC = rht.tempC();

    ldrRawData = analogRead(LDR_PIN);
    resistorVoltage = (float)ldrRawData / MAX_ADC_READING * ADC_REF_VOLTAGE;
    ldrVoltage = ADC_REF_VOLTAGE - resistorVoltage;
    ldrResistance = ldrVoltage / resistorVoltage * REF_RESISTANCE;
    ldrLux = LUX_CALC_SCALAR * pow(ldrResistance, LUX_CALC_EXPONENT);


    // Now print the values:
    Serial.println("Umidade: " + String(latestHumidity, 1) + "%");
    Serial.println("Temp (C): " + String(latestTempC, 1) + " deg C");
    Serial.println("Luminosidade: "+ String(ldrLux, 1) + "lux");

    if(client.connect(server, 80))
    {
      String postStr=apiKey;
        postStr+="&amp;field1=";
        postStr += String(latestTempC);
        postStr +="&amp;field2=";
        postStr += String(latestHumidity);
        postStr += "&amp;field3=";
        postStr += String(ldrLux);
        postStr += "\r\n\r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);
    }
  }
  else
  {
    // If the update failed, try delaying for RHT_READ_INTERVAL_MS ms before
    // trying again.
    Serial.println("Esperando...");
    Serial.println(RHT_READ_INTERVAL_MS);
    delay(RHT_READ_INTERVAL_MS);
  }

  client.stop();

  delay(1000);
}
