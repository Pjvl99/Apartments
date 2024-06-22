/*************************************************************

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "TMPLhkshwGSC"
#define BLYNK_TEMPLATE_NAME "Contador2"
#define BLYNK_AUTH_TOKEN "fQ8XylYAP4f5zJQxZKayomlS_cxM8R92"

// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial
#include <PZEM004Tv30.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <BlynkSimpleEsp8266.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiClient.h>

const char* serverName = "";
String message = "";
HTTPClient https;

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "";
char pass[] = ""; 
#define MAX_ATTEMPTS 10   // maximos intentos para obtener valor
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES 10  // Number of snowflakes in the animation example

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16
static const unsigned char PROGMEM logo_bmp[] = { B00000000, B11000000,
                                                  B00000001, B11000000,
                                                  B00000001, B11000000,
                                                  B00000011, B11100000,
                                                  B11110011, B11100000,
                                                  B11111110, B11111000,
                                                  B01111110, B11111111,
                                                  B00110011, B10011111,
                                                  B00011111, B11111100,
                                                  B00001101, B01110000,
                                                  B00011011, B10100000,
                                                  B00111111, B11100000,
                                                  B00111111, B11110000,
                                                  B01111100, B11110000,
                                                  B01110000, B01110000,
                                                  B00000000, B00110000 };

PZEM004Tv30 pzem(12, 14);  //rx tx (D5, D6)

String salida = "";
String salidaP = "";
float consumo, tarifa;
unsigned long t1, t2, ta, tr;
const unsigned long tp = 10000;  //lectura cada 10 Seg
boolean con, flag, res, cp1, cp2, blynk_conn;
int pinValue = 0;  //boolean para reset

//variables BLYNK
WidgetLCD lcd(V0);  //lcd en pin virtual 0

BLYNK_WRITE(V2) {            //rutina para leer valor enviado desde APP asignado a V2
  tarifa = param.asFloat();  // assigning incoming value from pin V1 to a variable
}

BLYNK_WRITE(V1) {            //rutina para leer valor enviado desde APP asignado a V1
  pinValue = param.asInt();  // assigning incoming value from pin V1 to a variable
}

BLYNK_CONNECTED() {
  Blynk.syncAll();
}


void setup(){
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  display.setTextSize(2);  // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.

  // Clear the buffer
  display.clearDisplay();
  flag = true;
  con = false;
  res = false;
  cp1 = false;
  cp2 = false;

  WiFi.begin(ssid, pass);
  Blynk.begin(auth, ssid, pass);

  t1 = millis();  //inicializamos contador
}

void loop(){
  checkConnection();
  Blynk.run();
  t2 = millis() - t1;
  if (t2 > tp) {  //lectura cada 20 segundos
    t1 = millis();
    Serial.println("tiempo de leer");
    Serial.println(tarifa);
    String message = "";
    float voltage = pzem.voltage();
    if (!isnan(voltage)) {
      Serial.print("Voltage: ");
      Serial.print(voltage);
      Serial.println("V");
    } else {
      message += " Error reading voltage.";
      voltage = 0;
      Serial.println("Error reading voltage");
    }

    float current = pzem.current();
    if (!isnan(current)) {
      Serial.print("Current: ");
      Serial.print(current);
      Serial.println("A");
    } else {
      current = 0;
      message += " Error reading current.";
      Serial.println("Error reading current");
    }

    float power = pzem.power();
    if (!isnan(power)) {
      Serial.print("Power: ");
      Serial.print(power);
      Serial.println("W");
      Blynk.virtualWrite(V4,power);
    } else {
      message += " Error reading power.";
      power = 0;
      Serial.println("Error reading power");
    }

    float energy = pzem.energy() * 1000;
    if (!isnan(energy)) {
      Serial.print("Energy: ");
      Serial.print(energy, 1);
      Serial.println("Wh");
      Blynk.virtualWrite(V3,energy);
    } else {
      message += " Error reading energy.";
      energy = 0;
      Serial.println("Error reading energy");
    }

    float frequency = pzem.frequency();
    if (!isnan(frequency)) {
      Serial.print("Frequency: ");
      Serial.print(frequency, 1);
      Serial.println("Hz");
    } else {
      message += " Error reading frequency.";
      frequency = 0;
      Serial.println("Error reading frequency");
    }

    float pf = pzem.pf();
    if (!isnan(pf)) {
      Serial.print("PF: ");
      Serial.println(pf);
    } else {
      message += " Error reading power factor.";
      pf = 0;
      Serial.println("Error reading power factor");
    }
    if(WiFi.status() == WL_CONNECTED){
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
      client->setInsecure();
      
      https.begin(*client, serverName);
      https.addHeader("Content-Type", "application/json");
      String httpJson = "{ \"message\": \"" + String(message) + "\", \"voltage\": \"" + String(voltage) + "\", ";
      httpJson += "\"current\": \"" + String(current) + "\", \"power\": \"" + String(power) + "\", \"energy\": \"" + String(energy) + "\", \"frequency\": \"" + String(frequency) + "\", \"pf\": \"" + String(pf) + "\", \"apartment\": \"" + "2" + "\"}";
      int httpResponseCode = https.POST(httpJson);
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = https.getString();
        Serial.println(payload);
        delay(5000);
      }
      https.end();
    }

    if (energy > 1000.0) {
      salida = "KWh";
      energy = energy / 1000;
      consumo = energy * tarifa;
    } else {
      salida = "Wh";
      consumo = (energy * tarifa) / 1000.0;
    }

    if (power > 1000.0) {
      salidaP = "KW";
      power = power / 1000;
    } else {
      salidaP = "W";
    }


    Serial.println("Consumo: " + String(consumo));
    lcd.clear();
    lcd.print(1, 0, String(energy) + salida + "," + String(power) + salidaP);
    lcd.print(1, 1, "Consumo: Q." + String(consumo));
    delay(100);
    display.clearDisplay();
    display.setCursor(5, 0);
    if(con && blynk_conn){
      display.println("Q." + String(consumo));
      display.setCursor(5, 16);
      display.println(String(energy) + salida);
      display.setCursor(5, 32);
      display.println(String(power) + salidaP);
    } else if (!con) {
      display.println("Error WIFI");
      if(flag){
        display.setCursor(5, 32);
        display.println("Conectando");
      }
    } else {
      display.println("Error Blynk");
    }
    display.display();  // Show initial text
    delay(100);
    if(blynk_conn){
      Serial.println("Blynk CONECTADO");
    } else {
      Serial.println("Blynk ERROR EN CONEXION");
    }
  
    Serial.println();
  }

  if (pinValue == 1) {
    //res = true;
    pinValue = 0;
    Blynk.virtualWrite(V1, 0);
    //tr = millis();
    //digitalWrite(relay, 1);
    Serial.println("Se activo reset!");
    pzem.resetEnergy();
  }
}

void checkConnection() {
  bool result;
  blynk_conn = Blynk.connected();
  if (WiFi.status() != WL_CONNECTED) {  // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
    if (!flag) {
      Serial.println("---starting wifi");
      WiFi.begin(ssid, pass);  // connect to the network
    }
    //digitalWrite(indicator,0);
    con = false;
    flag = true;
  } else if (flag) {
    Serial.println("connected");
    Serial.println(WiFi.localIP());
    //digitalWrite(indicator,0);
    flag = false;
    con = true;
  } else if (!blynk_conn && con){
    Serial.println("-------connecting to blynk...");
    result = Blynk.connect();
  }
}