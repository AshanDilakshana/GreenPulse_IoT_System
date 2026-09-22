#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>

class ActuatorManager {
private:
    uint8_t _redPin;
    uint8_t _greenPin;
    uint8_t _bluePin;
    uint8_t _pumpPin;
    uint8_t _lampPin;
    uint8_t _buzzerPin;

    bool _isPumpActive;
    unsigned long _lastBlinkTime;
    unsigned long _pumpStartTime;
    bool _blinkState;
    String _lastColorHex;
    
    // Buzzer state variables
    bool _isMotionActive;
    unsigned long _lastBuzzerToggle;
    bool _buzzerState;
    int _postMotionBeepsRemaining;
    
    // Edge Computing Auto-Off
    int _targetMoisture;

    // Pump Buzzer State
    bool _pumpBeepActive;
    unsigned long _pumpBeepStartTime;

public:
    ActuatorManager(uint8_t redPin, uint8_t greenPin, uint8_t bluePin, uint8_t pumpPin, uint8_t lampPin, uint8_t buzzerPin)
        : _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin), _pumpPin(pumpPin), _lampPin(lampPin), _buzzerPin(buzzerPin),
          _isPumpActive(false), _lastBlinkTime(0), _pumpStartTime(0), _blinkState(false), _lastColorHex("#000000"),
          _isMotionActive(false), _lastBuzzerToggle(0), _buzzerState(false), _postMotionBeepsRemaining(0),
          _targetMoisture(0), _pumpBeepActive(false), _pumpBeepStartTime(0) {}

    void begin() {
        pinMode(_redPin, OUTPUT);
        pinMode(_greenPin, OUTPUT);
        pinMode(_bluePin, OUTPUT);
        
        // Write HIGH before pinMode to prevent active-low relays from flickering ON during boot
        digitalWrite(_pumpPin, HIGH);
        digitalWrite(_lampPin, HIGH);
        pinMode(_pumpPin, OUTPUT);
        pinMode(_lampPin, OUTPUT);
        
        pinMode(_buzzerPin, OUTPUT);
        digitalWrite(_buzzerPin, LOW); // Buzzer normally OFF

        turnOff();
    }

    void setColor(uint8_t red, uint8_t green, uint8_t blue) {
        analogWrite(_redPin, red);
        analogWrite(_greenPin, green);
        analogWrite(_bluePin, blue);
    }

    void setColorHex(const char* hexColor, bool saveState = true) {
        if (hexColor == nullptr) return;

        String colorStr = String(hexColor);
        colorStr.trim();
        if (colorStr.startsWith("#")) {
            colorStr = colorStr.substring(1);
        }

        if (saveState) {
            _lastColorHex = "#" + colorStr;
        }

        if (colorStr.length() == 6) {
            long number = strtol(colorStr.c_str(), NULL, 16);
            uint8_t r = (number >> 16) & 0xFF;
            uint8_t g = (number >> 8) & 0xFF;
            uint8_t b = number & 0xFF;
            setColor(r, g, b);
        }
    }

    void setIndicatorColor(const char* colorName) {
        if (colorName == nullptr) return;
        String colorStr = String(colorName);
        colorStr.toUpperCase();
        
        String targetHex = "#0000FF"; // Default blue
        if (colorStr == "RED") {
            targetHex = "#FF0000";
        } else if (colorStr == "YELLOW") {
            targetHex = "#FFFF00";
        } else if (colorStr == "GREEN") {
            targetHex = "#00FF00";
        }
        
        // Save the intended state
        _lastColorHex = targetHex;

        // Apply immediately only if pump is not overriding it with blinking
        if (!_isPumpActive) {
            setColorHex(targetHex.c_str(), false);
            Serial.printf("[Actuator] Indicator set to %s (%s)\n", colorStr.c_str(), targetHex.c_str());
        }
    }

    void setPump(bool state, int targetMoisture = 0) {
        if (_isPumpActive != state) {
            _isPumpActive = state;
            // Active-LOW relay logic: state=true means LOW (ON), state=false means HIGH (OFF)
            digitalWrite(_pumpPin, state ? LOW : HIGH);
            Serial.printf("[Actuator] Pump is now %s\n", state ? "ON" : "OFF");
            
            if (state) {
                // Start blinking red immediately and record start time
                _blinkState = true;
                _lastBlinkTime = millis();
                _pumpStartTime = millis();
                _targetMoisture = targetMoisture; // Store target for Edge Computing
                if (_targetMoisture > 0) {
                    Serial.printf("[Actuator] Auto-Off target moisture set to: %d%%\n", _targetMoisture);
                }
                setColorHex("#FF0000", false);
            } else {
                // Pump turned off, restore the last known system color
                _targetMoisture = 0;
                setColorHex(_lastColorHex.c_str(), false);
                
                // Ensure pump beep is turned off
                if (_pumpBeepActive) {
                    _pumpBeepActive = false;
                    if (!_isMotionActive && _postMotionBeepsRemaining == 0) digitalWrite(_buzzerPin, LOW);
                }
            }
        }
    }

    void setLamp(bool state) {
        // Active-LOW relay logic
        digitalWrite(_lampPin, state ? LOW : HIGH);
        Serial.printf("[Actuator] Smart Lamp is now %s\n", state ? "ON" : "OFF");
    }

    void loop() {
        unsigned long now = millis();
        
        // Pump safety and Blinking logic
        if (_isPumpActive) {
            // Pulse Watering Safety Timeout (Max 15 seconds)
            if (now - _pumpStartTime >= 15000) {
                Serial.println("[Actuator] 15s hardware timeout reached. Auto-stopping pump (Pulse watering).");
                setPump(false);
            } else {
                // Blinking logic
                if (now - _lastBlinkTime >= 500) { // Toggle every 500ms
                    _lastBlinkTime = now;
                    _blinkState = !_blinkState;
                    if (_blinkState) {
                        setColorHex("#FF0000", false); // Red ON
                        
                        // Start a very short, non-annoying beep (50ms)
                        if (!_isMotionActive) { // Don't conflict with PIR buzzer
                            digitalWrite(_buzzerPin, HIGH);
                            _pumpBeepActive = true;
                            _pumpBeepStartTime = now;
                        }
                    } else {
                        setColorHex("#000000", false); // OFF
                    }
                }
            }
        }
        
        // Turn off the short pump beep after 50ms non-blockingly
        if (_pumpBeepActive && (now - _pumpBeepStartTime >= 50)) {
            _pumpBeepActive = false;
            if (!_isMotionActive && _postMotionBeepsRemaining == 0) {
                digitalWrite(_buzzerPin, LOW);
            }
        }
        
        // Non-blocking Buzzer Pattern for Motion Detection
        if (_isMotionActive) {
            if (now - _lastBuzzerToggle >= 250) { // Beep toggle every 250ms (fast pattern)
                _lastBuzzerToggle = now;
                _buzzerState = !_buzzerState;
                digitalWrite(_buzzerPin, _buzzerState ? HIGH : LOW);
            }
        } else if (_postMotionBeepsRemaining > 0) {
            // Post-motion double beep (same pattern speed as motion)
            if (now - _lastBuzzerToggle >= 250) {
                _lastBuzzerToggle = now;
                _buzzerState = !_buzzerState;
                digitalWrite(_buzzerPin, _buzzerState ? HIGH : LOW);
                _postMotionBeepsRemaining--;
                
                // Ensure it stays OFF when finished
                if (_postMotionBeepsRemaining == 0) {
                    digitalWrite(_buzzerPin, LOW);
                    _buzzerState = false;
                }
            }
        }
    }

    // Call this to update motion state for buzzer
    void setMotionState(bool state) {
        if (_isMotionActive != state) {
            _isMotionActive = state;
            if (!state) {
                // Turn off continuous buzzer and start 2-beep exit pattern (4 toggles)
                _postMotionBeepsRemaining = 4;
                _buzzerState = false; 
                digitalWrite(_buzzerPin, LOW);
                _lastBuzzerToggle = millis(); // start immediately
            } else {
                // Motion started again, cancel any exit beeps
                _postMotionBeepsRemaining = 0;
            }
        }
    }

    void turnOff() {
        setColor(0, 0, 0);
        // Active-LOW relays turn OFF when receiving HIGH signal
        digitalWrite(_pumpPin, HIGH);
        digitalWrite(_lampPin, HIGH);
        digitalWrite(_buzzerPin, LOW);
        _isPumpActive = false;
        _lastColorHex = "#000000";
    }

    // Check if LLM decided the plant needs care (Red or Yellow indicator)
    bool needsCare() {
        return (_lastColorHex == "#FF0000" || _lastColorHex == "#FFFF00");
    }

    // Trigger a fast triple beep alert
    void triggerCareAlert() {
        for (int i = 0; i < 3; i++) {
            digitalWrite(_buzzerPin, HIGH);
            delay(50); // Small 50ms blocking delay is acceptable for alert
            digitalWrite(_buzzerPin, LOW);
            delay(50);
        }
    }

    // --- Edge Computing Check ---
    // Called continuously from main loop to auto-stop pump instantly when target reached
    void checkAutoOff(int currentMoisture) {
        if (_isPumpActive && _targetMoisture > 0) {
            if (currentMoisture >= _targetMoisture) {
                Serial.printf("[Actuator] Target moisture (%d%%) reached. Edge computing auto-off triggered!\n", _targetMoisture);
                setPump(false);
                setIndicatorColor("GREEN"); // Recover immediately
            }
        }
    }
};

#endif
