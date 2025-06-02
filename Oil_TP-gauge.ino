#include <SPI.h>
#include <TFT_eSPI.h>
#include "bg.h" // Background image, convert at "https://notisrac.github.io/FileToCArray/" with Settings [Pallete mode: 16bit RGB] [Endianness:Big-endian] [Data type: uint16_t]
#include "Fonts/visitor19pt7b.h" // Font converter at "https://rop.nl/truetype2gfx/"
#include "Fonts/visitor115pt7b.h" // Font converter at "https://rop.nl/truetype2gfx/"
#include "manifold.h" // Warning image, convert at "https://notisrac.github.io/FileToCArray/" with Settings [Pallete mode: 16bit RGB] [Endianness:Big-endian] [Data type: uint16_t]
// Screen init settings
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
// Screen Settings
#define SCREEN_W 160
#define SCREEN_H 80
// Pinout for sensor inputs
#define OIL_TEMP_PIN 29
#define OIL_PRESS_PIN 28
// Pinout Settings and voltage divider for pressure sensor since it's 5V and GPIO only supports 0.5+VDD
#define ADC_REF_VOLTAGE 3.3
#define R2 4560
#define R1 9750            

// Variables for program
float lastOilPress = 0.0;
float lastOilTemp = 0.0;
float peakOilPress = 0.0;
float peakOilTemp = 0.0;
float maxOilPressText = 0.0;
float maxOilTempText = 0.0;
unsigned long peakPressBarTime = 0;
unsigned long peakTempBarTime = 0;
unsigned long lastMaxPressTime = 0;
unsigned long lastMaxTempTime = 0;
unsigned long lastFlashMillis = 0;
unsigned long pauseStartMillis = 0;
const int barWidths[] = {10,9,8,6,5,5,5,4,4,4,4,3,3,3,3,2,2,2};
const int numBars = sizeof(barWidths) / sizeof(barWidths[0]);
const int barHeight = 5;
const int barGap = 1;
enum WarningState { FLASHING, PAUSING };
WarningState warningState = FLASHING;
// Temperature lookup table from Bosch datasheet
const float tempTable[][2] = {
  {-40, 44864}, {-30, 25524}, {-20, 15067}, {-10, 9195},
  {0, 5784}, {10, 3740}, {20, 2480}, {30, 1683},
  {40, 1167}, {50, 824}, {60, 594}, {70, 434.9},
  {80, 323.4}, {90, 244}, {100, 186.6}, {110, 144.5},
  {120, 113.3}, {130, 89.9}, {140, 71.9}, {150, 58.1}
};
const int tableSize = sizeof(tempTable) / sizeof(tempTable[0]);

struct SensorPeakTracker {
  float& lastValue;
  float& peakValue;
  float& maxValue;
  unsigned long& peakTime;
  unsigned long& maxTime;
};

// ---SETTINGS-- Adjust sensor safety Thresholds if needed
const int NUM_FLASH_CYCLES = 4;        // Number of full ON-OFF flashes
const unsigned long FLASH_INTERVAL_MS = 125; // Time per toggle (on/off), so 1 cycle = 250ms
const unsigned long WARNING_PAUSE_MS = 5000; // Pause time after flashing
int flashCount = 0;
bool showYellow = true;
const unsigned long peakBarHoldTime = 15000;
const unsigned long maxValueHoldTime = 60000;
const float MIN_VALID_PRESS_BAR = 1;     // 1.5 bar
const float MAX_VALID_TEMP_C = 105;      // 100°C
const float MIN_VALID_TEMP_C = 20;      // -50°C


void updatePeakTracking(SensorPeakTracker tracker, float currentValue, unsigned long now, float holdTime, float maxHoldTime) {
  if (currentValue > tracker.peakValue) {
    tracker.peakValue = currentValue;
    tracker.peakTime = now;
  }

  if (currentValue > tracker.maxValue) {
    tracker.maxValue = currentValue;
    tracker.maxTime = now;
  }

  if (now - tracker.maxTime > maxHoldTime) {
    tracker.maxValue = currentValue;
    tracker.maxTime = now;
  }

  if (now - tracker.peakTime > holdTime) {
    tracker.peakValue += (currentValue - tracker.peakValue) * 0.1;
  }
}

void drawNormalDisplay(float oilPress, float oilTemp) {
  img.pushImage(0, 0, SCREEN_W, SCREEN_H, bg);
  img.setTextColor(TFT_WHITE);
  img.setTextDatum(TL_DATUM);

  drawBarBlocks(9, 30, lastOilPress, 10.0, peakOilPress, peakOilPress > 0);
  drawBarBlocks(9, 68, lastOilTemp, 150.0, peakOilTemp, peakOilTemp > 0);

  img.setFreeFont(&visitor115pt7b);
  img.drawString(String(oilPress, 1), 114, 7);
  img.drawString(String((int)oilTemp), 114, 45);

  img.setFreeFont(&visitor19pt7b);
  img.drawString(String(maxOilPressText, 1), 40, 0);
  img.drawString(String((int)maxOilTempText), 40, 38);

  img.pushSprite(0, 0);
}

void handleWarningFlash(unsigned long currentMillis) {
  if (currentMillis - lastFlashMillis >= FLASH_INTERVAL_MS) {
    lastFlashMillis = currentMillis;
    showYellow = !showYellow;

    img.pushImage(0, 0, SCREEN_W, SCREEN_H, showYellow ? manifold_y : manifold_r);
    img.pushSprite(0, 0);

    ++flashCount;
    if (flashCount >= NUM_FLASH_CYCLES * 2) { // 2 toggles per cycle
      warningState = PAUSING;
      pauseStartMillis = currentMillis;
      flashCount = 0;  // reset here
    }
  }
}

void handlePauseState(unsigned long currentMillis, float oilPress, float oilTemp) {
  if (currentMillis - pauseStartMillis < WARNING_PAUSE_MS) {
    drawNormalDisplay(oilPress, oilTemp);
  } else {
    warningState = FLASHING;
    lastFlashMillis = currentMillis;
    flashCount = 0;  // reset again, just in case
  }
}

float calculateTempFromResistance(float resistance) {
  // Handle out of range cases
  if (resistance >= tempTable[0][1]) return tempTable[0][0];  // <= -40°C
  if (resistance <= tempTable[tableSize-1][1]) return tempTable[tableSize-1][0];  // >= 150°C
  
  // Linear interpolation between table points
  for (int i = 0; i < tableSize - 1; i++) {
    if (resistance <= tempTable[i][1] && resistance >= tempTable[i+1][1]) {
      float temp1 = tempTable[i][0];
      float res1 = tempTable[i][1];
      float temp2 = tempTable[i+1][0];
      float res2 = tempTable[i+1][1];
      
      // Linear interpolation
      float temp = temp1 + (temp2 - temp1) * (res1 - resistance) / (res1 - res2);
      return temp;
    }
  }
  
  return -999;  // Error value
}

uint16_t blendColor(uint16_t bg, uint16_t fg, float alpha) {
  uint8_t br = (bg >> 11) << 3;
  uint8_t bgc = ((bg >> 5) & 0x3F) << 2;
  uint8_t bb = (bg & 0x1F) << 3;

  uint8_t fr = (fg >> 11) << 3;
  uint8_t fgf = ((fg >> 5) & 0x3F) << 2;
  uint8_t fb = (fg & 0x1F) << 3;

  uint8_t r = br * (1 - alpha) + fr * alpha;
  uint8_t g = bgc * (1 - alpha) + fgf * alpha;
  uint8_t b = bb * (1 - alpha) + fb * alpha;

  return tft.color565(r, g, b);
}

void drawBarBlocks(int x, int y, float value, float maxVal, float peakVal, bool showPeak) {
  float totalWidth = 0;
  for (int i = 0; i < numBars; i++) {
    totalWidth += barWidths[i];
    if (i < numBars - 1) totalWidth += barGap;
  }

  float valPerPixel = maxVal / totalWidth;
  float valuePixels = value / valPerPixel;
  float peakPixels = peakVal / valPerPixel;

  int currentX = x;

  for (int i = 0; i < numBars; i++) {
    int w = barWidths[i];
    int lit = (valuePixels >= (currentX + w - x)) ? 1 : 0;
    int peak = (showPeak && peakPixels >= (currentX - x) && peakPixels <= (currentX - x + w)) ? 1 : 0;

    for (int px = 0; px < w; px++) {
      for (int py = 0; py < barHeight; py++) {
        int drawX = currentX + px;
        int drawY = y + py;
        uint16_t bgColor = img.readPixel(drawX, drawY);
        if (lit) {
          uint16_t glow = tft.color565(180, 220, 255);
          img.drawPixel(drawX, drawY, blendColor(bgColor, glow, 0.7));
        }
      }
    }

    if (peak) {
      img.drawPixel(currentX, y, TFT_WHITE);
      img.drawPixel(currentX + w - 1, y, TFT_WHITE);
      img.drawPixel(currentX, y + barHeight - 1, TFT_WHITE);
      img.drawPixel(currentX + w - 1, y + barHeight - 1, TFT_WHITE);
      for (int px = 1; px < w - 1; px++) {
        img.drawPixel(currentX + px, y, TFT_WHITE);
        img.drawPixel(currentX + px, y + barHeight - 1, TFT_WHITE);
      }
      for (int py = 1; py < barHeight - 1; py++) {
        img.drawPixel(currentX, y + py, TFT_WHITE);
        img.drawPixel(currentX + w - 1, y + py, TFT_WHITE);
      }
    }

    currentX += w + barGap;
  }
}

void setup() {
  Serial.begin(9600);  // For debugging
  tft.init();
  tft.setRotation(1);
  img.createSprite(SCREEN_W, SCREEN_H);
  img.setTextDatum(TL_DATUM);
}

void loop() {
  unsigned long currentMillis = millis();

  int rawPress = analogRead(OIL_PRESS_PIN);
  int rawTemp = analogRead(OIL_TEMP_PIN);

  // PRESSURE Conversion (Bosch PST-F 1)
  float pressVoltageADC = rawPress * (ADC_REF_VOLTAGE / 1023.0);  // Read actual voltage seen by ADC
  float pressVoltage = pressVoltageADC * ((R1 + R2) / R2);        // Reverse voltage divider
  
  // Datasheet: 0.5V = 0 bar, 4.5V = 10 bar, 400mV/bar sensitivity
  float oilPress = (pressVoltage - 0.5) * (10.0 / 4.0);  // Convert to pressure
  oilPress = constrain(oilPress, 0.0, 10.0);             // Clamp to 0–10 bar range

  // TEMPERATURE Conversion
  // Using 4.6kΩ pull-up resistor as per datasheet recommendation
  const float pullUpRes = 4600.0;  
  float tempVoltage  = rawTemp  * (ADC_REF_VOLTAGE / 1023.0);
  
  // Avoid division by zero
  if (tempVoltage >= 4.99) tempVoltage = 4.98;
  
  float ntcResistance = (pullUpRes * tempVoltage) / (5.0 - tempVoltage);
  float oilTemp = calculateTempFromResistance(ntcResistance);

  // Debug output
  /*
  Serial.print("Raw Press: "); Serial.print(rawPress);
  Serial.print(" Raw Temp: "); Serial.print(rawTemp);
  Serial.print(" Press V: "); Serial.print(pressVoltage, 3);
  Serial.print(" Temp V: "); Serial.print(tempVoltage, 3);
  Serial.print(" NTC Ohms: "); Serial.print(ntcResistance, 1);
  Serial.print(" Oil Press: "); Serial.print(oilPress, 2);
  Serial.print(" Oil Temp: "); Serial.println(oilTemp, 1);
  */
  
  // Apply smoothing filter
  lastOilPress += (oilPress - lastOilPress) * 0.1;
  lastOilTemp += (oilTemp - lastOilTemp) * 0.1;

  updatePeakTracking(
    { lastOilPress, peakOilPress, maxOilPressText, peakPressBarTime, lastMaxPressTime },
    lastOilPress,
    currentMillis,
    peakBarHoldTime,
    maxValueHoldTime
  );

  updatePeakTracking(
    { lastOilTemp, peakOilTemp, maxOilTempText, peakTempBarTime, lastMaxTempTime },
    lastOilTemp,
    currentMillis,
    peakBarHoldTime,
    maxValueHoldTime
  );

  // Sensor validation - adjust these thresholds as needed at the start of code
  bool sensorsNormal = (oilPress >= MIN_VALID_PRESS_BAR && oilTemp <= MAX_VALID_TEMP_C && oilTemp > MIN_VALID_TEMP_C);

  if (sensorsNormal && warningState == FLASHING) {
    drawNormalDisplay(oilPress, oilTemp);
    flashCount = 0;
    warningState = FLASHING;
    delay(50);
  } 
  else {
    if (warningState == FLASHING) {
      handleWarningFlash(currentMillis);
    } else if (warningState == PAUSING) {
      handlePauseState(currentMillis, oilPress, oilTemp);
    }
    delay(10);
  }

}