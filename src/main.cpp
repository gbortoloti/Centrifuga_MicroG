//Integração dos Codigos do Wagner (Relé e controle)

//#include <Arduino.h>
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


const char* rede = "microg";
const char* senha = "senhamicrog";
String apiKey = "HIUE1DNTL6744VTI";
const char* server = "api.thingspeak.com";

WiFiClient client;
RHT03 rht;
ESP8266WebServer server2 ( 80 );

//pino rele
const int pinorele =  8;  //Verificar se essa porta está disponivel

//receptor infravermelho TSOP
int RECV_PIN = 11;        //Verificar se essa porta está disponivel
IRrecv irrecv(RECV_PIN);
decode_results results;


//LCD 16x2
unsigned char buffer =0;
#define pino_rs_0    bitClear(buffer,4)
#define pino_rs_1    bitSet(buffer,4)
#define pino_e_0     bitClear(buffer,5)
#define pino_e_1     bitSet(buffer,5)

#define cursor_on    0x0c
#define cursor_off   0x0e
#define cursor_blink 0x0f

void lcd_send4bits(unsigned char com);            //Função que envia um comando de 4 bits(1 nibble) ao LCD.
void lcd_wrcom4(unsigned char com);               //Função que envia um 4 bits(1 nibble) ao LCD, chama a função send4bits.
void lcd_wrcom(unsigned char com);                //Função que envia um commando ao lcd.
void lcd_wrchar(unsigned char ch);                //Função que envia um dado para o lcd.
void lcd_init(unsigned char estado_cursor);        //Função que inicializa o lcd.
void lcd_goto(unsigned char x, unsigned char y);  //Função que desloca o cursor do lcd em uma posiçao do lcd.
void lcd_clear (void);                            //Função que limpa o lcd.
void pcf_wr(unsigned char dat);
int lcd_putc( char c, FILE * );

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
   Wire.begin();
   Serial.begin(9600);
   irrecv.enableIRIn();
   while(!Serial);
  // fdevopen( &lcd_putc, NULL );
   lcd_init(0x0c);
   lcd_goto(5,1);
   printf("MICROG");
   pinMode(pinorele, OUTPUT);
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

  if (irrecv.decode(&results)) {
       Serial.println();

           switch(results.value)
           {
             case 16756815:
               lcd_clear();
               digitalWrite(pinorele, HIGH);
               Serial.print("Liga");
               printf("Liga");
             break;

             case 16775175:
               lcd_clear();
               digitalWrite(pinorele, LOW);
               Serial.print("Desliga");
               printf("Desliga");
             break;

             case 16748655:
               lcd_clear();
               Serial.print("Mais");
               printf("Mais");
             break;

             case 16758855:
               lcd_clear();
               Serial.print("Menos");
               printf("Menos");
             break;

             case 16767015:
               lcd_clear();
               Serial.print("Enter");
               printf("Enter");
             break;

             case 16746615:
               lcd_clear();
               lcd_goto(0,0);
             break;

             case 16754775:
                 for (int i=0; i<100; i++) {
                 Serial.print("\n");}
                 lcd_clear();
             break;

             default:
               lcd_clear();
               Serial.print("Unknown");
               printf("Unknown");
             break;
            }
           delay(200);
           irrecv.resume();
           Serial.println();
 }

  client.stop();

  delay(1000);
}

int lcd_putc( char c, FILE * )
{
  lcd_wrchar( c );
  return 0;
}

void pcf_wr(unsigned char dat)
{
    Wire.beginTransmission(0x38);
    Wire.write(dat);
    Wire.endTransmission();
}


void lcd_send4bits(unsigned char dat)
{
  // D4 0 - D5 1 - D6 2 - D7 3
  buffer=buffer&0xf0;
  buffer=buffer+(dat&0x0f);
  pcf_wr(buffer);
}

void lcd_wrcom4(unsigned char com)
{
  lcd_send4bits(com);
  pino_rs_0;
  pcf_wr(buffer);
  pino_e_1;
  pcf_wr(buffer);
  delayMicroseconds(5);
  pino_e_0;
  pcf_wr(buffer);
  delay(5);
}

void lcd_wrcom(unsigned char com)
{
  lcd_send4bits(com/0x10); // D7...D4
  pino_rs_0;
  pcf_wr(buffer);
  pino_e_1;
  pcf_wr(buffer);
  delayMicroseconds(5);
  pino_e_0;
  pcf_wr(buffer);
  delayMicroseconds(5);

  lcd_send4bits(com%0x10); // D3...D0
  pino_e_1;
  pcf_wr(buffer);
  delayMicroseconds(5);
  pino_e_0;
  pcf_wr(buffer);
  delay(5);
}

void lcd_wrchar(unsigned char ch)
{
  lcd_send4bits(ch/0x10); // D7...D4
  pino_rs_1;
  pcf_wr(buffer);
  pino_e_1;
  pcf_wr(buffer);
  delayMicroseconds(5);
  pino_e_0;
  pcf_wr(buffer);
  delayMicroseconds(5);

  lcd_send4bits(ch%0x10); // D3...D0
  pino_e_1;
  pcf_wr(buffer);
  delayMicroseconds(5);
  pino_e_0;
  pcf_wr(buffer);
  delay(5);
}

void lcd_init(unsigned char estado_cursor)
{
  lcd_wrcom4(3);
  lcd_wrcom4(3);
  lcd_wrcom4(3);
  lcd_wrcom4(2);
  lcd_wrcom(0x28);
  lcd_wrcom(estado_cursor);
  lcd_wrcom(0x06);
  lcd_wrcom(0x01);
}

void lcd_goto(unsigned char x, unsigned char y)
{
  if(x<16)
  {
    if(y==0) lcd_wrcom(0x80+x);
    if(y==1) lcd_wrcom(0xc0+x);
    if(y==2) lcd_wrcom(0x90+x);
    if(y==3) lcd_wrcom(0xd0+x);
  }
}

void lcd_clear (void)
{
    lcd_wrcom(0x01);
}
