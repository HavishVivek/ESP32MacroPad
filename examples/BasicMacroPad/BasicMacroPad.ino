/**
 * BasicMacroPad.ino
 * ------------------
 * Replicates the original 3-layout BLE macropad using the ESP32MacroPad library.
 *
 * Hardware:
 *   - ESP32
 *   - 9 buttons/key switches (GPIO 15, 2, 0, 4, 16, 17, 5, 18, 19)
 *   - Rotary encoder 1 — volume      (A=14, B=12, Btn=27)
 *   - Rotary encoder 2 — layout sel  (A=26, B=25)
 *   - SSD1306 128×64 OLED via I²C
 *
 * Layouts (0-based):
 *   0 → App launcher  (Ctrl+0 … Ctrl+8 shortcuts)
 *   1 → Media control (play/pause, next, prev)
 *   2 → Dev shortcuts (terminal aliases, arduino-cli)
 */

#include <ESP32MacroPad.h>

// ── Pin definitions ──────────────────────────────────────────────────────────
const int BTN_PINS[] = {15, 2, 0, 4, 16, 17, 5, 18, 19};
const int NUM_BTNS   = 9;

const int ENC1_A   = 14;
const int ENC1_B   = 12;
const int ENC1_BTN = 27;

const int ENC2_A   = 26;
const int ENC2_B   = 25;

// ── Button indices (returned by addButton) ───────────────────────────────────
int btns[9];

// ── Encoder callbacks ────────────────────────────────────────────────────────

void volUp()   { MacroPad.keyboard().write(KEY_MEDIA_VOLUME_UP);   }
void volDown() { MacroPad.keyboard().write(KEY_MEDIA_VOLUME_DOWN); }
void mute()    { MacroPad.keyboard().write(KEY_MEDIA_MUTE);        }

void layoutCW() {
    int next = (MacroPad.getLayout() + 1) % 3;
    MacroPad.setLayout(next);
    // Show layout name on OLED
    const char* names[] = {"App Launcher", "Media", "Dev Tools"};
    MacroPad.showMessage(names[next]);
}
void layoutCCW() {
    int prev = (MacroPad.getLayout() + 2) % 3; // +2 mod 3 = -1 mod 3
    MacroPad.setLayout(prev);
    const char* names[] = {"App Launcher", "Media", "Dev Tools"};
    MacroPad.showMessage(names[prev]);
}

// ── Layout 0 — App launcher ──────────────────────────────────────────────────

void appArc()     { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('0'); MacroPad.keyboard().releaseAll(); }
void appSpotify() { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write(1);   MacroPad.keyboard().releaseAll(); }
void appCapCut()  { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('2'); MacroPad.keyboard().releaseAll(); }
void appOBS()     { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('3'); MacroPad.keyboard().releaseAll(); }
void appNotion()  { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('4'); MacroPad.keyboard().releaseAll(); }
void appDiscord() { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('5'); MacroPad.keyboard().releaseAll(); }
void appWhatsApp(){ MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('6'); MacroPad.keyboard().releaseAll(); }
void appEasyEDA() { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('7'); MacroPad.keyboard().releaseAll(); }
void appWarp()    { MacroPad.keyboard().press(KEY_LEFT_CTRL); MacroPad.keyboard().write('8'); MacroPad.keyboard().releaseAll(); }

// ── Layout 1 — Media ─────────────────────────────────────────────────────────

void mediaPlayPause() { MacroPad.keyboard().write(KEY_MEDIA_PLAY_PAUSE);     }
void mediaNext()      { MacroPad.keyboard().write(KEY_MEDIA_NEXT_TRACK);     }
void mediaPrev()      { MacroPad.keyboard().write(KEY_MEDIA_PREVIOUS_TRACK); }

// ── Layout 2 — Dev tools ─────────────────────────────────────────────────────

void devNv()              { MacroPad.keyboard().print("nv");              MacroPad.keyboard().write(KEY_RETURN); }
void devTmNew()           { MacroPad.keyboard().print("tm new");          MacroPad.keyboard().write(KEY_RETURN); }
void devLg()              { MacroPad.keyboard().print("lg");              MacroPad.keyboard().write(KEY_RETURN); }
void devArduinoCompile()  { MacroPad.keyboard().print("arduino-compile");                                        }
void devArduinoUpload()   { MacroPad.keyboard().print("arduino-upload");                                         }
void devArduinoMonitor()  { MacroPad.keyboard().print("arduino-monitor");                                        }
void devESP32Compile()    { MacroPad.keyboard().print("esp32-compile");   MacroPad.keyboard().write(KEY_RETURN); }
void devESP32Upload()     { MacroPad.keyboard().print("esp32-upload");    MacroPad.keyboard().write(KEY_RETURN); }
void devESP32Monitor()    { MacroPad.keyboard().print("esp32-monitor");   MacroPad.keyboard().write(KEY_RETURN); }

// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    // Register buttons
    for (int i = 0; i < NUM_BTNS; i++) {
        btns[i] = MacroPad.addButton(BTN_PINS[i]);
    }

    // Register encoders
    MacroPad.addEncoder(ENC1_A, ENC1_B, volUp, volDown, ENC1_BTN, mute);
    MacroPad.addEncoder(ENC2_A, ENC2_B, layoutCW, layoutCCW);

    MacroPad.setLayoutCount(3);

    // ── Bind Layout 0 (App Launcher) ─────────────────────────────────────────
    MacroPad.bindButton(btns[0], 0, appArc,      "Arc");
    MacroPad.bindButton(btns[1], 0, appSpotify,  "Spotify");
    MacroPad.bindButton(btns[2], 0, appCapCut,   "CapCut");
    MacroPad.bindButton(btns[3], 0, appOBS,      "OBS");
    MacroPad.bindButton(btns[4], 0, appNotion,   "Notion");
    MacroPad.bindButton(btns[5], 0, appDiscord,  "Discord");
    MacroPad.bindButton(btns[6], 0, appWhatsApp, "WhatsApp");
    MacroPad.bindButton(btns[7], 0, appEasyEDA,  "EasyEDA");
    MacroPad.bindButton(btns[8], 0, appWarp,     "Warp");

    // ── Bind Layout 1 (Media) ─────────────────────────────────────────────────
    MacroPad.bindButton(btns[0], 1, mediaPlayPause, "Play/Pause");
    MacroPad.bindButton(btns[1], 1, mediaNext,      "Next Track");
    MacroPad.bindButton(btns[2], 1, mediaPrev,      "Prev Track");

    // ── Bind Layout 2 (Dev Tools) ─────────────────────────────────────────────
    MacroPad.bindButton(btns[0], 2, devNv,             "nv");
    MacroPad.bindButton(btns[1], 2, devTmNew,          "tm new");
    MacroPad.bindButton(btns[2], 2, devLg,             "lg");
    MacroPad.bindButton(btns[3], 2, devArduinoCompile, "Compiling");
    MacroPad.bindButton(btns[4], 2, devArduinoUpload,  "Uploading");
    MacroPad.bindButton(btns[5], 2, devArduinoMonitor, "Monitoring");
    MacroPad.bindButton(btns[6], 2, devESP32Compile,   "ESP Compile");
    MacroPad.bindButton(btns[7], 2, devESP32Upload,    "ESP Upload");
    MacroPad.bindButton(btns[8], 2, devESP32Monitor,   "ESP Monitor");

    // Start BLE + OLED
    MacroPad.begin("My MacroPad");
}

void loop() {
    MacroPad.update();
}
