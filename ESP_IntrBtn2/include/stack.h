#ifndef __STACK_H
#define __STACK_H

#include <inttypes.h>
#include <string.h>

template <class T, uint8_t MAX_SIZE = 32>
class Stack {
public:
  Stack() : _length(0) {}

  uint8_t length() const {
    return _length;
  }
  bool push(const T *t);
  const T *peek();
  const T *pop();

protected:
  struct __packed {
    uint8_t _length;
    T _items[MAX_SIZE];
  };
};

template <class T, uint8_t MAX_SIZE>
bool Stack<T, MAX_SIZE>::push(const T *t) {
  if (_length >= MAX_SIZE)
    return false;
  memcpy(&_items[_length++], t, sizeof(T));

  return true;
}

template <class T, uint8_t MAX_SIZE>
const T *Stack<T, MAX_SIZE>::peek() {
  if (! _length)
    return NULL;

  return &_items[_length - 1];
}

template <class T, uint8_t MAX_SIZE>
const T *Stack<T, MAX_SIZE>::pop() {
  if (! _length)
    return NULL;

  return &_items[--_length];
}

#endif
