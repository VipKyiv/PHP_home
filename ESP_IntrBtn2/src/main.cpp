#include <Arduino.h>
#include "buttons.h"

QueuedButtons btns;

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  btns.add(D1, LOW);
  btns.add(D2, LOW);
  btns.add(D3, LOW);
  btns.add(D5, LOW);
}

void loop() {
  while (QueuedButtons::event_t *evt = (QueuedButtons::event_t*)btns.getEvent()) {
    Serial.print(F("Button #"));
    Serial.print(evt->button);
    Serial.print(' ');
    if (evt->state == Buttons::BTN_RELEASED) {
      Serial.println(F("released"));
    } else if (evt->state == Buttons::BTN_PRESSED) {
      Serial.println(F("pressed"));
    } else if (evt->state == Buttons::BTN_CLICK) {
      Serial.println(F("clicked"));
    } else if (evt->state == Buttons::BTN_LONGCLICK) {
      Serial.println(F("long clicked"));
    } else if (evt->state == Buttons::BTN_DBLCLICK) {
      Serial.println(F("double clicked"));
    }
    if (evt->button == 0) {
      digitalWrite(LED_BUILTIN, evt->state != Buttons::BTN_PRESSED);
    }
  }
  delay(1);
}
