/*
* ============================================================
*  TESTEUR DE TÉLÉCARTES T1G / T2G
*  Arduino Nano
*  Connecteur FIC2
*  Arduino Uno + OLED I2C  
* ============================================================
*/

/* Cablage
Contacts puce numérotés ISO
1 \     / 5
2 --\ /-- 6
3 --/ \-- 7
4 /     \ 8     

Cnx puce     Breadboard        Arduino
    1          Rail +                      fil rouges
    2        b22  e22           D2         fils blancs
    3        b19  e19           D3         fils jaunes 
    4        b14  e14           D4         fils noirs

    5          Rail -
    6          Rail +
    7         b25  e25          D7           fils verts
    8       Non utilisé

DETECT1       b16  e16          D9           fils bleus
DETECT2        Rail -
*/

#include <Adafruit_SSD1306.h>
#include "SSD1306AsciiAvrI2c.h"

#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

#define USE_SERIAL

byte data[64];
int total_units;

// ========== Configuration Arduino ==========
#define PIN_CLK 3
#define PIN_RAZ 4
#define PIN_RW 6
#define PIN_IO 7
#define PIN_DETECT 9

#define DELAY1 50
#define DELAY2 10

void setup() {
  pinMode(PIN_RW, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_RAZ, OUTPUT);
  pinMode(PIN_IO, INPUT);
  pinMode(PIN_DETECT, INPUT_PULLUP);

  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);

#ifdef USE_SERIAL
  Serial.begin(9600);
  while (!Serial)
    ;
#endif
}

void loop() {
  waitCardIn();
  readCard();
  waitCardOut();
}

void waitCardIn() {
  digitalWrite(PIN_RW, LOW);
  digitalWrite(PIN_RAZ, LOW);
  digitalWrite(PIN_CLK, LOW);

  oled.clear();
  oled.set1X();
  oled.println("  TESTEUR TELECARTE  ");
  oled.println("      T1G/T2G");
  oled.println("");
  oled.println("");
  oled.println("  INSEREZ UNE CARTE");
  oled.println("");

  while (digitalRead(PIN_DETECT) == LOW) {
    delay(DELAY1);
  }
}

void waitCardOut() {
  while (digitalRead(PIN_DETECT) == HIGH) {
    delay(DELAY1);
  }
}

void readCard() {
  oled.clear();
  oled.println("    LECTURE CARTE  ");

  if (getContent()) {
    //analyzeContent();
  } else {
    oled.clear();
    oled.println("   ERREUR  LECTURE ");
  }
}
/*
void PrintHex0(byte b) {
  if (b < 0x10) Serial.print("0");
  Serial.print(b, HEX);
}
*/
bool getContent() {
  int i;
  byte b, j, sum;
  byte *ptr;

  for (i = 0; i < 64; i++)
    data[i] = 0;

  digitalWrite(PIN_CLK, 0);

  digitalWrite(PIN_RW, 0);  // Reset Counter
  digitalWrite(PIN_RAZ, 0);
  digitalWrite(PIN_CLK, 1);
  digitalWrite(PIN_CLK, 0);

  digitalWrite(PIN_RAZ, 1);  // Counter++

  for (i = 0; i < 512; i++) {
    data[i >> 3] |= digitalRead(PIN_IO) << (7 - (i & 7));
    digitalWrite(PIN_CLK, 1);
    digitalWrite(PIN_CLK, 0);
  }

  // Dump content
#ifdef USE_SERIAL
  Serial.println("");
  Serial.println("Contenu carte :");
#endif
  oled.clear();
  oled.println("Contenu carte :");

  ptr = data;
  do {
    for (j = 0; j < 4; j++) {
      b = ptr - &data[0];
#ifdef USE_SERIAL
      Serial.print("$");
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(":");
#endif
      oled.print("$");
      if (b < 0x10) oled.print("0");
      oled.print(b, HEX);
      oled.print(":");

      for (i = 0; i < 8; i++) {
        b = *ptr++;
#ifdef USE_SERIAL
        if (b < 0x10) Serial.print("0");
        Serial.print(b, HEX);
#endif
        if (b < 0x10) oled.print("0");
        oled.print(b, HEX);
      }
#ifdef USE_SERIAL
      Serial.println("");
#endif
      oled.println("");
    }
  } while ((data[0] == 0x81) && (ptr != &data[64]));

  return true;
}
