/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

#include <avr/wdt.h>
//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio



#define ZAP  500 // 49.5V  70%
#define VYP  300 // 46.0V  20%
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

uint16_t analogPrepeti = 0;
uint16_t analogProud = 0;
bool stavPinu = true;

void setup() {
  wdt_enable(WDTO_500MS);
  pinMode(1, OUTPUT);
}

void loop() {
  analogPrepeti = analogRead(A2);
  analogProud = analogRead(A1);

  stavPinu = VoltageControl(analogPrepeti) | CurrentProtection(analogProud);

  digitalWrite(1, stavPinu);

  wdt_reset();
}

bool VoltageControl(uint16_t analog) {
  static bool prepetovaOchranaAktivni = false; // Stavová proměnná pro přepěťovou ochranu
  static bool out = true;

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

enum CurrentProtectionState_t {
  STATE_START,
  STATE_NABEH,
  STATE_NORMAL,
  STATE_CEKA,
  STATE_OBNOVA,
  STATE_PAUZA,
  STATE_OBNOVA2,
  STATE_CHYBA,
  STATE_COUNT
};
CurrentProtectionState_t stateNow = STATE_START;
CurrentProtectionState_t statePrev = STATE_COUNT;

uint64_t timeCounter = 0;

bool CurrentProtection(uint16_t analog) {
  if (stateNow != statePrev) {
    switch (statePrev) {
    case STATE_NORMAL:
      timeCounter = millis();
      break;
      
    case STATE_CEKA:
      break;
      
    case STATE_OBNOVA:
      if (stateNow == STATE_CEKA) {
        
      }
      else if (stateNow == STATE_NORMAL) {
        
      }
      break;
      
    case STATE_PAUZA:
      break;

    default:
      break;
    }
    statePrev = stateNow;
  }

  switch (stateNow) {
  case STATE_START:
    break;

  case STATE_NABEH:
    break;
    
  case STATE_NORMAL:
    break;
    
  case STATE_CEKA:
    break;
    
  case STATE_OBNOVA:
    break;
    
  case STATE_PAUZA:
    break;
    
  case STATE_OBNOVA2:
    break;
    
  case STATE_CHYBA:
    break;

  default:
    delay(10000); // only way to get here is a glitch, reset the chip
    break;
  }

  return false;
}
