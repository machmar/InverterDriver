#include <avr/wdt.h>

#define ZAP  500 // TESTOVACI HODNOTA JE POTREBA NASTAVIT
#define VYP  300 // TESTOVACI HODNOTA JE POTREBA NASTAVIT
#define MaxVyp 800 // TESTOVACI HODNOTA JE POTREBA NASTAVIT
#define MaxZap 700 // TESTOVACI HODNOTA JE POTREBA NASTAVIT

#define VYSTUPNI_NAPETI_HRANICE 512 // 2.5V
#define VYSTUPNI_NAPETI_HYSTEREZE 50 // 250mV

#if (VYP) >= (ZAP)
#error VYP musi byt mensi nez ZAP
#endif

#if (MaxZap) <= (ZAP) || (MaxVyp) <= (MaxZap)
#error MaxZap musi byt vetsi nez ZAP a MaxVyp musi byt vetsi nez MaxZap
#endif

bool VoltageControl(uint16_t analog);
bool outControl(uint16_t analog);
void inline OutControlReset();

enum outControlState_t {
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

uint16_t analogPrepeti = 0;
uint16_t analogVystupniNapeti = 0;
uint64_t timeCounter = 0;

void setup() {
  wdt_enable(WDTO_500MS);
  pinMode(1, OUTPUT);

  TCCR0A = 0b10; // CTC mode
  TCCR0B = 0b10; // div clock by 8
  OCR0A = 125;   // every 125 triggers interrupt (1ms)
  TIMSK = 1 << 4;// enable the interrupt
}

void loop() {
  analogPrepeti = analogRead(A2);
  analogVystupniNapeti = analogRead(A1);

  bool stavNapeti = VoltageControl(analogPrepeti);
  if (stavNapeti) OutControlReset();
  bool stavVystupnihoNapeti = outControl(analogVystupniNapeti);

  digitalWrite(1, stavNapeti | stavVystupnihoNapeti);

  wdt_reset();
}

bool VoltageControl(uint16_t analog) {
  static bool prepetovaOchranaAktivni = false; // Stavová proměnná pro přepěťovou ochranu
  static bool out = true;

  // Kontrola přepěťové ochrany
  if (analog > (MaxVyp)) {
    prepetovaOchranaAktivni = true;
  } else if (analog < (MaxZap)) {
    prepetovaOchranaAktivni = false;
  }

  // Zápis výstupu podle přepěťové ochrany nebo základních hodnot
  if (prepetovaOchranaAktivni) {
    out = true;
  } else {
    if (analog < (VYP)) {
      out = true;
    } else if (analog > (ZAP)) {
      out = false;
    }
  }

  return out;
}

static outControlState_t stateNow = STATE_START;

bool outControl(uint16_t analog) {
  static uint8_t resetAccumulator = 0;
  static bool out = true;

  switch (stateNow) {
  case STATE_START:
    resetAccumulator = 0;
    timeCounter = 0;
    out = true;

    stateNow = STATE_NABEH;
    break;

  case STATE_NABEH:
    out = false;

    if (timeCounter > 3000) { // magic number is 3s
      stateNow = STATE_NORMAL;
    }
    break;
    
  case STATE_NORMAL:
    timeCounter = 0;
    out = false;

    if (analog < (VYSTUPNI_NAPETI_HRANICE) - (VYSTUPNI_NAPETI_HYSTEREZE)) {
      stateNow = STATE_CEKA;
    }
    break;
    
  case STATE_CEKA:
    out = true;

    if (timeCounter > 6000) { // magic number is 6s
      stateNow = STATE_OBNOVA;
      timeCounter = 0;
    }
    break;
    
  case STATE_OBNOVA:
    out = false;

    if (resetAccumulator > 2) {
      stateNow = STATE_PAUZA;
    }
    else if (timeCounter > 3000) { // magic number is 3s
      stateNow = STATE_CEKA;
      timeCounter = 0;
      resetAccumulator += 1;
    }
    else if (analog > (VYSTUPNI_NAPETI_HRANICE) + (VYSTUPNI_NAPETI_HYSTEREZE)) {
      stateNow = STATE_NORMAL;
      resetAccumulator += 1;
    }
    break;
    
  case STATE_PAUZA:
    out = true;

    if (timeCounter > 600000) { // magic number is 10 minutes
      stateNow = STATE_OBNOVA2;
      timeCounter = 0;
      resetAccumulator = 0;
    }
    break;
    
  case STATE_OBNOVA2:
    out = false;

    if (timeCounter > 3000) { // magic number is 3 seconds
      stateNow = STATE_CHYBA;
    }
    else if (analog > (VYSTUPNI_NAPETI_HRANICE) + (VYSTUPNI_NAPETI_HYSTEREZE)) {
      stateNow = STATE_NORMAL;
    }
    break;
    
  case STATE_CHYBA:
    out = true;
    // only way to get out of here is to reset the chip
    // I also think it would be good to add some signaling that this occured
    break;

  default:
    digitalWrite(1, true); // turn off the inverter before rerstart
    delay(10000); // only way to get here is a glitch, reset the chip
    break;
  }

  return out;
}

void inline OutControlReset() {
  stateNow = STATE_START;
}

ISR(TIM0_COMPA_vect) { // add one each milliseconds (not using millis as they are only 32 bit)
  timeCounter++;
}
