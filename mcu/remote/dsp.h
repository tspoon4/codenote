#pragma once

#define SIGNAL_SIZE 1024
#define DIGITAL_SIZE 1024

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

bool dspSignalDigital(Signal *_input, Digital *_output)
{
  // Estimate clock
  _output->clockus = 0xffff;
  for(uint16_t i = 0; i < _input->count; ++i)
    if(_output->clockus > _input->data[i]) _output->clockus = _input->data[i];

  // TODO: remove hardcode
  _output->clockus = 416;
  //_output->clockus = 600;

  uint8_t bit = 0;
  _output->count = 0;
  memset(_output->data, 0, sizeof(uint8_t) * DIGITAL_SIZE);
  for(uint16_t i = 0; i < _input->count; ++i)
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
  
  return true;
}

bool dspSignalRS232(Signal *input, Digital *output)
{

}

bool dspDigitalSignal(Digital *output, Signal *input)
{

}
