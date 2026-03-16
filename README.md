# ESP32MacroPad

A flexible BLE macro-pad library for ESP32, featuring:

- ✅ Up to **16 buttons / key switches**
- ✅ Up to **4 rotary encoders** (with optional push-buttons)
- ✅ Up to **8 switchable layouts** per button
- ✅ **SSD1306 OLED** action feedback (auto-clears)
- ✅ **BLE Keyboard** output (works with macOS, Windows, Linux, iOS, Android)
- ✅ Interrupt-driven quadrature decoding with fast-spin detection
- ✅ Simple **callback API** — no state machines in your sketch

---

## Dependencies

Install these via Arduino Library Manager before using ESP32MacroPad:

| Library | Author |
|---|---|
| ESP32 BLE Keyboard | T-vK |
| Adafruit SSD1306 | Adafruit |
| Adafruit GFX Library | Adafruit |

---

## Quick Start

```cpp
#include <ESP32MacroPad.h>

// Register hardware
int btn0 = MacroPad.addButton(15);
int btn1 = MacroPad.addButton(2);

MacroPad.addEncoder(14, 12,               // pinA, pinB
    []{ MacroPad.keyboard().write(KEY_MEDIA_VOLUME_UP);   },  // CW
    []{ MacroPad.keyboard().write(KEY_MEDIA_VOLUME_DOWN); },  // CCW
    27,                                   // encoder button pin
    []{ MacroPad.keyboard().write(KEY_MEDIA_MUTE); }          // button press
);

// Bind actions to buttons per layout (0-based)
MacroPad.bindButton(btn0, 0, []{ /* layout 0 action */ }, "Label");
MacroPad.bindButton(btn0, 1, []{ /* layout 1 action */ }, "Label");

MacroPad.setLayoutCount(2);
MacroPad.begin("My MacroPad");   // BLE name

void loop() {
    MacroPad.update();
}
```

---

## API Reference

### Setup

| Method | Description |
|---|---|
| `addButton(pin)` | Register a key switch on `pin`. Returns button index. |
| `addEncoder(pinA, pinB, onCW, onCCW, pinBtn, onPress)` | Register a rotary encoder. `pinBtn` and `onPress` are optional (-1 / nullptr). Returns encoder index. |
| `bindButton(btnIdx, layout, callback, label)` | Assign a callback (and optional OLED label) to a button for a given layout. |
| `setLayoutCount(n)` | Set total number of layouts (default: 3). |
| `begin(deviceName, useDisplay)` | Initialise BLE + OLED. Call in `setup()`. |

### Runtime

| Method | Description |
|---|---|
| `update()` | Poll all inputs. Call every `loop()`. |
| `setLayout(n)` | Switch to layout `n` (0-based). |
| `getLayout()` | Returns current layout index. |
| `showMessage(msg, durationMs)` | Show text on OLED, auto-clears after `durationMs` (default 2000). |
| `keyboard()` | Returns reference to `BleKeyboard` for direct use in callbacks. |
| `display()` | Returns reference to `Adafruit_SSD1306` for custom drawing. |
| `isConnected()` | Returns `true` when BLE host is connected. |

---

## Publishing to Arduino Library Manager

1. Create a GitHub release tagged `v1.0.0`
2. Submit via the [Arduino Library Manager form](https://github.com/arduino/library-registry)

---

## License

MIT
