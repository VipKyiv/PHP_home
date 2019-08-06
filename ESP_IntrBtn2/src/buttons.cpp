#ifndef INTR_EXCLUSIIVE
#include <functional>
#include <FunctionalInterrupt.h>
#endif
#include "buttons.h"

uint8_t Buttons::add(uint8_t pin, bool level) {
  uint8_t result;
  button_t b;

  if (pin > 15)
    return ERR_INDEX;
  b.pin = pin;
  b.level = level;
  b.paused = false;
  b.pressed = false;
  b.dblclickable = false;
  b.duration = 0;
  result = List<button_t, 10>::add(b);
  if (result != ERR_INDEX) {
    pinMode(pin, level ? INPUT : INPUT_PULLUP);
#ifdef INTR_EXCLUSIIVE
    ETS_GPIO_INTR_DISABLE();
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
    gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_ANYEDGE);
    ETS_GPIO_INTR_ENABLE();
#else
    attachInterrupt(pin, [this]() { this->_isr(this); }, CHANGE);
#endif
  }

  return result;
}

void Buttons::pause(uint8_t index) {
  if (_items && (index < _count)) {
    _items[index].paused = true;
#ifdef INTR_EXCLUSIIVE
    ETS_GPIO_INTR_DISABLE();
    gpio_pin_intr_state_set(GPIO_ID_PIN(_items[index].pin), GPIO_PIN_INTR_DISABLE);
    ETS_GPIO_INTR_ENABLE();
#else
    detachInterrupt(_items[index].pin);
#endif
  }
}

void Buttons::resume(uint8_t index) {
  if (_items && (index < _count)) {
    _items[index].paused = false;
    _items[index].pressed = false;
    _items[index].dblclickable = false;
    _items[index].duration = 0;
#ifdef INTR_EXCLUSIIVE
    ETS_GPIO_INTR_DISABLE();
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(_items[index].pin));
    gpio_pin_intr_state_set(GPIO_ID_PIN(_items[index].pin), GPIO_PIN_INTR_ANYEDGE);
    ETS_GPIO_INTR_ENABLE();
#else
    attachInterrupt(_items[index].pin, [this]() { this->_isr(this); }, CHANGE);
#endif
  }
}

void ICACHE_RAM_ATTR Buttons::_isr(Buttons *_this) {
#ifdef INTR_EXCLUSIIVE
  uint32_t status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
#endif

  if (_this->_items && _this->_count) {
    uint32_t time = millis() - _this->_isrtime;
    uint32_t inputs = GPI;

    for (uint8_t i = 0; i < _this->_count; ++i) {
      if (_this->_items[i].paused)
        continue;
      if (_this->_items[i].duration) {
        if (time + _this->_items[i].duration < 0xFFFF)
          _this->_items[i].duration += time;
        else
          _this->_items[i].duration = 0xFFFF;
      }
      if (((inputs >> _this->_items[i].pin) & 0x01) == _this->_items[i].level) { // Button pressed
        if (! _this->_items[i].pressed) {
          _this->_items[i].dblclickable = (_this->_items[i].duration > 0) && (_this->_items[i].duration <= DBLCLICK_TIME);
          _this->_items[i].pressed = true;
          _this->_items[i].duration = 1;
          _this->onChange(BTN_PRESSED, i);
        }
      } else { // Button released
        if (_this->_items[i].pressed) { // Was pressed
          if (_this->_items[i].duration >= LONGCLICK_TIME) {
            _this->onChange(BTN_LONGCLICK, i);
          } else if (_this->_items[i].duration >= CLICK_TIME) {
            if (_this->_items[i].dblclickable)
              _this->onChange(BTN_DBLCLICK, i);
            else
              _this->onChange(BTN_CLICK, i);
          } else {
            _this->onChange(BTN_RELEASED, i);
          }
          _this->_items[i].pressed = false;
          if ((_this->_items[i].duration >= CLICK_TIME) && (_this->_items[i].duration < LONGCLICK_TIME))
            _this->_items[i].duration = 1;
          else
            _this->_items[i].duration = 0;
        }
      }
    }
  }
#ifdef INTR_EXCLUSIIVE
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);
#endif
  _this->_isrtime = millis();
}

void Buttons::cleanup(void *ptr) {
#ifdef INTR_EXCLUSIIVE
  ETS_GPIO_INTR_DISABLE();
  gpio_pin_intr_state_set(GPIO_ID_PIN(((button_t*)ptr)->pin), GPIO_PIN_INTR_DISABLE);
  ETS_GPIO_INTR_ENABLE();
#else
  detachInterrupt(((button_t*)ptr)->pin);
#endif
}

bool Buttons::match(uint8_t index, const void *t) {
  if (_items && (index < _count)) {
    return (_items[index].pin == ((button_t*)t)->pin);
  }

  return false;
}

void QueuedButtons::onChange(buttonstate_t state, uint8_t button) {
  event_t e;

  e.state = state;
  e.button = button;

  _events.put(&e, true);
}
