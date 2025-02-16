/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

#include <avr/wdt.h>
//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio



#define ZAP  500 // 49.5V  70%
#define VYP  400 // 46.0V  20%
#define MaxVyp 800 // 540 Přepěťová ochrana aktivace  
#define MaxZap 700 // 520 Přepěťová ochrana deaktivace 

#if VYP >= ZAP
#error VYP musi byt mensi nez ZAP
#endif

#if MaxZap <= ZAP || MaxVyp <= MaxZap
#error MaxZap musi byt vetsi nez ZAP a MaxVyp musi byt vetsi nez MaxZap
#endif

bool VoltageControl(uint16_t analog);
bool CurrentProtection(uint16_t analog);

void setup() {
  wdt_enable(WDTO_500MS);
  pinMode(1, OUTPUT);
}

uint16_t analogPrepeti = 0;
uint16_t analogOchrana = 0;
bool stavPinu = false;

void loop() {
  analogPrepeti = analogRead(A2);
  analogOchrana = analogRead(A1);

  stavPinu = VoltageControl(analogPrepeti);

  digitalWrite(1, stavPinu);

  wdt_reset();
}

bool prepetovaOchranaAktivni = false; // Stavová proměnná pro přepěťovou ochranu
bool VoltageControl(uint16_t analog) {
  bool out = true;

  // Kontrola přepěťové ochrany
  if (analog > MaxVyp) {
    prepetovaOchranaAktivni = true;
  } else if (analog < MaxZap) {
    prepetovaOchranaAktivni = false;
  }

  // Zápis výstupu podle přepěťové ochrany nebo základních hodnot
  if (prepetovaOchranaAktivni) {
    out = true;
  } else {
    if (analog < VYP) {
      out = true;
    } else if (analog > ZAP) {
      out = false;
    }
  }

  return out;
}

bool CurrentProtection(uint16_t analog) {
  
}
