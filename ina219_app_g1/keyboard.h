#ifndef X_KEYBOARD_H
#define X_KEYBOARD_H

struct Button {
  const uint8_t pin;
  volatile int currentState;
  uint32_t debounceTimeStarted;
  char bChar;
};

void IRAM_ATTR isrButtonR();
void IRAM_ATTR isrButtonL();
void IRAM_ATTR isrButtonU();
void IRAM_ATTR isrButtonD();

bool keyAvail();
char getKey();

#endif
