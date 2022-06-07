//*************************************************************************************************************

#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif



int status;     // the Wifi radio's status

const char* ssid        = "RED ACCESA";
const char* password    = "037E32E7";

#if (ESP8266)
  #include <ESP8266WiFi.h>
#elif (ESP32)
  #include <WiFi.h>
#endif

#include <Ticker.h>

// 10s
#define HEARTBEAT_INTERVAL        10

int input_contador = 33;
int relay_boleto_completo = 2;
int relay_alterno = 15;
Ticker ticker1;



void heartBeatPrint()
{
   Serial2.println("T");
   delay(10);
   Serial2.println("BIENVENIDOS~");

   Serial.println("LED...");
}




void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600);
  //while (!Serial);

  delay(200);
  //WiFi.mode(WIFI_STA);

  //WiFi.begin(ssid, password);
  
  Serial.print(F("Connecting to WiFi SSID: ")); Serial.println(ssid);
/*
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }
  */
   pinMode(input_contador, INPUT);
   pinMode(relay_boleto_completo,OUTPUT);
   pinMode(relay_alterno,OUTPUT);
   digitalWrite(relay_alterno,LOW);
   Serial2.println("T");
   delay(10);
   Serial2.println("BIENVENIDOS~");

   ticker1.attach(HEARTBEAT_INTERVAL, heartBeatPrint);
  
}

void loop()
{ 
  int Push_button_state = digitalRead(input_contador);
  if(Push_button_state== HIGH){
    Serial.print("se recive señal de contador, imprimir boleto");
    Serial2.println("T");
    delay(10);
    Serial2.println("9.00~");
    digitalWrite(relay_boleto_completo,HIGH);
    delay(500);
    digitalWrite(relay_boleto_completo,LOW);
  }
  else{
    digitalWrite(relay_boleto_completo,LOW);
  }
}
