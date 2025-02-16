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

  TCCR0A = 0b10;  // CTC mod
  TCCR0B = 0b10;  // deleni hodin osmi
  OCR0A = 125;    // kazdych 125 poctu zavola interrupt (1ms)
  TIMSK = 1 << 4; // povolit interrupt
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

    if (timeCounter > 3000) { // magicky cislo 3s
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

    if (timeCounter > 6000) { // magicky cislo 6s
      stateNow = STATE_OBNOVA;
      timeCounter = 0;
    }
    break;
    
  case STATE_OBNOVA:
    out = false;

    if (resetAccumulator > 2) {
      stateNow = STATE_PAUZA;
    }
    else if (timeCounter > 3000) { // magicky cislo 3s
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

    if (timeCounter > 600000) { // magicky cislo 10minut
      stateNow = STATE_OBNOVA2;
      timeCounter = 0;
      resetAccumulator = 0;
    }
    break;
    
  case STATE_OBNOVA2:
    out = false;

    if (timeCounter > 3000) { // magicky cislo 3s
      stateNow = STATE_CHYBA;
    }
    else if (analog > (VYSTUPNI_NAPETI_HRANICE) + (VYSTUPNI_NAPETI_HYSTEREZE)) {
      stateNow = STATE_NORMAL;
    }
    break;
    
  case STATE_CHYBA:
    out = true;
    // jediny zpusob jak se odsud dostat je resetovat chip
    // asi by bylo fajn sem pridat signalizaci nebo neco takovyho
    break;

  default:
    digitalWrite(1, true); // vypnout menic pred restartem
    delay(10000); // jediny zpusb jak se sem dostat je glitch, restartuje chip
    break;
  }

  return out;
}

void inline OutControlReset() {
  stateNow = STATE_START;
}

ISR(TIM0_COMPA_vect) { // pricte kazdou milisekundu, nepouzivam millis() protoze ty jsou jen 32bit
  timeCounter++;
}
