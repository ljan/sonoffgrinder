/*
 * TTBOUNCE
 * Version 1.0 January, 2014
 * Version 1.1 February, 2014
 * Version 1.2 July, 2018
 * Version 1.3 October, 2018
 * Version 1.4 January, 2020
 * Copyright 2009 TOLDO TECHNIK
 * For details, see https://github.com/TOLDOTECHNIK/TTBOUNCE
 */

#include "TTBOUNCE.h"

TTBOUNCE::TTBOUNCE(uint8_t pin) {
  _useWithHardwarePin = pin < TTBOUNCE_WITHOUT_PIN ? true : false;
  if (_useWithHardwarePin) {
    _pin = pin;
    pinMode(_pin, INPUT);
  }
  setActiveHigh();
  _debounceInterval = TTBOUNCE_DEFAULT_DEBOUNCE_INTERVAL;
  _clickInterval    = TTBOUNCE_DEFAULT_CLICK_INTERVAL;
  _pressInterval    = TTBOUNCE_DEFAULT_PRESS_INTERVAL;
  _reTickInterval   = TTBOUNCE_DEFAULT_RETICK_INTERVAL;
  _timestamp        = millis();
}

void TTBOUNCE::setActiveHigh() {
  _activeHigh      = true;
  _currentPinState = LOW;
}

void TTBOUNCE::setActiveLow() {
  _activeHigh      = false;
  _currentPinState = HIGH;
}

void TTBOUNCE::enablePullup() {
  if (_useWithHardwarePin) {
    digitalWrite(_pin, HIGH);
  }
}

void TTBOUNCE::disablePullup() {
  if (_useWithHardwarePin) {
    digitalWrite(_pin, LOW);
  }
}

void TTBOUNCE::setDebounceInterval(unsigned int interval) {
  _debounceInterval = interval;
}

void TTBOUNCE::setPressInterval(unsigned int interval) {
  _pressInterval = interval;
}

void TTBOUNCE::setClickInterval(unsigned int interval) {
  _clickInterval = interval;
}

void TTBOUNCE::setReTickInterval(unsigned int interval) {
  _reTickInterval = interval;
}

void TTBOUNCE::attachClick(callbackFunction function) {
  _clickFunction = function;
}

void TTBOUNCE::attachDoubleClick(callbackFunction function) {
  _doubleClickFunction = function;
}

void TTBOUNCE::attachPress(callbackFunction function) {
  _pressFunction = function;
}

void TTBOUNCE::attachRelease(callbackFunction function) {
  _releaseFunction = function;
}

void TTBOUNCE::attachReTick(callbackFunction function) {
  _reTickFunction = function;
}

void TTBOUNCE::update(boolean virtualPinState) {
  uint8_t pinState = virtualPinState;
  if (_useWithHardwarePin) {
    pinState = digitalRead(_pin);
  }

  // debounce
  if (pinState != _currentPinUnstableState) {
    _timestamp = millis();
  } else if (millis() - _timestamp >= _debounceInterval) {
    if (pinState != _currentPinState) {
      _currentPinState = pinState;
      if (read()) {
        _previousHighStateTime = millis();
      }
    }
  }
  _currentPinUnstableState = pinState;

  // states
  switch (_state) {
    case 0:
      if (read() == HIGH) {
        _state     = 1;
        _timestamp = millis();    // remember starting time
      }
      break;
    case 1:
      if (read() == LOW) {
        _state = 2;
      } else if ((read() == HIGH) && (millis() > _timestamp + _pressInterval)) {
        if (_pressFunction)
          _pressFunction();
        _state = 4;
      }
      break;
    case 2:
      if (millis() > _timestamp + _clickInterval ||
          (read() == LOW && !_doubleClickFunction)) {
        if (_clickFunction)
          _clickFunction();
        _state = 0;
      } else if (read() == HIGH) {
        _state = 3;
      }
      break;
    case 3:
      if (read() == LOW) {
        if (_doubleClickFunction)
          _doubleClickFunction();
        _state = 0;
      }
      break;
    case 4:
      if (read() == LOW) {
        if (_releaseFunction)
          _releaseFunction();
        _state = 0;
      } else {
        if (_reTickFunction) {
          if (millis() - _previousReTickTime > _reTickInterval) {
            _reTickFunction();
            _previousReTickTime = millis();
          }
        }
      }
      break;
  }
}

uint8_t TTBOUNCE::read() {
  if (_activeHigh) {
    return _currentPinState;
  } else {
    return !_currentPinState;
  }
}

unsigned long TTBOUNCE::getHoldTime() {
  if (read()) {
    return millis() - _previousHighStateTime;
  } else {
    return 0;
  }
}
