/*
* ============================================================
*  TESTEUR DE TÉLÉCARTES T1G / T2G
*  Arduino Nano
*  Connecteur FIC2
*  Arduino Uno + OLED I2C  
* ============================================================
*/

/* Cablage
Cartes de test ISO
FIC2 conn       Breadboard        Arduino
PIN1=VCC          Rail +                         fils rouges
PIN2=RST         b22  e22           D2           fils blancs
PIN3=CLK         b19  e19           D3           fils jaunes
PIN5=VSS          Rail -
PIN7=I/O         b25  e25           D4           fils verts
DETECT1          b16  e16           D5           fils bleus
DETECT2           Rail -
*/

// >>> Code analysis from Grok IA

#include <Adafruit_SSD1306.h>
#include "SSD1306AsciiAvrI2c.h"

#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

#define USE_SERIAL

byte mem[32];  // Buffer de 256 bits (32 octets)

// ========== Configuration Arduino ==========
#define PIN_RST 2
#define PIN_CLK 3
#define PIN_IO 7
#define PIN_DETECT 9

#define DELAY1 50
#define DELAY2 10

void setup() {
  pinMode(PIN_RST, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
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
  digitalWrite(PIN_RST, LOW);
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
    analyzeContent();
  } else {
    oled.clear();
    oled.println("   ERREUR  LECTURE ");
  }
}

// ---------- Lecture bas niveau ----------

void pulseClock() {
  digitalWrite(PIN_CLK, HIGH);
  delayMicroseconds(DELAY2);
  digitalWrite(PIN_CLK, LOW);
  delayMicroseconds(DELAY2);
}

void resetAddress() {
  digitalWrite(PIN_RST, HIGH);
  delayMicroseconds(DELAY2);
  pulseClock();  // Un front d'horloge pendant RST=1 remet le compteur à 0
  digitalWrite(PIN_RST, LOW);
  delayMicroseconds(DELAY2);
}

bool getContent() {
  // Lecture de 256 bits (32 octets)
  resetAddress();

  for (int i = 0; i < 32; i++) {
    byte octet = 0;
    for (int b = 7; b >= 0; b--) {  // MSB first
      pulseClock();
      if (digitalRead(PIN_IO)) {
        octet |= (1 << b);
      }
    }
    mem[i] = octet;
  }

  // Dump content in serial monitor
#ifdef USE_SERIAL
  for (byte i = 0; i < 32; i++) {
    if (mem[i] < 0x10) Serial.print('0');
    Serial.print(mem[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
#endif

  return true;
}

// ---------- Analyse T1G / T2G ----------
void analyzeContent() {
  // from Grok IA
  oled.clear();

  // --- Détection T2G ---
  if (mem[0] == 0x81 && mem[1] == 0x40) {
    // T2G
    oled.println("Type : T2G");

    // Numéro de série (octets 2 à 5 + partie de 6)
    char serie[12];
    sprintf(serie, "%02X%02X%02X%02X", mem[2], mem[3], mem[4], mem[5]);
    oled.print("Serie: ");
    oled.println(serie);

    // Unités initiales
    uint16_t val = (mem[6] << 8) | mem[7];
    int unitesInit = 0;
    if (val == 0x0005 || val == 0x0105) unitesInit = 50;
    else if (val == 0x000C || val == 0x010C) unitesInit = 120;
    else if (val == 0x0001 || val == 0x0101) unitesInit = 5;
    else if (val == 0x0003 || val == 0x0103) unitesInit = 25;
    else unitesInit = val & 0xFF;  // fallback

    oled.print("Init : ");
    oled.print(unitesInit);
    oled.println(" u");

    // Unités restantes (approximation simple sur les compteurs)
    // Les compteurs T2G sont en abaque octal (complexe).
    // On affiche une estimation basée sur les bits restants.
    int restants = estimerRestantsT2G();
    oled.print("Rest : ");
    oled.print(restants);
    oled.println(" u");
  }
  // --- Détection T1G ---
  else {
    oled.println("Type : T1G");

    // Numéro de série approximatif (octets 1-3 + 5-6)
    char serie[14];
    sprintf(serie, "%02X%02X%02X%02X%02X", mem[1], mem[2], mem[3], mem[5], mem[6]);
    oled.print("Serie:");
    oled.println(serie);

    // Unités initiales (octet 11 classique)
    int unitesInit = 0;
    switch (mem[11]) {
      case 0x13: unitesInit = 120; break;
      case 0x06: unitesInit = 50; break;
      case 0x05: unitesInit = 40; break;
      case 0x04: unitesInit = 25; break;
      case 0x02: unitesInit = 5; break;
      default: unitesInit = mem[11];
    }

    oled.print("Init : ");
    oled.print(unitesInit);
    oled.println(" u");

    // Unités restantes = nombre de bits à 0 dans la zone unités (à partir du bit 96)
    int restants = compterBitsZero(12, 31);  // octets 12 à 31
    oled.print("Rest : ");
    oled.print(restants);
    oled.println(" u");
  }
}

// Compte les bits à 0 dans une plage d'octets (T1G)
int compterBitsZero(int debut, int fin) {
  int count = 0;
  for (int i = debut; i <= fin; i++) {
    uint8_t v = mem[i];
    for (int b = 0; b < 8; b++) {
      if ((v & (1 << b)) == 0) count++;
    }
  }
  return count;
}

// Estimation très simplifiée pour T2G (à améliorer selon tes cartes)
int estimerRestantsT2G() {
  // Les compteurs commencent vers l'octet 8
  // On compte grossièrement les bits encore à 1
  int bits = 0;
  for (int i = 8; i <= 12; i++) {
    uint8_t v = mem[i];
    for (int b = 0; b < 8; b++) {
      if (v & (1 << b)) bits++;
    }
  }
  // Approximation très grossière
  return bits * 2;  // à ajuster avec tes vraies cartes
}
