#pragma once

#include <Arduino.h>
#include <BleKeyboard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── Limits ──────────────────────────────────────────────────────────────────
#define MACROPAD_MAX_BUTTONS  16
#define MACROPAD_MAX_ENCODERS  4
#define MACROPAD_MAX_LAYOUTS   8

// ─── Display defaults ────────────────────────────────────────────────────────
#define MACROPAD_SCREEN_W     128
#define MACROPAD_SCREEN_H      64
#define MACROPAD_OLED_ADDR   0x3C
#define MACROPAD_FEEDBACK_MS 2000   // ms to show action label

// ─── Encoder timing ──────────────────────────────────────────────────────────
#define MACROPAD_ENC_PAUSE_US  25000
#define MACROPAD_ENC_FAST_MUL     10

// ─── Callback types ──────────────────────────────────────────────────────────
typedef void (*MacroPadCallback)();           // simple action
typedef void (*MacroPadLayoutCallback)(int);  // called with new layout index

// ─── Internal structures ──────────────────────────────────────────────────────

struct MacroPadButton {
    int  pin;
    bool active;
    int  lastState;
    int  currentState;

    // Per-layout callbacks (up to MACROPAD_MAX_LAYOUTS)
    MacroPadCallback callbacks[MACROPAD_MAX_LAYOUTS];
    // Per-layout labels shown on OLED (optional, nullptr = no display)
    const char*      labels[MACROPAD_MAX_LAYOUTS];
};

struct MacroPadEncoder {
    int  pinA;
    int  pinB;
    int  pinBtn;       // -1 if no button
    bool active;

    // Interrupt state (marked volatile)
    volatile int     counter;
    volatile int     lastCounter;

    // Timing for fast-spin
    volatile unsigned long lastIncTime;
    volatile unsigned long lastDecTime;

    // Callbacks
    MacroPadCallback onCW;        // clockwise
    MacroPadCallback onCCW;       // counter-clockwise
    MacroPadCallback onPress;     // button press (optional)

    // Internal quadrature state
    volatile uint8_t oldAB;
    volatile int8_t  encVal;
};

// ─── Main class ───────────────────────────────────────────────────────────────

class ESP32MacroPad {
public:
    // ── Constructor ──────────────────────────────────────────────────────────
    ESP32MacroPad();

    // ── Setup ────────────────────────────────────────────────────────────────

    /**
     * @brief  Initialise BLE keyboard, OLED, and all registered pins.
     * @param  deviceName  BLE device name visible to host.
     * @param  useDisplay  Pass false to skip OLED initialisation.
     * @return true if OLED started OK (or useDisplay==false).
     */
    bool begin(const char* deviceName = "ESP32 MacroPad",
               bool useDisplay = true);

    // ── Register inputs ──────────────────────────────────────────────────────

    /**
     * @brief  Register a button/key switch.
     * @param  pin    GPIO pin (INPUT_PULLUP assumed).
     * @return button index, or -1 on failure.
     */
    int addButton(int pin);

    /**
     * @brief  Bind a callback to a button for a specific layout.
     * @param  btnIdx   Index returned by addButton().
     * @param  layout   0-based layout index.
     * @param  cb       Function to call on press.
     * @param  label    Optional OLED label (e.g. "Play/Pause"). nullptr = no display.
     */
    void bindButton(int btnIdx, int layout, MacroPadCallback cb,
                    const char* label = nullptr);

    /**
     * @brief  Register a rotary encoder.
     * @param  pinA    Encoder channel A GPIO.
     * @param  pinB    Encoder channel B GPIO.
     * @param  onCW    Callback for clockwise rotation.
     * @param  onCCW   Callback for counter-clockwise rotation.
     * @param  pinBtn  Encoder push-button GPIO (-1 if none).
     * @param  onPress Callback for encoder button press (nullptr if none).
     * @return encoder index, or -1 on failure.
     */
    int addEncoder(int pinA, int pinB,
                   MacroPadCallback onCW, MacroPadCallback onCCW,
                   int pinBtn = -1, MacroPadCallback onPress = nullptr);

    // ── Layout ───────────────────────────────────────────────────────────────

    /** Set total number of layouts (1–MACROPAD_MAX_LAYOUTS). */
    void setLayoutCount(int count);

    /** Jump to a specific layout (0-based). */
    void setLayout(int layout);

    /** Get current layout index (0-based). */
    int  getLayout() const;

    // ── OLED helpers ─────────────────────────────────────────────────────────

    /**
     * @brief  Show a temporary message on the OLED.
     *         Automatically cleared after MACROPAD_FEEDBACK_MS ms on next update().
     */
    void showMessage(const char* msg, uint32_t durationMs = MACROPAD_FEEDBACK_MS);

    /** Direct access to the display for custom drawing. */
    Adafruit_SSD1306& display();

    // ── BLE helpers ──────────────────────────────────────────────────────────

    /** Direct access to BleKeyboard for use inside callbacks. */
    BleKeyboard& keyboard();

    bool isConnected() const;

    // ── Main loop ────────────────────────────────────────────────────────────

    /**
     * @brief  Poll buttons and encoders. Call this every loop() iteration.
     *         Only active when BLE is connected.
     */
    void update();

    // ── Static ISR trampolines ────────────────────────────────────────────────
    // Public so attachInterrupt lambdas can reach them; not intended for user code.
    void _isrEncoder(int idx);

private:
    // ── State ────────────────────────────────────────────────────────────────
    MacroPadButton  _buttons[MACROPAD_MAX_BUTTONS];
    int             _buttonCount;

    MacroPadEncoder _encoders[MACROPAD_MAX_ENCODERS];
    int             _encoderCount;

    int  _layout;
    int  _layoutCount;

    bool _useDisplay;
    bool _displayOK;

    Adafruit_SSD1306 _display;
    BleKeyboard      _keyboard;

    // ── OLED feedback ────────────────────────────────────────────────────────
    bool     _msgPending;
    uint32_t _msgClearAt;

    void _clearMessage();

    // ── Internal helpers ─────────────────────────────────────────────────────
    void _initPins();
    void _pollButtons();
    void _pollEncoders();
};

// ─── Singleton helper (optional) ─────────────────────────────────────────────
// If you only ever have one MacroPad in a sketch you can use the global below.
// Interrupt service routines need a global/static pointer to reach class methods.

extern ESP32MacroPad MacroPad;
