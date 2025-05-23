#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <stdio.h>
#include <string.h>

#define TFT_CS     5
#define TFT_DC     16
#define TFT_RST    17

#define BTN_UP     25
#define BTN_DOWN   26
#define BTN_LEFT   27
#define BTN_RIGHT  14
#define BTN_MODE   13

static Adafruit_ST7735 Tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define RESISTOR_BODY_COLOR 0xA534
const uint16_t COLORS[] = {
  0x0000, // Black
  0xA145, // Brown
  0xF800, // Red
  0xFD20, // Orange
  0xFFE0, // Yellow
  0x07E0, // Green
  0x001F, // Blue
  0x780F, // Purple
  0xC618, // Gray
  0xFFFF, // White
};
const float TOLERANCES[] = {-1, 1.0, 2.0, -1, -1, 0.5, 0.25, 0.1, 0.05, -1};

#define COLOR_COUNT 10
#define MAX_RING_COUNT 5
#define DEFAULT_RING_COUNT 4

struct Context {
  int ring_values[MAX_RING_COUNT];
  int current_ring;
  int ring_count = DEFAULT_RING_COUNT;
  int previous_ring_count = DEFAULT_RING_COUNT;
};

void setup() {
  Serial.begin(115200);

  Tft.initR(INITR_BLACKTAB);
  Tft.setRotation(1);
  Tft.fillScreen(ST77XX_BLACK);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);

  draw_resistor_body();
}

void loop() {
  static Context context = {};

  delay(150);
  if (digitalRead(BTN_LEFT) == LOW) {
    Serial.println("LEFT pushed");
    context.current_ring = (context.current_ring + context.ring_count - 1) % context.ring_count;
  }
  if (digitalRead(BTN_RIGHT) == LOW) {
    Serial.println("RIGHT pushed");
    context.current_ring = (context.current_ring + 1) % context.ring_count;
  }
  if (digitalRead(BTN_UP) == LOW) {
    Serial.println("UP pushed");
    context.ring_values[context.current_ring] = (context.ring_values[context.current_ring] + 1) % COLOR_COUNT;
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    Serial.println("DOWN pushed");
    context.ring_values[context.current_ring] = (context.ring_values[context.current_ring] + COLOR_COUNT - 1) % COLOR_COUNT;
  }
  if (digitalRead(BTN_MODE) == LOW) {
    Serial.println("MODE pushed");
    context.ring_count += (context.ring_count == 4) ? +1 : -1;
    context.current_ring = 0;
    for (int i = 0; i < context.ring_count; ++i)
      context.ring_values[i] = 0;
  }
  drawInterface(&context);
  context.previous_ring_count = context.ring_count;
}

void draw_arrow(int x, int y) {
  Tft.drawLine(x, y - 12, x, y, ST77XX_RED);
  Tft.fillTriangle(x, y, x - 4, y - 6, x + 4, y - 6, ST77XX_RED);
}

void clear_arrow(int x, int y) {
  Tft.drawLine(x, y - 12, x, y, ST77XX_BLACK);
  Tft.fillTriangle(x, y, x - 4, y - 6, x + 4, y - 6, ST77XX_BLACK);
}

void draw_resistor_body() {
#define X 20
#define Y 30
#define WIDTH 120
#define HEIGHT 30
#define RING_WIDTH 6
  Tft.fillRect(X - 10, Y + HEIGHT / 2 - 2, 10, 4, ST77XX_WHITE);
  Tft.fillRect(X + WIDTH, Y + HEIGHT / 2 - 2, 10, 4, ST77XX_WHITE);
  Tft.fillRoundRect(X, Y, WIDTH, HEIGHT, 8, RESISTOR_BODY_COLOR);
}

void draw_resistor_rings(const Context *context) {
#define X 20
#define Y 30
#define WIDTH 120
#define HEIGHT 30
#define RING_WIDTH 6
  int spacing = (WIDTH - RING_WIDTH*context->ring_count)/(context->ring_count + 1);
  int current_ring_position = X + spacing*(context->current_ring + 1) + RING_WIDTH*context->current_ring;

  static int previous_arrow_position = 0;
  int arrow_position = current_ring_position + RING_WIDTH/2;
  if (previous_arrow_position != arrow_position)
    clear_arrow(previous_arrow_position, Y - 2);
  draw_arrow(arrow_position, Y - 2);
  previous_arrow_position = arrow_position;

  static int previous_ring_positions[MAX_RING_COUNT] = {-100, -100, -100, -100, -100};

  if (context->ring_count != context->previous_ring_count)
    for (int i = 0; i < context->previous_ring_count; ++i)
      Tft.fillRect(previous_ring_positions[i], Y + 2, RING_WIDTH, HEIGHT - 4, RESISTOR_BODY_COLOR);
  for (int i = 0; i < context->ring_count; ++i) {
    int ring_position = X + spacing*(i + 1) + RING_WIDTH*i;
    previous_ring_positions[i] = ring_position;
    Tft.fillRect(ring_position, Y + 2, RING_WIDTH, HEIGHT - 4, COLORS[context->ring_values[i]]);
  }
}

void draw_resist_value(const Context *context) {

#define MESSAGE_SIZE 32
  static char previous_message[MESSAGE_SIZE] = "";
  char message[MESSAGE_SIZE] = "";

  int mantissa = 0;
  for (int i = 0; i < context->ring_count - 2; ++i)
    mantissa = 10*mantissa + context->ring_values[i];
  int exponent = context->ring_values[context->ring_count - 2];
  float tolerance = TOLERANCES[context->ring_values[context->ring_count - 1]];

#define BUFFER_SIZE 32
  char buffer[BUFFER_SIZE] = "";
  sprintf(message, "%de", mantissa);
  if (exponent >= 9)
    sprintf(buffer, "%d", exponent - 9);
  else if (exponent >= 6)
    sprintf(buffer, "%d", exponent - 6);
  else if (exponent >= 3)
    sprintf(buffer, "%d", exponent - 3);
  else
    sprintf(buffer, "%d", exponent);
  strcat(message, buffer);
  if (tolerance > 0)
    sprintf(buffer, "+-%.2f%%", tolerance);
  else
    buffer[0] = '\0';
  strcat(message, buffer);
  if (exponent >= 9)
    strcat(message, "GOhm");
  else if (exponent >= 6)
    strcat(message, "MOhm");
  else if (exponent >= 3)
    strcat(message, "kOhm");
  else
    strcat(message, "Ohm");

  Tft.setTextSize(1);
  if (strcmp(message, previous_message)) {
    Tft.setCursor(10, 85);
    Tft.setTextColor(ST77XX_BLACK);
    Tft.print(previous_message);
  }
  Tft.setCursor(10, 85);
  Tft.setTextColor(ST77XX_GREEN);
  Tft.print(message);

  strcpy(previous_message, message);
}

void drawInterface(const Context *context) {
  draw_resistor_rings(context);
  draw_resist_value(context);
}

