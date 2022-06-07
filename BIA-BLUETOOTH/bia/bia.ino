#include <EEPROM.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <AsyncHTTPRequest_Generic.h> // https://github.com/khoih-prog/AsyncHTTPRequest_Generic
#include <Wire.h>                     // for I2C with RTC module
#include "RTClib.h"                   //to show time
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
//#include <Fonts/FreeMono9pt7b.h>
#include <Ticker.h>
#include "update_firmware.h"
#include "config_wifi_esp32.h"

// 3 seconds WDT
#define WDT_TIMEOUT 10
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD13ejecutaco06 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define CAPACIDAD_EEPROM 512
#define DEBOUNCETIME 50
#define SENSOR_GAVETA 18
#define publicaDatos 10
#define intentaReconexion 6
#define USB_SERIAL Serial
#define obtenerEstatus 5
#define direccionGavetaAbierta 200
#define ESTADO_PAPEL 4
#define ESTADO_IMP 1
#define CAUSA_OFFLINE 2
#define TIPO_ERROR 3

uint8_t contadorPublicaDatos = 0, contadorIntentaReconexion = 0;
bool banderaCorte = false, suscrito = false, banderaWDT = false, banderaEnviar = true, handleResponseFlag = false, handleHttpCodeFlag = false;
String rutaServidor = "http://c68d9ab.online-server.cloud/TIA/phpESP/camiones.php";
String numeroSerie = "TIAANBRIOYRK2", versionFirmware = "Firmware: 2.0", payloadConteo = "";
char ruta[] = "", unidad[] = "";
String VENTAS = "2222608242", REPORTES = "";

// 1s
#define HEARTBEAT_INTERVAL 1
//#define HEARTBEAT_STATUS          1
float HEARTBEAT_STATUS = 0.01; // seconds

void status_led_datos();

Ticker ticker1;
Ticker ticker2;
// Ticker tickerObject(callbackFunction, 100, 0, MICROS_MICROS) // interval is now 100us
// Ticker ticker2(status_led_datos, 100, 0, MICROS_MICROS); // internal resolution is MICRO seconds

int LED_STATUS = 15;
int LED_LINK = 2;
int LED_ACTIVITY = 32;
// int PWM1_DutyCycle = 0;
//#define PWM1_Ch    0
//#define PWM1_Res   4
//#define PWM1_Freq  10

int toggle_activity = 0;
int toggle_link = 0;
int toggle_status = 0;
int actividad_datos = 0;

/*
char ssid[] = "RED TIA";
char pass[] = "12345678";
*/

const char *ssid = "RED ACCESA";
const char *pass = "037E32E7";

// setting PWM properties
const int buzzer = 33;
const int buzzerChannel = 0;
const int resolution = 8;

int responseHttpCode = 0;

struct SensorMag
{
  const uint8_t PIN;
  volatile bool banderaInterrupt;
  uint8_t vecesAbierta;
  String fechaTimestamp;
  DateTime fechaApertura;
  volatile uint8_t estado;
};
class Total
{
public:
  Total(short direccionCuenta)
  {
    this->direccionCuenta = direccionCuenta;
  }
  short GetCuentaTotal()
  {
    short cuentaTotal = EEPROM.readShort(this->direccionCuenta);
    return cuentaTotal;
  }
  void SetCuentaTotal(int total)
  {
    EEPROM.put(this->direccionCuenta, total);
    EEPROM.commit();
  }

private:
  short direccionCuenta;
};
class Boton
{
public:
  Boton(uint8_t pin, float costo, short direccionCuenta, short direccionDinero, String nombre)
  {
    this->pin = pin;
    this->costo = costo;
    this->direccionCuenta = direccionCuenta;
    this->direccionDinero = direccionDinero;
    this->nombre = nombre;
    pinMode(this->pin, INPUT);
  }
  int GetCuenta()
  {
    short cuenta = EEPROM.readShort(this->direccionCuenta);
    return cuenta;
  }
  void SetCuenta(int cuenta)
  {
    EEPROM.put(this->direccionCuenta, cuenta);
    EEPROM.commit();
  }
  float GetDinero()
  {
    float dinero = EEPROM.readFloat(this->direccionDinero);
    return dinero;
  }
  void SetDinero(float dinero)
  {
    EEPROM.put(this->direccionDinero, dinero);
    EEPROM.commit();
  }
  void Reset()
  {
    short resetCuenta = 0;
    SetCuenta(0);
    float resetDinero = 0;
    SetDinero(0);
  }
  uint8_t pin;
  float costo;
  String nombre;
  volatile bool lastState;
  bool saveLastState;
  volatile uint32_t debounceTimeout = 0;
  uint32_t saveDebounceTimeout;
  bool bandera = false;

private:
  short direccionCuenta;
  short direccionDinero;
};
enum TipoSolicitud
{
  TOTAL
};
// OBJETOS
AsyncHTTPRequest ConteoRequest;

Boton completo(12, 8.50, 0, 2, "COMPLETO");
Boton estudiante(14, 6.00, 6, 8, "ESTUDIANTE");
Boton terceraEdad(26, 4.00, 12, 14, "TERCERA EDAD");
Boton capacidadDiferente(27, 0.00, 18, 20, "CAPACIDAD DIFERENTE");
Boton generarCorte(25, 0.00, 0, 0, "CORTE");
Total total(24);
SensorMag SensorGaveta = {SENSOR_GAVETA};
HardwareSerial serialImpresora(2);
RTC_DS3231 rtc;
hw_timer_t *timer = NULL;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TaskHandle_t TareaBuscarWiFi;

// For setting up critical sections (enableinterrupts and disableinterrupts not available)
// used to disable and interrupt interrupts
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux1 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux2 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux3 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux4 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE muxTimer = portMUX_INITIALIZER_UNLOCKED;

void borrarRenglonOled(int direccionHorizontal, int direccionVertical, byte ancho);
void borrarIntensidadWiFi();

void BuscarWiFi(void *parameter)
{
  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      if (contadorIntentaReconexion >= intentaReconexion)
      {
        USB_SERIAL.println("Buscando WiFi...");
        digitalWrite(LED_LINK, LOW);
        reconexionWifi();
        contadorIntentaReconexion = 0;
      }
    }
  }
}

void IRAM_ATTR completoISR()
{
  portENTER_CRITICAL_ISR(&mux);
  completo.lastState = digitalRead(completo.pin);
  completo.debounceTimeout = xTaskGetTickCount(); // version of millis() that works from interrupt
  completo.bandera = true;
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR estudianteISR()
{
  portENTER_CRITICAL_ISR(&mux1);
  estudiante.lastState = digitalRead(estudiante.pin);
  estudiante.debounceTimeout = xTaskGetTickCount(); // version of millis() that works from interrupt
  estudiante.bandera = true;
  portEXIT_CRITICAL_ISR(&mux1);
}

void IRAM_ATTR terceraEdadISR()
{
  portENTER_CRITICAL_ISR(&mux2);
  terceraEdad.lastState = digitalRead(terceraEdad.pin);
  terceraEdad.debounceTimeout = xTaskGetTickCount(); // version of millis() that works from interrupt
  terceraEdad.bandera = true;
  portEXIT_CRITICAL_ISR(&mux2);
}

void IRAM_ATTR capacidadDiferenteISR()
{
  portENTER_CRITICAL_ISR(&mux3);
  capacidadDiferente.lastState = digitalRead(capacidadDiferente.pin);
  capacidadDiferente.debounceTimeout = xTaskGetTickCount(); // version of millis() that works from interrupt
  capacidadDiferente.bandera = true;
  portEXIT_CRITICAL_ISR(&mux3);
}

void IRAM_ATTR generarCorteISR()
{
  portENTER_CRITICAL_ISR(&mux4);
  generarCorte.lastState = digitalRead(generarCorte.pin);
  generarCorte.debounceTimeout = xTaskGetTickCount(); // version of millis() that works from interrupt
  generarCorte.bandera = true;
  portEXIT_CRITICAL_ISR(&mux4);
}

void IRAM_ATTR guardarFechaGaveta()
{
  SensorGaveta.banderaInterrupt = true;
}

void setup()
{
  USB_SERIAL.begin(115200);
  USB_SERIAL.println();
  serialImpresora.begin(38400);

  ///////////////// inicializaion para configuracion de wifi ///////////////////////////

  preferences.begin("wifi_access", false);

  if (!init_wifi())
  { // Connect to Wi-Fi fails
    Serial.println("Wifi paso 1");
    SerialBT.register_callback(callback);
  }
  else
  {
    Serial.println("Wifi paso 2");
    SerialBT.register_callback(callback_show_ip);
    bluetooth_disconnect = true;
  }
  Serial.println("Wifi paso 3");

  SerialBT.begin(bluetooth_name);

  //
  while (bluetooth_disconnect == false)
  {
    Serial.println("Wifi paso 4");
    configurar_wifi();
  }

  // SerialBT.begin(bluetooth_name);

  //////////////////////////////////////////////////////////////////////////////////////

  /*
  if (rtc.lostPower()) {
      Serial.println("RTC lost power, let's set the time!");
      // When time needs to be set on a new device, or after a power loss, the
      // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2022, 1, 21, 3, 0, 0));
    }
  */
  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2022, 3, 4, 12, 30, 0));

  iniciaImpresora();
  EEPROM.begin(CAPACIDAD_EEPROM);

  pinMode(SensorGaveta.PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(completo.pin), completoISR, RISING);
  attachInterrupt(digitalPinToInterrupt(estudiante.pin), estudianteISR, RISING);
  attachInterrupt(digitalPinToInterrupt(terceraEdad.pin), terceraEdadISR, RISING);
  attachInterrupt(digitalPinToInterrupt(capacidadDiferente.pin), capacidadDiferenteISR, RISING);
  attachInterrupt(digitalPinToInterrupt(generarCorte.pin), generarCorteISR, RISING);
  attachInterrupt(digitalPinToInterrupt(SensorGaveta.PIN), guardarFechaGaveta, FALLING);

  xTaskCreatePinnedToCore(
      BuscarWiFi,       // Function to implement the task
      "BuscarWiFi",     // Name of the task
      10000,            // Stack size in words (32 bits)
      NULL,             // task input parameter
      0,                // priority of the task (0 most important)
      &TareaBuscarWiFi, // TaskHandle
      0                 // Core where the task should run
  );
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  startTimer();
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  display.clearDisplay();
  dibujaNumeroSerieVersion();

  imprimeLogo();

  SensorGaveta.vecesAbierta = EEPROM.read(direccionGavetaAbierta);
  USB_SERIAL.println(SensorGaveta.vecesAbierta);
  suscrito = EEPROM.read(180);
  dibujaEstadoServicio(suscrito);
  dibujaVigenciaEEPROM();

  ConteoRequest.setDebug(false);
  ConteoRequest.onReadyStateChange(ConteoCB);
  /*
  EEPROM.put(123, 2);
  EEPROM.commit();
  delay(50);
  EEPROM.put(129, 2);
  EEPROM.commit();
  delay(50);
  */
  ticker2.attach(HEARTBEAT_STATUS, status_led_datos);
  ticker1.attach(HEARTBEAT_INTERVAL, heartBeatPrint);
  // Ticker tickerObject(callbackFunction, 1000, 0, MILLIS)

  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_LINK, OUTPUT);
  pinMode(LED_ACTIVITY, OUTPUT);

  // ledcAttachPin(LED_ACTIVITY, PWM1_Ch);
  // ledcSetup(PWM1_Ch, PWM1_Freq, PWM1_Res);

  // digitalWrite(LED_STATUS,HIGH);
  // digitalWrite(LED_LINK,HIGH);

  rtc.adjust(DateTime(2022, 3, 4, 12, 30, 0));
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);               // add current thread to WDT watch

  // arranquemDash=true;
}

// void reconexionWifi()
//{
//   int redesEncontradas = WiFi.scanNetworks();
//   if (redesEncontradas == 0)
//   {
//     Serial.println("no networks found");
//   }
//   else
//   {
//     Serial.print(redesEncontradas);
//     Serial.println(" networks found");
//
//     for (int i = 0; i < redesEncontradas; ++i)
//     {
//       Serial.print(WiFi.SSID(i));
//       Serial.print(" (");
//       Serial.print(WiFi.RSSI(i));
//       Serial.print(")");
//       Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
//       if (WiFi.SSID(i) == ssid)
//       {
//         //WiFi.begin(ssid, pass);
//         //Serial.println("Conectado a red wifi");
//         Serial.println(WiFi.begin(ssid, pass));
//          while(WiFi.status() != WL_CONNECTED)
//          {
//           Serial.println(".");
//           delay(500);
//           if(WiFi.status() == WL_CONNECTED){
//             Serial.println("Wifi conectado correctamente.");
//         ////////////////////////////////socket.io//////////////////////////////////////////////////////
//         // setReconnectInterval to 10s, new from v2.5.1 to avoid flooding server. Default is 0.5s
//         //socketIO.setReconnectInterval(10000);
//         //socketIO.setExtraHeaders("Authorization: 1234567891");
//         // server address, port and URL
//         // void begin(IPAddress host, uint16_t port, String url = "/socket.io/?EIO=4", String protocol = "arduino");
//         // To use default EIO=4 from v2.5.1
//         //socketIO.begin(serverIP, serverPort);
//
//         // event handler
//         //socketIO.onEvent(socketIOEvent);
//         //
//         //  /////////////////////////////////////////////////////////
//         //  /**************************NOMBRES PARA FUNCION DE ACTUALIZACION DE FIRMWARE*******************/
//         mDashBegin(DEVICE_PASSWORD);
//         Serial.println("Se inicio proceso de busqueda de firmware nuevo...");
//         /////////////////////////////////////////////////////////
//           }
//          }
//         break;
//       }
//     }
//   }
// }

// void BuscarWiFi(void *parameter)
//{
//   while (true)
//   {
//     socketIO.loop();
//     if (WiFi.status() != WL_CONNECTED)
//     {
//       //Serial.println("B.W.");
//       //if (contadorIntentaReconexion >= intentaReconexion)
//       //{
//         Serial.println("Buscando WiFi...");
//         reconexionWifi();
//        // contadorIntentaReconexion = 0;
//      // }
//     }
//     ////////////////reiniciar wdt/////////////////////////////
////    if (!socketIOconnect && contadorEstadoSocketIO >= intentaReconexion)
////    {
////      Serial.println("No se pudo conectar con servidor SocketIO.");
////      contadorEstadoSocketIO = 0;
////      esp_task_wdt_reset();
////    }
//    /////////////////////////////////////////////////////////
//  }
//}
void loop()
{

  // configurar_wifi();

  if (SensorGaveta.banderaInterrupt)
  {
    if (!digitalRead(SensorGaveta.PIN))
    {
      SensorGaveta.fechaApertura = rtc.now(); // TIMESTAMP_FULL //!< `YYYY-MM-DDThh:mm:ss`
      SensorGaveta.fechaTimestamp = SensorGaveta.fechaApertura.timestamp();
      SensorGaveta.vecesAbierta++;
      EEPROM.write(direccionGavetaAbierta, SensorGaveta.vecesAbierta);
      EEPROM.commit();
      USB_SERIAL.println(SensorGaveta.fechaTimestamp);
    }
    SensorGaveta.banderaInterrupt = false;
  }
  if (suscrito)
  {
    if (completo.bandera)
    {
      if (completo.lastState)
      {
        completo.saveLastState = completo.lastState;
        completo.saveDebounceTimeout = completo.debounceTimeout;
      }
      else
      {
        if (completo.saveLastState && (millis() - completo.saveDebounceTimeout > DEBOUNCETIME))
        {
          boletoMas(completo);
          completo.saveLastState = completo.lastState;
        }
      }
      completo.bandera = false;
    }

    if (estudiante.bandera)
    {
      if (estudiante.lastState)
      {
        estudiante.saveLastState = estudiante.lastState;
        estudiante.saveDebounceTimeout = estudiante.debounceTimeout;
      }
      else
      {
        if (estudiante.saveLastState && (millis() - estudiante.saveDebounceTimeout > DEBOUNCETIME))
        {
          boletoMas(estudiante);
          estudiante.saveLastState = estudiante.lastState;
        }
      }
      estudiante.bandera = false;
    }

    if (terceraEdad.bandera)
    {
      if (terceraEdad.lastState)
      {
        terceraEdad.saveLastState = terceraEdad.lastState;
        terceraEdad.saveDebounceTimeout = terceraEdad.debounceTimeout;
      }
      else
      {
        if (terceraEdad.saveLastState && (millis() - terceraEdad.saveDebounceTimeout > DEBOUNCETIME))
        {
          boletoMas(terceraEdad);
          terceraEdad.saveLastState = terceraEdad.lastState;
        }
      }
      terceraEdad.bandera = false;
    }

    if (capacidadDiferente.bandera)
    {
      if (capacidadDiferente.lastState)
      {
        capacidadDiferente.saveLastState = capacidadDiferente.lastState;
        capacidadDiferente.saveDebounceTimeout = capacidadDiferente.debounceTimeout;
      }
      else
      {
        if (capacidadDiferente.saveLastState && (millis() - capacidadDiferente.saveDebounceTimeout > DEBOUNCETIME))
        {
          boletoMas(capacidadDiferente);
          capacidadDiferente.saveLastState = capacidadDiferente.lastState;
        }
      }
      capacidadDiferente.bandera = false;
    }

    if (generarCorte.bandera)
    {
      if (generarCorte.lastState)
      {
        generarCorte.saveLastState = generarCorte.lastState;
        generarCorte.saveDebounceTimeout = generarCorte.debounceTimeout;
      }
      else
      {
        if (generarCorte.saveLastState && (millis() - generarCorte.saveDebounceTimeout > DEBOUNCETIME))
        {
          banderaCorte = true;
          generarCorte.saveLastState = generarCorte.lastState;
        }
      }
      generarCorte.bandera = false;
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {

    if (arranquemDash == true)
    {
      //  /////////////////////////////////////////////////////////
      //  /**************************NOMBRES PARA FUNCION DE ACTUALIZACION DE FIRMWARE*******************/
      mDashBegin(DEVICE_PASSWORD);
      Serial.println("Se inicio proceso de busqueda de firmware nuevo...");
      arranquemDash = false;
      /////////////////////////////////////////////////////////
    }

    imprimeRSSI();
    if (contadorPublicaDatos >= publicaDatos)
    {
      if (banderaEnviar)
      {
        USB_SERIAL.println("Enviando solicitud");
        sendConteoRequest();
        banderaEnviar = false;
      }
      contadorPublicaDatos = 0;
    }
  }
  else
  {
    borrarIntensidadWiFi();
    imprimeLogo();
  }
  if (handleHttpCodeFlag)
  {
    USB_SERIAL.println("Solicitud http code response");
    USB_SERIAL.println(responseHttpCode);
    handleHttpCodeFlag = false;
    if (responseHttpCode >= 200 && responseHttpCode < 300)
    {
      DibujaEstadoHTTPRequest(true);
    }
    else
    {
      USB_SERIAL.println("solicitud mala");
      DibujaEstadoHTTPRequest(0);
    }
    banderaEnviar = true;
  }
  if (handleResponseFlag)
  {

    DynamicJsonDocument doc(256);
    deserializeJson(doc, payloadConteo);
    String conteo = doc["conteo"];           // "0"
    String fechaGaveta = doc["fechaGaveta"]; // "0"
    String reset = doc["reset"];             // "0"
    String vigencia = doc["vigencia"];       // "2021-01-31-"
    String RTC = doc["RTC"];                 // "2021-01-08 09:20:20"
    String boleto = doc["boleto"];           // "Azul;Esp;"
    USB_SERIAL.println("Conteo: " + conteo + "fechaGaveta: " + fechaGaveta + "reset: " + reset + "Vigencia: " + vigencia + "RTC: " + RTC + "boleto: " + boleto);
    sincronizarRTC(RTC);
    String axo = encontrarCaracter(vigencia, '-', 0);
    String mes = encontrarCaracter(vigencia, '-', axo.length());
    String dia = encontrarCaracter(vigencia, '-', mes.length() + axo.length());
    dibujaVigencia(axo, mes, dia);
    escribeVigenciaEEPROM(axo, mes, dia);
    if (validarVigencia())
    {
      suscrito = true;
      if (!EEPROM.read(180))
      {
        EEPROM.write(180, 1);
        EEPROM.commit();
      }
    }
    else
    {
      suscrito = false;
      if (EEPROM.read(180))
      {
        EEPROM.write(180, 0);
        EEPROM.commit();
      }
    }
    // edicionBoleto(boleto);
    if (reset == "1")
    {
      resetCuenta();
    }
    else
    {
      USB_SERIAL.println("No reset");
    }
    handleResponseFlag = false;
    banderaEnviar = true;
  }

  if (banderaWDT)
  {
    Serial.println("Resetting WDT...");
    esp_task_wdt_reset();
    banderaWDT = false;
  }

  if (banderaCorte)
  {
    if (obtenerEstado(ESTADO_PAPEL) == 18 || 22)
    {
      ejecutaCorte();
    }
  }
}
