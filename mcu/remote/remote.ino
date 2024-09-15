
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_PN532.h>
#include "dsp.h"

#define RX433_PIN GPIO_NUM_18
#define TX433_PIN GPIO_NUM_19
#define RXIR_PIN GPIO_NUM_22
#define TXIR_PIN GPIO_NUM_23

#define HSPI_MOSI GPIO_NUM_13
#define HSPI_MISO GPIO_NUM_27
#define HSPI_CLK GPIO_NUM_14

#define SDCARD_CS GPIO_NUM_26
#define PN532_CS GPIO_NUM_25

#define SERIAL_SPEED 115200
#define RX_TIMEOUT 15000


enum State
{
  STATE_IDLE = 0,
  STATE_RECEIVE,
  STATE_TRANSMIT,
  STATE_NFC_READ,
  STATE_NFC_WRITE,
  STATE_NFC_EMULATE,
  STATE_COUNT
};


struct Global
{  
  Signal signal;
  Digital digital;
  State state;
  hw_timer_t *timer0;
  uint64_t startus;
  uint32_t txpin;
  uint32_t txindex;
} global;


const char ssid[] = "wifi-name-placeholder";
const char password[] = "wifi-password-placeholder";
const char index_html[] = "index";
WebServer web(80);
SPIClass spi = SPIClass(HSPI);
Adafruit_PN532 nfc(PN532_CS, &spi);


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


void testNFC(void) {
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t currentblock;                     // Counter to keep track of which block we're on
  bool authenticated = false;               // Flag to indicate if the sector is authenticated
  uint8_t data[16];                         // Array to store block data during reads

  // Keyb on NDEF and Mifare Classic should be the same
  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ...
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

      // Now we try to go through all 16 sectors (each having 4 blocks)
      // authenticating each sector, and then dumping the blocks
      for (currentblock = 0; currentblock < 64; currentblock++)
      {
        // Check if this is a new block so that we can reauthenticate
        if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

        // If the sector hasn't been authenticated, do so first
        if (!authenticated)
        {
          // Starting of a new sector ... try to to authenticate
          Serial.print("------------------------Sector ");Serial.print(currentblock/4, DEC);Serial.println("-------------------------");
          if (currentblock == 0)
          {
              // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
              // or 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 for NDEF formatted cards using key a,
              // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
              success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
          }
          else
          {
              // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
              // or 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 for NDEF formatted cards using key a,
              // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
              success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
          }
          if (success)
          {
            authenticated = true;
          }
          else
          {
            Serial.println("Authentication error");
          }
        }
        // If we're still not authenticated just skip the block
        if (!authenticated)
        {
          Serial.print("Block ");Serial.print(currentblock, DEC);Serial.println(" unable to authenticate");
        }
        else
        {
          // Authenticated ... we should be able to read the block now
          // Dump the data into the 'data' array
          success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
          if (success)
          {
            // Read successful
            Serial.print("Block ");Serial.print(currentblock, DEC);
            if (currentblock < 10)
            {
              Serial.print("  ");
            }
            else
            {
              Serial.print(" ");
            }
            // Dump the raw data
            nfc.PrintHexChar(data, 16);
          }
          else
          {
            // Oops ... something happened
            Serial.print("Block ");Serial.print(currentblock, DEC);
            Serial.println(" unable to read this block");
          }
        }
      }
    }
    else
    {
      Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!");
    }
  }
  // Wait a bit before trying again
  // Serial.println("\n\nSend a character to run the mem dumper again!");
  // Serial.flush();
  // while (!Serial.available());
  // while (Serial.available()) {
  // Serial.read();
  // }
  // Serial.flush();
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
  if(global.txindex < global.digital.count)
  {
    uint16_t index = global.txindex >> 3;
    uint16_t bitindex = global.txindex % 8;
    uint8_t bit = (global.digital.data[index] >> (7 - bitindex)) & 0x1;

    digitalWrite(global.txpin, bit ? HIGH : LOW);
    ++global.txindex;
  }
}


void setup()
{
  // Setup pins
  pinMode(RX433_PIN, INPUT);
  pinMode(TX433_PIN, OUTPUT);
  pinMode(RXIR_PIN, INPUT);
  pinMode(TXIR_PIN, OUTPUT);

  delay(100);

  // Initialize Serial
  Serial.begin(SERIAL_SPEED);
  Serial.setTimeout(100);

  // SPI settings
  spi.begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI);

  // Initialize SD Card
  if(SD.begin(SDCARD_CS, spi)) { Serial.println("[Log] SD Card mount successful"); }
  else { Serial.println("[Error] SD Card mount failed"); }
  testSD();

  // Initialize PN532
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

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

  //testNFC();

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
      timerAlarmWrite(global.timer0, global.digital.clockus, true);
      timerAlarmEnable(global.timer0);

      global.txindex = 0;
      global.txpin = TX433_PIN;
      global.state = STATE_TRANSMIT;
    }
    else if(cmd.equals("txIR\n"))
    {
      global.timer0 = timerBegin(0, 80, true);
      timerAttachInterrupt(global.timer0, &txInterrupt, true);
      timerAlarmWrite(global.timer0, global.digital.clockus, true);
      timerAlarmEnable(global.timer0);

      global.txindex = 0;
      global.txpin = TXIR_PIN;
      global.state = STATE_TRANSMIT;
    }
    else if(cmd.equals("rNFC\n"))
    {
      global.state = STATE_NFC_READ;
    }
    else if(cmd.equals("wNFC\n"))
    {

    }
    else if(cmd.equals("eNFC\n"))
    {

    }
  }
}


void stateReceive()
{
  delay(5);

  if(global.signal.count > 0)
  {
    uint64_t timeus = timerReadMicros(global.timer0);

    if(timeus - global.startus > RX_TIMEOUT)
    {
      detachInterrupt(RX433_PIN);
      detachInterrupt(RXIR_PIN);
      timerEnd(global.timer0);
      global.state = STATE_IDLE;

      dspDebugSignal(&global.signal);

      // Analyse signal
      Histogram histogram;
      dspSignalHistogram(&global.signal, &histogram);
      dspDebugHistogram(&histogram);

      uint16_t index = dspHistogramClock(&histogram);

      // Convert signal to digital
      dspSignalDigital(&global.signal, &global.digital, bitDurations[index]);
      dspDebugDigital(&global.digital);
    }
  }
}


void stateTransmit()
{
  delay(5);

  if(global.txindex == global.digital.count)
  {
      timerAlarmDisable(global.timer0);
      timerDetachInterrupt(global.timer0);
      timerEnd(global.timer0);
      global.state = STATE_IDLE;

      dspDebugDigital(&global.digital);
  }
}


void stateNFCRead()
{
  delay(100);  

  bool success;
  uint8_t recv[255];
  uint8_t recvLength = 0;

  success = nfc.inListPassiveTarget();
  if(success)
  {
    Serial.println("[Log] Success enumerating passive smart card");

    uint8_t apdu[] = {0x00, 0xA4, 0x04, 0x00, 0x0e, 0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};
    success = nfc.inDataExchange(apdu, sizeof(apdu), recv, &recvLength);

    if(success)
    {
      Serial.println("[Log] Success sending APDU");
      for(uint8_t i = 0; i < recvLength; ++i)
      {
        Serial.print(recv[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");      
    }
  }  
}


void loop()
{
  //web.handleClient();

  switch(global.state)
  {
    case STATE_IDLE: stateIdle(); break;
    case STATE_RECEIVE: stateReceive(); break;
    case STATE_TRANSMIT: stateTransmit(); break;
    case STATE_NFC_READ: stateNFCRead(); break;
  }
}

