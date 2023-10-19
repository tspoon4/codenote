#pragma once

const uint16_t bitDurations[] =
{
  9,    // 115200 bauds 	115200 bits/s 	8.681 µs
  13,   // 76800 bauds 	76800 bits/s 	13.021 µs
  17,   // 57600 bauds 	57600 bits/s 	17.361 µs
  26,   // 38400 bauds 	38400 bits/s 	26.042 µs
  35,   // 28800 bauds 	28800 bits/s 	34.722 µs
  52,   // 19200 bauds 	19200 bits/s 	52.083 µs
  104,  // 9600 bauds 	9600 bits/s 	104.167 µs
  208,  // 4800 bauds 	4800 bits/s 	208.333 µs
  417,  // 2400 bauds 	2400 bits/s 	416.667 µs
  556,  // 1800 bauds 	1800 bits/s 	555.556 µs
  833,  // 1200 bauds 	1200 bits/s 	833.333 µs
  1667, // 600 bauds 	600 bits/s 	1.667 ms
  3333, // 300 bauds 	300 bits/s 	3.333 ms
  5000, // 200 bauds 	200 bits/s 	5.000 ms
  6667, // 150 bauds 	150 bits/s 	6.667 ms
  7463, // 134 bauds 	134 bits/s 	7.463 ms
  9091, // 110 bauds 	110 bits/s 	9.091 ms
};

#define SIGNAL_SIZE 1024
#define DIGITAL_SIZE 1024
#define BUCKET_SIZE (sizeof(bitDurations) / sizeof(uint16_t))

struct Signal
{
  uint16_t data[SIGNAL_SIZE];
  uint16_t count;
};

struct Digital
{
  uint8_t data[DIGITAL_SIZE];
  uint16_t count;
  uint16_t clockus;
};

struct Bucket
{
  uint64_t sum;
  uint16_t count;
  uint16_t min;
  uint16_t max;
  uint16_t pad;
};

struct Histogram
{
  Bucket buckets[BUCKET_SIZE];
  uint16_t count;
};

static uint16_t dspFindBucket(Histogram *_histogram, uint16_t _value)
{
  for(uint16_t i = 0; i < _histogram->count; ++i)
  {
    uint16_t min = 0;
    uint16_t max = 0xffff;

    if(i > 0) min = (bitDurations[i] + bitDurations[i-1]) >> 1;
    if(i < BUCKET_SIZE-1) max = (bitDurations[i+1] + bitDurations[i]) >> 1;

    if(_value > min && _value <= max) return i;
  }

  return 0xffff;
}

bool dspSignalHistogram(Signal *_input, Histogram *_histogram)
{
  // Initialize histogram
  _histogram->count = BUCKET_SIZE;
  for(uint16_t i = 0; i < _histogram->count; ++i)
  {
    _histogram->buckets[i] = {0, 0, 0xffff, 0, 0};
  }

  // Fill histogram
  for(uint16_t i = 0; i < _input->count; ++i)
  {
    uint16_t value = _input->data[i];
    uint16_t index = dspFindBucket(_histogram, value);
    if(index != 0xffff)
    {
      Bucket *bucket = _histogram->buckets + index;

      if(bucket->max < value) bucket->max = value;
      if(bucket->min > value) bucket->min = value;
      bucket->sum += value;
      ++bucket->count;
    }
  }

  return true;
}

uint16_t dspHistogramClock(const Histogram *_histogram)
{  
  uint16_t count = 0;
  uint16_t index = 0xffff;

  for(uint16_t i = 0; i < _histogram->count; ++i)
  {
    if(_histogram->buckets[i].count < count) { index = i - 1; break; }
    else count = _histogram->buckets[i].count;
  }

  return index;
}

bool dspSignalDigital(Signal *_input, Digital *_output, uint16_t _clockus)
{  
  uint8_t bit = 0;
  _output->count = 0;
  _output->clockus = _clockus;
  memset(_output->data, 0, sizeof(uint8_t) * DIGITAL_SIZE);

  for(uint16_t i = 0; i < _input->count; ++i)
  {
    if(_input->data[i] > 0)
    {
      bit ^= 0x1;
      float bitcount = (float)_input->data[i] / _output->clockus;
      uint16_t count = (uint16_t) round(bitcount);

      for(uint16_t j = 0; j < count; ++j)
      {
        uint16_t index = _output->count >> 3;
        uint16_t bitindex = _output->count % 8;
        _output->data[index] |= bit << (7 - bitindex);
        _output->count++;
      }
    }
  }

  // Add a trailing 0
  _output->count++;
  
  return true;
}

bool dspSignalRS232(Signal *input, Digital *output)
{

}

bool dspDigitalSignal(Digital *output, Signal *input)
{

}
