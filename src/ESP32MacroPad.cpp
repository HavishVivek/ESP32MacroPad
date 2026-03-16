#include "ESP32MacroPad.h"
#include <Fonts/FreeMonoBold12pt7b.h>

// ─── Singleton definition ─────────────────────────────────────────────────────
ESP32MacroPad MacroPad;

// ─── ISR trampoline table ─────────────────────────────────────────────────────
// ESP32 ISRs cannot be member functions, so we keep a small static array of
// pointers and a set of free-function trampolines.

static ESP32MacroPad* _isrInstances[MACROPAD_MAX_ENCODERS] = {};

static void IRAM_ATTR _enc0_isr() { if (_isrInstances[0]) _isrInstances[0]->_isrEncoder(0); }
static void IRAM_ATTR _enc1_isr() { if (_isrInstances[1]) _isrInstances[1]->_isrEncoder(1); }
static void IRAM_ATTR _enc2_isr() { if (_isrInstances[2]) _isrInstances[2]->_isrEncoder(2); }
static void IRAM_ATTR _enc3_isr() { if (_isrInstances[3]) _isrInstances[3]->_isrEncoder(3); }

static void (* const _isrFuncs[MACROPAD_MAX_ENCODERS])() = {
    _enc0_isr, _enc1_isr, _enc2_isr, _enc3_isr
};

// ─── Constructor ──────────────────────────────────────────────────────────────

ESP32MacroPad::ESP32MacroPad()
    : _buttonCount(0),
      _encoderCount(0),
      _layout(0),
      _layoutCount(3),
      _useDisplay(true),
      _displayOK(false),
      _display(MACROPAD_SCREEN_W, MACROPAD_SCREEN_H, &Wire, -1),
      _keyboard("ESP32 MacroPad"),
      _msgPending(false),
      _msgClearAt(0)
{
    memset(_buttons,  0, sizeof(_buttons));
    memset(_encoders, 0, sizeof(_encoders));

    for (int b = 0; b < MACROPAD_MAX_BUTTONS; b++) {
        _buttons[b].lastState = HIGH;
        for (int l = 0; l < MACROPAD_MAX_LAYOUTS; l++) {
            _buttons[b].callbacks[l] = nullptr;
            _buttons[b].labels[l]    = nullptr;
        }
    }
    for (int e = 0; e < MACROPAD_MAX_ENCODERS; e++) {
        _encoders[e].oldAB       = 3;
        _encoders[e].encVal      = 0;
        _encoders[e].counter     = 0;
        _encoders[e].lastCounter = 0;
        _encoders[e].lastIncTime = 0;
        _encoders[e].lastDecTime = 0;
        _encoders[e].pinBtn      = -1;
    }
}

// ─── begin() ─────────────────────────────────────────────────────────────────

bool ESP32MacroPad::begin(const char* deviceName, bool useDisplay) {
    _useDisplay = useDisplay;
    _keyboard   = BleKeyboard(deviceName);
    _keyboard.begin();

    _initPins();

    if (_useDisplay) {
        _displayOK = _display.begin(SSD1306_SWITCHCAPVCC, MACROPAD_OLED_ADDR);
        if (_displayOK) {
            _display.clearDisplay();
            _display.setFont(&FreeMonoBold12pt7b);
            _display.setTextSize(1);
            _display.setTextColor(WHITE);
            _display.setCursor(0, 20);
            _display.println("MacroPad");
            _display.display();
        }
    }

    return (!_useDisplay) || _displayOK;
}

// ─── addButton() ──────────────────────────────────────────────────────────────

int ESP32MacroPad::addButton(int pin) {
    if (_buttonCount >= MACROPAD_MAX_BUTTONS) return -1;
    int idx = _buttonCount++;
    _buttons[idx].pin       = pin;
    _buttons[idx].active    = true;
    _buttons[idx].lastState = HIGH;
    return idx;
}

// ─── bindButton() ─────────────────────────────────────────────────────────────

void ESP32MacroPad::bindButton(int btnIdx, int layout,
                                MacroPadCallback cb, const char* label) {
    if (btnIdx < 0 || btnIdx >= _buttonCount) return;
    if (layout < 0 || layout >= MACROPAD_MAX_LAYOUTS) return;
    _buttons[btnIdx].callbacks[layout] = cb;
    _buttons[btnIdx].labels[layout]    = label;
}

// ─── addEncoder() ────────────────────────────────────────────────────────────

int ESP32MacroPad::addEncoder(int pinA, int pinB,
                               MacroPadCallback onCW, MacroPadCallback onCCW,
                               int pinBtn, MacroPadCallback onPress) {
    if (_encoderCount >= MACROPAD_MAX_ENCODERS) return -1;
    int idx = _encoderCount++;

    _encoders[idx].pinA    = pinA;
    _encoders[idx].pinB    = pinB;
    _encoders[idx].pinBtn  = pinBtn;
    _encoders[idx].onCW    = onCW;
    _encoders[idx].onCCW   = onCCW;
    _encoders[idx].onPress = onPress;
    _encoders[idx].active  = true;

    _isrInstances[idx] = this;

    return idx;
}

// ─── Layout ──────────────────────────────────────────────────────────────────

void ESP32MacroPad::setLayoutCount(int count) {
    _layoutCount = constrain(count, 1, MACROPAD_MAX_LAYOUTS);
}

void ESP32MacroPad::setLayout(int layout) {
    _layout = layout % _layoutCount;
    if (_layout < 0) _layout += _layoutCount;
}

int ESP32MacroPad::getLayout() const { return _layout; }

// ─── OLED helpers ────────────────────────────────────────────────────────────

void ESP32MacroPad::showMessage(const char* msg, uint32_t durationMs) {
    if (!_displayOK) return;
    _display.clearDisplay();
    _display.setFont(&FreeMonoBold12pt7b);
    _display.setTextSize(1);
    _display.setTextColor(WHITE);
    _display.setCursor(0, 20);
    _display.println(msg);
    _display.display();
    _msgPending  = true;
    _msgClearAt  = millis() + durationMs;
}

void ESP32MacroPad::_clearMessage() {
    if (_msgPending && millis() >= _msgClearAt) {
        _display.clearDisplay();
        _display.display();
        _msgPending = false;
    }
}

Adafruit_SSD1306& ESP32MacroPad::display() { return _display; }
BleKeyboard&      ESP32MacroPad::keyboard() { return _keyboard; }
bool              ESP32MacroPad::isConnected() const { return _keyboard.isConnected(); }

// ─── update() ────────────────────────────────────────────────────────────────

void ESP32MacroPad::update() {
    if (!_keyboard.isConnected()) return;

    _pollButtons();
    _pollEncoders();

    if (_useDisplay && _displayOK) {
        _clearMessage();
    }
}

// ─── _initPins() ─────────────────────────────────────────────────────────────

void ESP32MacroPad::_initPins() {
    // Buttons
    for (int i = 0; i < _buttonCount; i++) {
        if (_buttons[i].active) {
            pinMode(_buttons[i].pin, INPUT_PULLUP);
        }
    }

    // Encoders
    for (int i = 0; i < _encoderCount; i++) {
        if (!_encoders[i].active) continue;

        pinMode(_encoders[i].pinA, INPUT_PULLUP);
        pinMode(_encoders[i].pinB, INPUT_PULLUP);

        if (_encoders[i].pinBtn >= 0) {
            pinMode(_encoders[i].pinBtn, INPUT_PULLUP);
        }

        // Attach quadrature interrupts
        attachInterrupt(digitalPinToInterrupt(_encoders[i].pinA), _isrFuncs[i], CHANGE);
        attachInterrupt(digitalPinToInterrupt(_encoders[i].pinB), _isrFuncs[i], CHANGE);
    }
}

// ─── _pollButtons() ──────────────────────────────────────────────────────────

void ESP32MacroPad::_pollButtons() {
    for (int i = 0; i < _buttonCount; i++) {
        if (!_buttons[i].active) continue;

        _buttons[i].currentState = digitalRead(_buttons[i].pin);

        // Detect falling edge (press) with pull-up logic
        if (_buttons[i].lastState == HIGH && _buttons[i].currentState == LOW) {
            MacroPadCallback cb    = _buttons[i].callbacks[_layout];
            const char*      label = _buttons[i].labels[_layout];

            if (cb) cb();

            if (label && _displayOK) {
                showMessage(label);
            }
        }

        _buttons[i].lastState = _buttons[i].currentState;
    }
}

// ─── _pollEncoders() ─────────────────────────────────────────────────────────

void ESP32MacroPad::_pollEncoders() {
    for (int i = 0; i < _encoderCount; i++) {
        if (!_encoders[i].active) continue;

        int c = _encoders[i].counter;  // snapshot volatile

        if (c > _encoders[i].lastCounter) {
            if (_encoders[i].onCW) _encoders[i].onCW();
            _encoders[i].lastCounter = c;
        } else if (c < _encoders[i].lastCounter) {
            if (_encoders[i].onCCW) _encoders[i].onCCW();
            _encoders[i].lastCounter = c;
        }

        // Encoder push-button
        if (_encoders[i].pinBtn >= 0 && _encoders[i].onPress) {
            // We store button state in encVal temporarily (reuse field is messy —
            // use a dedicated static per encoder instead)
            static int encBtnLast[MACROPAD_MAX_ENCODERS] = {HIGH, HIGH, HIGH, HIGH};
            int cur = digitalRead(_encoders[i].pinBtn);
            if (encBtnLast[i] == HIGH && cur == LOW) {
                _encoders[i].onPress();
            }
            encBtnLast[i] = cur;
        }
    }
}

// ─── _isrEncoder() ───────────────────────────────────────────────────────────

void IRAM_ATTR ESP32MacroPad::_isrEncoder(int idx) {
    static const int8_t enc_states[] =
        {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

    MacroPadEncoder& enc = _encoders[idx];

    enc.oldAB <<= 2;
    if (digitalRead(enc.pinA)) enc.oldAB |= 0x02;
    if (digitalRead(enc.pinB)) enc.oldAB |= 0x01;

    enc.encVal += enc_states[enc.oldAB & 0x0F];

    if (enc.encVal > 3) {
        int delta = 1;
        if ((micros() - enc.lastIncTime) < MACROPAD_ENC_PAUSE_US)
            delta = MACROPAD_ENC_FAST_MUL;
        enc.lastIncTime = micros();
        enc.counter    += delta;
        enc.encVal      = 0;
    } else if (enc.encVal < -3) {
        int delta = -1;
        if ((micros() - enc.lastDecTime) < MACROPAD_ENC_PAUSE_US)
            delta = -MACROPAD_ENC_FAST_MUL;
        enc.lastDecTime = micros();
        enc.counter    += delta;
        enc.encVal      = 0;
    }
}
