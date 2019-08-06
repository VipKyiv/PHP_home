#ifndef __LIST_H
#define __LIST_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

template <class T, uint8_t MAX_SIZE = 255>
class List {
public:
  static const uint8_t ERR_INDEX = 0xFF;

  List() : _count(0), _items(NULL) {}
  ~List() {
    clear();
  }

  uint8_t count() const {
    return _count;
  }
  void clear();
  uint8_t add(const T &t);
  void remove(uint8_t index);
  uint8_t find(const T &t);
  T &operator[](uint8_t index) {
    return _items[index];
  }
  const T &operator[](uint8_t index) const {
    return _items[index];
  }

protected:
  virtual void cleanup(void *ptr) {}
  virtual bool match(uint8_t index, const void *t);

  struct __packed {
    uint8_t _count;
    T *_items;
  };
};

template <class T, uint8_t MAX_SIZE>
void List<T, MAX_SIZE>::clear() {
  if (_items) {
    for (int16_t i = _count - 1; i >= 0; --i) {
      cleanup((void*)&_items[i]);
    }
    free((void*)_items);
    _items = NULL;
  }
  _count = 0;
}

template <class T, uint8_t MAX_SIZE>
uint8_t List<T, MAX_SIZE>::add(const T &t) {
  if (_count >= MAX_SIZE)
    return ERR_INDEX;
  if (_items) {
    void *ptr = realloc((void*)_items, sizeof(T) * (_count + 1));
    if (! ptr)
      return ERR_INDEX;
    _items = (T*)ptr;
    ++_count;
  } else {
    _items = (T*)malloc(sizeof(T));
    if (! _items)
      return ERR_INDEX;
    _count = 1;
  }
  memcpy(&_items[_count - 1], &t, sizeof(T));

  return _count - 1;
}

template <class T, uint8_t MAX_SIZE>
void List<T, MAX_SIZE>::remove(uint8_t index) {
  if (_items && (index < _count)) {
    cleanup((void*)&_items[index]);
    if ((_count > 1) && (index < _count - 1))
      memmove((void*)&_items[index], (void*)&_items[index + 1], sizeof(T) * (_count - index - 1));
    --_count;
    if (_count)
      _items = (T*)realloc((void*)_items, sizeof(T) * _count);
    else {
      free((void*)_items);
      _items = NULL;
    }
  }
}

template <class T, uint8_t MAX_SIZE>
uint8_t List<T, MAX_SIZE>::find(const T &t) {
  if (_items) {
    for (uint8_t i = 0; i < _count; ++i) {
      if (match(i, t))
        return i;
    }
  }

  return ERR_INDEX;
}

template <class T, uint8_t MAX_SIZE>
bool List<T, MAX_SIZE>::match(uint8_t index, const void *t) {
  if (index < _count) {
    return (memcmp((void*)&_items[index], t, sizeof(T)) == 0);
  }

  return false;
}

#endif
