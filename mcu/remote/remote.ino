#include "dsp.h"

#define RX433_PIN GPIO_NUM_2
#define TX433_PIN GPIO_NUM_4
#define RXIR_PIN GPIO_NUM_5
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
} global;


void IRAM_ATTR rxInterrupt()
{
  uint64_t timeus = timerReadMicros(global.timer0);

  if(global.signal.count == 0)
    global.startus = timeus;

  global.signal.data[global.signal.count++] = timeus - global.startus;
  global.startus = timeus;

  if(global.signal.count > 1023) global.signal.count = 1023;
}


void setup()
{
  Serial.begin(SERIAL_SPEED);
  Serial.setTimeout(100);

  pinMode(RX433_PIN, INPUT);
  pinMode(TX433_PIN, OUTPUT);
  pinMode(RXIR_PIN, INPUT);

  global.state = STATE_IDLE;
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

      // Analyse signal
      Digital digital;
      dspSignalDigital(&global.signal, &digital);

      // Print message information
      Serial.print(global.signal.count);
      Serial.print("s, ");
      Serial.print(digital.count);
      Serial.print("d, ");
      Serial.print(digital.clockus);
      Serial.print("us, ");

      // Print message
      for(uint16_t i = 0; i < (digital.count >> 3) + 1; ++i)
        Serial.print(digital.data[i], HEX);
      Serial.println("");      
    }
  }
}


void loop()
{
  switch(global.state)
  {
    case STATE_IDLE: stateIdle(); break;
    case STATE_RECEIVE: stateReceive(); break;
  }
}

