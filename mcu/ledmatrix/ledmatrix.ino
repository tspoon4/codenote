#include <WiFi.h>
#include <WebServer.h>
#include "index.h"
#include "lut.h"
#include "sine.h"
#include "hmap.h"

#define PIN_R1 (GPIO_SEL_4)
#define PIN_G1 (GPIO_SEL_16)
#define PIN_B1 (GPIO_SEL_17)
#define PIN_R2 (GPIO_SEL_5)
#define PIN_G2 (GPIO_SEL_18)
#define PIN_B2 (GPIO_SEL_19)
#define MASK_RGB (PIN_R1 | PIN_G1 | PIN_B1 | PIN_R2 | PIN_G2 | PIN_B2)

#define PIN_CLK (GPIO_SEL_21)
#define PIN_LAT (GPIO_SEL_22)
#define PIN_NOE (GPIO_SEL_23)
#define MASK_ADDR (GPIO_SEL_12 | GPIO_SEL_13 | GPIO_SEL_14 | GPIO_SEL_15)
#define MASK_PIN (MASK_RGB | PIN_CLK | PIN_LAT | PIN_NOE | MASK_ADDR)

const char *ssid = "wifi-name-placeholder";
const char *password = "wifi-password-placeholder";

WebServer server(80);

// LED matrix management
SemaphoreHandle_t mutex;
hw_timer_t *timer = NULL;
uint32_t pwmc = 0;
uint16_t frameBuffer[2][64*32];
uint16_t frameIndex = 0;
uint8_t temp[64*33];
uint8_t seed[8*16];

// Effects state machine
enum Effect { E_BLACK = 0, E_SQUARE, E_CIRCLE, E_PLASMA, E_FIRE, E_BLOB, E_PIXEL, E_VOXEL, E_COUNT };
enum Palette { P_RAINBOW = 0, P_FIRE, P_TEMP, P_EARTH, P_YELLOW, P_CYAN, P_PURPLE, P_CYAN2, P_GRAD, P_MIDDLE, P_HIGH, P_RING, P_COUNT };
uint8_t effect = E_SQUARE;
uint8_t palette = P_RAINBOW;


void handleRoot() { server.send(200, "text/html", index_html); }
void handleOk() { server.send(200, "text/plain", "Ok");}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void vsyncWait()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  frameIndex ^= 1;
  xSemaphoreGive(mutex);
}


void IRAM_ATTR hsyncInterrupt()
{ 
  if(pwmc == 0)
    xSemaphoreTake(mutex, portMAX_DELAY);
  
  // Cache GPIO register
  uint32_t gpio_out = REG_READ(GPIO_OUT_REG);

  int32_t full = pwmc >> 4;
  uint32_t row = pwmc & 0xf;

  uint8_t id = frameIndex ^ 1;
  uint16_t *row1 = frameBuffer[id] + row*64;
  uint16_t *row2 = frameBuffer[id] + row*64 + 16*64;
  
  for(uint32_t i = 0; i < 64; ++i)
  {
    int32_t red, green, blue;
    uint16_t color;

    gpio_out = gpio_out & ~MASK_RGB;

    color = row1[i];
    red = full - ((color >> 8) & 0x0f);
    gpio_out |= (red >> 27) & PIN_R1;
    green = full - ((color >> 4) & 0x0f);
    gpio_out |= (green >> 15) & PIN_G1;
    blue = full - (color & 0x0f);
    gpio_out |= (blue >> 14) & PIN_B1;

    color = row2[i];
    red = full - ((color >> 8) & 0x0f);
    gpio_out |= (red >> 26) & PIN_R2;
    green = full - ((color >> 4) & 0x0f);
    gpio_out |= (green >> 13) & PIN_G2;
    blue = full - (color & 0x0f);
    gpio_out |= (blue >> 12) & PIN_B2;
    
    REG_WRITE(GPIO_OUT_REG, gpio_out);
    
    gpio_out |= PIN_CLK;
    REG_WRITE(GPIO_OUT_REG, gpio_out);
    
    gpio_out &= ~PIN_CLK;
    REG_WRITE(GPIO_OUT_REG, gpio_out);
  }

  gpio_out |= PIN_NOE;
  REG_WRITE(GPIO_OUT_REG, gpio_out);
  
  gpio_out = (gpio_out & ~MASK_ADDR) | (row << 12);
  REG_WRITE(GPIO_OUT_REG, gpio_out);
  
  gpio_out |= PIN_LAT;
  REG_WRITE(GPIO_OUT_REG, gpio_out);
  
  gpio_out &= ~PIN_LAT;
  REG_WRITE(GPIO_OUT_REG, gpio_out);
  
  gpio_out &= ~PIN_NOE;
  REG_WRITE(GPIO_OUT_REG, gpio_out);

  // Update PWM counter
  pwmc = (pwmc + 1) & 0xff;

  if(pwmc == 0)
    xSemaphoreGive(mutex);
}


void setup(void)
{
  // Configure GPIO
  gpio_config_t gpio;
  gpio.pin_bit_mask = MASK_PIN;
  gpio.mode = GPIO_MODE_OUTPUT;
  gpio.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&gpio);
  
  // Initialize serial
  Serial.begin(115200);

  // Initialize wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize LED matrix hsync interrupt
  mutex = xSemaphoreCreateMutex();
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &hsyncInterrupt, true);
  timerAlarmWrite(timer, 63, true); // (0.016ms / 16level / 16rows) = 62.5us
  timerAlarmEnable(timer);

  // Initialize webserver
  server.on("/", handleRoot);
  server.on("/eff/0", [](){ effect = E_BLACK; handleOk(); });
  server.on("/eff/1", [](){ effect = E_SQUARE; handleOk(); });
  server.on("/eff/2", [](){ effect = E_CIRCLE; handleOk(); });
  server.on("/eff/3", [](){ effect = E_PLASMA; handleOk(); });
  server.on("/eff/4", [](){ effect = E_FIRE; handleOk(); });
  server.on("/eff/5", [](){ effect = E_BLOB; handleOk(); });
  server.on("/eff/6", [](){ effect = E_PIXEL; handleOk(); });
  server.on("/eff/7", [](){ effect = E_VOXEL; handleOk(); });
  server.on("/pal/0", [](){ palette = P_RAINBOW; handleOk(); });
  server.on("/pal/1", [](){ palette = P_FIRE; handleOk(); });
  server.on("/pal/2", [](){ palette = P_TEMP; handleOk(); });
  server.on("/pal/3", [](){ palette = P_EARTH; handleOk(); });
  server.on("/pal/4", [](){ palette = P_YELLOW; handleOk(); });
  server.on("/pal/5", [](){ palette = P_CYAN; handleOk(); });
  server.on("/pal/6", [](){ palette = P_PURPLE; handleOk(); });
  server.on("/pal/7", [](){ palette = P_CYAN2; handleOk(); });
  server.on("/pal/8", [](){ palette = P_GRAD; handleOk(); });
  server.on("/pal/9", [](){ palette = P_MIDDLE; handleOk(); });
  server.on("/pal/10", [](){ palette = P_HIGH; handleOk(); });
  server.on("/pal/11", [](){ palette = P_RING; handleOk(); });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  // Initialize effect buffer
  for(uint16_t i = 0; i < 64*33; ++i) temp[i] = 0;

  // Fill the seed buffer
  for(uint8_t i = 0; i < 8*16; ++i) seed[i] = rand() & 0xff;
}

uint8_t offset = 0;
uint16_t offset16 = 0;
const uint16_t *lut = 0;


void black(uint16_t *fb)
{
  for(uint16_t i = 0; i < 64*32; ++i) fb[i] = 0;
}


void squares(uint16_t *fb)
{
  for(uint8_t j = 0; j < 32; ++j)
  {
    for(uint8_t i = 0; i < 64; ++i)
    {
      int8_t x = i - 32;
      int8_t y = j - 16;
      x = (x > 0) ? x : -x;
      y = (y > 0) ? y : -y;
      uint8_t a = x + y;
      uint8_t index = sine[a] + (offset << 2);

      *fb = lut[index];
      ++fb;
    }
  }
  ++offset;
}


void circles(uint16_t *fb)
{
  for(uint8_t j = 0; j < 32; ++j)
  {
    for(uint8_t i = 0; i < 64; ++i)
    {
      uint8_t x = i << 1;
      uint8_t y = j << 1;
      uint8_t index = sine[x] + sine[y] + (offset << 2);

      *fb = lut[index];
      ++fb;
    }
  }
  ++offset;
}


void plasma(uint16_t *fb)
{
  for(uint8_t j = 0; j < 32; ++j)
  {
    for(uint8_t i = 0; i < 64; ++i)
    {
      int8_t x = i - 32;
      int8_t y = j - 16;
      int16_t rot = ((x << 2)*sine[offset] + (y << 2)*sine[(offset+64) & 0xff]) >> 6;
      int16_t sinx = sine[(x<<1) & 0xff];
      int16_t siny = sine[(y<<1) & 0xff];
      uint8_t index = rot + sinx + siny;

      *fb = lut[index];
      ++fb;
    }
  }
  ++offset;
}


void firescreen(uint16_t *fb)
{
  // Seed last row of temperature
  uint8_t *row = temp + 32*64;
  for(uint8_t i = 0; i < 64; ++i)
  {
    row[i] = (rand() & 0xff) > 0x7f ? 0xff : 0;
  }
  
  // Diffuse temperature buffer
  for(uint8_t j = 0; j < 32; ++j)
  {
    uint8_t *row = temp + j*64;    
    for(uint8_t i = 0; i < 64; ++i)
    {
      uint8_t i1 = 64 + ((i - 1) & 0x3f);
      uint8_t i2 = 64 + i;
      uint8_t i3 = 64 + ((i + 1) & 0x3f);
      int16_t t = ((row[i1] + row[i2] + row[i3] + row[i]) >> 2) - 2;
      row[i] = (t > 0) ? t : 0;
      *fb = lut[row[i]];
      ++fb;
    }
  }
}


void blobs(uint16_t *fb)
{
  int8_t x1 = (sine[((offset<<1) + 64) & 0xff] >> 4) + 32;
  int8_t y1 = (sine[offset] >> 4) + 16;
  int8_t x2 = (sine[(offset+64) & 0xff] >> 3) + 32;
  int8_t y2 = -(sine[(offset<<1) & 0xff] >> 5) + 16;
  int8_t x3 = -(sine[((offset<<1) + 64) & 0xff] >> 4) + 32;
  int8_t y3 = (sine[(offset<<1) & 0xff] >> 5) + 16;
  
  for(uint8_t j = 0; j < 32; ++j)
  {
    for(uint8_t i = 0; i < 64; ++i)
    {
      uint16_t d1 = (i-x1)*(i-x1) + (j-y1)*(j-y1);
      uint16_t d2 = (i-x2)*(i-x2) + (j-y2)*(j-y2);
      uint16_t d3 = (i-x3)*(i-x3) + (j-y3)*(j-y3);
      uint16_t q1 = 0x3fff / (d1 + 1);
      uint16_t q2 = 0x3fff / (d2 + 1);
      uint16_t q3 = 0x3fff / (d3 + 1);
      uint16_t index = q1 + q2 + q3;
      index = (index > 0xff) ? 0xff : index;
      *fb = lut[index];      
      ++fb;
    }
  }
  ++offset;
}


void pixels(uint16_t *fb)
{
  // Randomly update a few seeds
  for(uint8_t i = 0; i < 4; ++i)
  {
    uint8_t rnd = rand() & 0x1f;
    seed[rnd] = rand() & 0xff;
  }
  
  // Fill screen
  for(uint8_t j = 0; j < 32; ++j)
  {
    uint8_t y = j >> 3;
    for(uint8_t i = 0; i < 64; ++i)
    {
      uint8_t x = i >> 3;
      uint8_t index = ((seed[x+(y<<3)] >> 2) + (offset16 >> 2)) & 0xff;
      *fb = lut[index];
      ++fb;
    }
  }
  ++offset16;
}


void voxel(uint16_t *fb) 
{
  // Cache sin-cos
  int16_t s1 = sine[offset16 & 0xff];
  int16_t s2 = sine[(offset16>>1) & 0xff];
  int16_t c1 = sine[(offset16+64) & 0xff];
  int16_t c2 = sine[((offset16>>1)+64) & 0xff];
  
  // Animate camera
  int16_t cx = 31 + (s1 >> 1);
  int16_t cy = 111 + (s2 >> 1);
  int16_t x1 = (64*c1) >> 8;
  int16_t y1 = (64*c2) >> 8;
  int16_t x2 = 8*y1;
  int16_t y2 = -8*x1;
  
  // Raycast heightmap
  for(uint8_t i = 0; i < 64; ++i)
  {
    uint8_t hmax = 0;
    uint16_t *pix = fb + 31*64 + i;
    int16_t rx = ((i - 32) * x2) >> 8;
    int16_t ry = ((i - 32) * y2) >> 8;
    
    for(int16_t j = 0; j < 512; j += 1 + (j>>6))
    {
      int16_t vx = ((rx + x1) * j) >> 8;
      int16_t vy = ((ry + y1) * j) >> 8;
      int16_t sx = (cx + vx) & 0xff;
      int16_t sy = (cy + vy) & 0xff;

      // Sample heightmap
      uint8_t hgnd = hmap[sx + (sy << 8)];
      int16_t hproj = 32 + ((hgnd - 256) * 20) / (j + 1);

      // Clamp projection
      hproj = (hproj > 0) ? hproj : 0;
      hproj = (hproj < 32) ? hproj : 32;

      if(hmax < hproj)
      {
        uint16_t rgb = lut[hgnd];
        for(uint8_t k = hmax; k < hproj; ++k) { *pix = rgb; pix -= 64; }
        hmax = hproj;
      }
    }

    // Fill remaining black
    for(uint8_t k = hmax; k < 32; ++k) { *pix = 0; pix -= 64; }
  }
  
  ++offset16;
}


void loop(void)
{
  server.handleClient();

  uint8_t id = frameIndex;
  uint16_t *fb = frameBuffer[id];

  switch(palette)
  {
    case P_RAINBOW: lut = rainbow; break;
    case P_FIRE: lut = fire; break;
    case P_TEMP: lut = temperature; break;
    case P_EARTH: lut = earth; break;
    case P_YELLOW: lut = yellow; break;
    case P_CYAN: lut = cyan; break;
    case P_PURPLE: lut = purple; break;
    case P_CYAN2: lut = cyan2; break;
    case P_GRAD: lut = grad; break;
    case P_MIDDLE: lut = middle; break;
    case P_HIGH: lut = high; break;
    case P_RING: lut = ring; break;
  }
  
  switch(effect)
  {
    case E_BLACK: black(fb); break;
    case E_SQUARE: squares(fb); break;
    case E_CIRCLE: circles(fb); break;
    case E_PLASMA: plasma(fb); break;
    case E_FIRE: firescreen(fb); break;
    case E_BLOB: blobs(fb); break;
    case E_PIXEL: pixels(fb); break;
    case E_VOXEL: voxel(fb); break;
  }

  vsyncWait();
}
