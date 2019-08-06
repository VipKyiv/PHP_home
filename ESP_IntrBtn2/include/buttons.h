#ifndef __BUTTONS_H
#define __BUTTONS_H

//#define INTR_EXCLUSIIVE

#ifdef INTR_EXCLUSIIVE
extern "C" {
#include <user_interface.h>
}
#endif
#include <Arduino.h>
#include "list.h"
#include "queue.h"

struct __packed button_t {
  uint8_t pin : 4;
  bool level : 1;
  bool paused : 1;
  bool pressed : 1;
  bool dblclickable : 1;
  volatile uint16_t duration;
};

class Buttons : public List<button_t, 10> {
public:
  enum buttonstate_t : uint8_t { BTN_RELEASED, BTN_PRESSED, BTN_CLICK, BTN_LONGCLICK, BTN_DBLCLICK };

  Buttons() : List<button_t, 10>(), _isrtime(0) {
#ifdef INTR_EXCLUSIIVE
    ETS_GPIO_INTR_ATTACH(_isr, this);
#endif
  }

  uint8_t add(uint8_t pin, bool level);
  void pause(uint8_t index);
  void resume(uint8_t index);

protected:
  static const uint16_t CLICK_TIME = 20; // 20 ms.
  static const uint16_t LONGCLICK_TIME = 1000; // 1 sec.
  static const uint16_t DBLCLICK_TIME = 500; // 0.5 sec.

  void cleanup(void *ptr);
  bool match(uint8_t index, const void *t);

  static void _isr(Buttons *_this);
  virtual void onChange(buttonstate_t state, uint8_t button) {}

  uint32_t _isrtime;
};

class QueuedButtons : public Buttons {
public:
  struct __packed event_t {
    buttonstate_t state : 3;
    uint8_t button : 4;
  };

  QueuedButtons() : Buttons(), _events() {}

  uint8_t eventCount() const {
    return _events.depth();
  }
  const event_t *peekEvent() {
    return _events.peek();
  }
  const event_t *getEvent() {
    return _events.get();
  }

protected:
  void onChange(buttonstate_t state, uint8_t button);

  Queue<event_t, 32> _events;
};

#endif
