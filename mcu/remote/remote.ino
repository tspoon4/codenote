#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include "dsp.h"

#define RX433_PIN GPIO_NUM_2
#define TX433_PIN GPIO_NUM_4
#define RXIR_PIN GPIO_NUM_5
#define TXIR_PIN GPIO_NUM_18

#define HSPI_MOSI GPIO_NUM_13
#define HSPI_MISO GPIO_NUM_12
#define HSPI_CLK GPIO_NUM_14

#define SDCARD_CS GPIO_NUM_15
#define PN532_CS GPIO_NUM_27

#define SERIAL_SPEED 115200


enum State
{
  STATE_IDLE = 0,
  STATE_RECEIVE,
  STATE_TRANSMIT,
  STATE_COUNT
};


struct Global
{  
  Signal signal;
  State state;
  hw_timer_t *timer0;
  uint64_t startus;
  uint32_t txpin;
  uint32_t bit;
} global;


const char ssid[] = "wifi-name-placeholder";
const char password[] = "wifi-password-placeholder";
const char index_html[] = "index";
WebServer web(80);


void handleRoot() { web.send(200, "text/html", index_html); }
void handleOk() { web.send(200, "text/plain", "Ok");}
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += web.uri();
  message += "\nMethod: ";
  message += (web.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += web.args();
  message += "\n";
  for (uint8_t i = 0; i < web.args(); i++) {
    message += " " + web.argName(i) + ": " + web.arg(i) + "\n";
  }
  web.send(404, "text/plain", message);
}


void debugHistogram(Histogram *_histogram)
{
  for(uint16_t i = 0; i < _histogram->count; ++i)
  {
    Serial.print(bitDurations[i]);
    Serial.print(" : ");
    Serial.print(_histogram->buckets[i].count);
    Serial.print(", ");
    Serial.print(_histogram->buckets[i].min);
    Serial.print(", ");
    Serial.print(_histogram->buckets[i].max);
    Serial.println("");
  }
}


void debugSignal(Signal *_signal)
{
  for(uint16_t i = 0; i < _signal->count; ++i)
  {
    Serial.println(_signal->data[i]);
  }
}


void debugDigital(Digital *_digital)
{
  for(uint16_t i = 0; i < (_digital->count >> 3) + 1; ++i)
    Serial.print(_digital->data[i], HEX);
  Serial.println("");
}


void testSD()
{
  const char path[] = "/data.txt";
  Serial.println("[Log] Running tests on SD card");

  // Test writing file
  {
    File file = SD.open(path, FILE_WRITE);
    if(file)
    {
      const char buf[] = "toto";
      file.write((uint8_t *) buf, sizeof(buf));
      file.close();
    }
    else { Serial.println("[Error] Failed to open file"); }    
  }

  // Test reading file
  {
    File file = SD.open(path, FILE_READ);
    if(file)
    {
      char buf[5];
      file.read((uint8_t *)buf, 5);
      Serial.print("[Log] String read from SD card : ");
      Serial.println(buf);
      file.close();
    }
    else { Serial.println("[Error] Failed to open file"); }    
  }

  // Test removing file
  int ret = SD.remove(path);
  if(ret != 1) { Serial.println("[Error] File removal failed"); }
}


void IRAM_ATTR rxInterrupt()
{
  uint64_t timeus = timerReadMicros(global.timer0);

  if(global.signal.count == 0)
    global.startus = timeus;

  global.signal.data[global.signal.count++] = timeus - global.startus;
  global.startus = timeus;

  if(global.signal.count > 1023) global.signal.count = 1023;
}


void IRAM_ATTR txInterrupt()
{
  digitalWrite(global.txpin, global.bit ? HIGH : LOW);
  global.bit ^= 1;
}


void setup()
{
  // Initialize Serial
  Serial.begin(SERIAL_SPEED);
  Serial.setTimeout(100);

  // SPI settings
  SPIClass spi = SPIClass(HSPI);
  spi.begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI);

  // Initialize SD Card
  if(SD.begin(SDCARD_CS, spi)) { Serial.println("[Log] SD Card mount successful"); }
  else { Serial.println("[Error] SD Card mount failed"); }
  //testSD();

  pinMode(RX433_PIN, INPUT);
  pinMode(TX433_PIN, OUTPUT);
  pinMode(RXIR_PIN, INPUT);
  pinMode(TXIR_PIN, OUTPUT);

  global.state = STATE_IDLE;

  // Initialize wifi
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // Serial.println("");
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // Serial.print("Connected to ");
  // Serial.println(ssid);
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  //server.on("/", handleRoot);
  //server.on("/eff/0", [](){ effect = E_BLACK; handleOk(); });
}


void stateIdle()
{
  delay(100);

  if(Serial.available())
  {
    String cmd = Serial.readString();
    if(cmd.equals("rx433\n"))
    {
      global.signal.count = 0;
      global.timer0 = timerBegin(0, 80, true);
      attachInterrupt(RX433_PIN, &rxInterrupt, CHANGE);

      global.state = STATE_RECEIVE;
    }
    else if(cmd.equals("rxIR\n"))
    {
      global.signal.count = 0;
      global.timer0 = timerBegin(0, 80, true);
      attachInterrupt(RXIR_PIN, &rxInterrupt, CHANGE);

      global.state = STATE_RECEIVE;
    }
    else if(cmd.equals("tx433\n"))
    {
      global.timer0 = timerBegin(0, 80, true);
      timerAttachInterrupt(global.timer0, &txInterrupt, true);
      timerAlarmWrite(global.timer0, 1000, true);
      timerAlarmEnable(global.timer0);

      global.bit = 0;
      global.txpin = TX433_PIN;
      global.state = STATE_TRANSMIT;
    }
    else if(cmd.equals("txIR\n"))
    {
      global.timer0 = timerBegin(0, 80, true);
      timerAttachInterrupt(global.timer0, &txInterrupt, true);
      timerAlarmWrite(global.timer0, 1000, true);
      timerAlarmEnable(global.timer0);

      global.bit = 0;
      global.txpin = TXIR_PIN;
      global.state = STATE_TRANSMIT;
    }
  }
}


void stateReceive()
{
  delay(5);

  if(global.signal.count > 0)
  {
    uint64_t timeus = timerReadMicros(global.timer0);

    if(timeus - global.startus > 15000)
    {
      detachInterrupt(RX433_PIN);
      detachInterrupt(RXIR_PIN);
      timerEnd(global.timer0);
      global.state = STATE_IDLE;

      debugSignal(&global.signal);

      // Analyse signal
      Histogram histogram;
      dspSignalHistogram(&global.signal, &histogram);
      debugHistogram(&histogram);

      // Convert signal to digital
      Digital digital;
      dspSignalDigital(&global.signal, &digital, 556);
      debugDigital(&digital);
    }
  }
}


void stateTransmit()
{
  delay(5);
}


void loop()
{
  //web.handleClient();

  switch(global.state)
  {
    case STATE_IDLE: stateIdle(); break;
    case STATE_RECEIVE: stateReceive(); break;
    case STATE_TRANSMIT: stateTransmit(); break;
  }
}

