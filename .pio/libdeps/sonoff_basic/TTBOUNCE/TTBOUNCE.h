#ifndef TTBOUNCE_h
#define TTBOUNCE_h

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <Wprogram.h>
#endif

#define TTBOUNCE_DEFAULT_DEBOUNCE_INTERVAL  10
#define TTBOUNCE_DEFAULT_CLICK_INTERVAL     300
#define TTBOUNCE_DEFAULT_PRESS_INTERVAL     1000
#define TTBOUNCE_DEFAULT_RETICK_INTERVAL    200
#define TTBOUNCE_WITHOUT_PIN                255

extern "C" {
  typedef void (*callbackFunction)(void);
}

class TTBOUNCE{
public:
  TTBOUNCE(uint8_t pin); 
  void setActiveHigh();
  void setActiveLow();
  void enablePullup();
  void disablePullup();
  void setDebounceInterval(unsigned int interval);
  void setPressInterval(unsigned int interval);
  void setClickInterval(unsigned int interval);
  void setReTickInterval(unsigned int interval);
  void attachClick(callbackFunction function);
  void attachDoubleClick(callbackFunction function);
  void attachPress(callbackFunction function);
  void attachRelease(callbackFunction function);
  void attachReTick(callbackFunction function);
  void update(boolean virtualPinState = 0);
  uint8_t read();
  unsigned long getHoldTime();

private:
  uint8_t _pin;
  uint8_t _useWithHardwarePin;
  uint8_t _activeHigh;
  uint8_t _state;
  uint8_t _currentPinState;
  uint8_t _currentPinUnstableState;
  
  callbackFunction _clickFunction;
  callbackFunction _doubleClickFunction;
  callbackFunction _pressFunction;
  callbackFunction _releaseFunction;
  callbackFunction _reTickFunction;

  unsigned long _timestamp;
  unsigned int _debounceInterval;
  unsigned int _clickInterval;
  unsigned int _pressInterval;
  unsigned int _reTickInterval;
  unsigned long _previousHighStateTime;
  unsigned long _previousReTickTime;
};

#endif
