# ESP32 Color/Dice Game

Mini game for ESP32 with a **mode select** screen (Color or Dice), a **single control button**, an **SH1106 (128×64) OLED**, an **active buzzer**, and **3 LEDs**.

* **Color Game**: shows 2 boxes on the first row and 1 centered box below with color names.
* **Dice Game**: shows three dice (no boxes) in the same layout.
* **Play control**: short-press to start/stop rolling; **auto-stops after 7s** and shows a full-screen **CONGRATULATIONS** overlay with the **final result**; long-press (≥3s) returns to Mode Select.

---

## ✨ Features

* **Startup Screen** (dice art + title/credits)
* **Mode Select** (caret auto-toggles every 2 seconds; single press to choose)
* **Color Mode** (2 top boxes + 1 centered bottom; labels in font size 1)
* **Dice Mode** (only dice graphics; correct “6” pattern as 3 columns × 2 rows)
* **Single Button UX**

  * Short press: toggle rolling
  * Auto-lock: after **7 seconds** of rolling
  * Long press (≥3s): go back to **Mode Select**
* **Buzzer**: gentle tick while rolling; short celebratory beeps on win
* **Header** shows mode and last results (`R|G|Y` or `3|5|1`)

---

## 🧰 Hardware Required

* **ESP32 DevKit** (ESP32-DevKitC or similar)
* **OLED 128×64 SH1106** (I²C, 0x3C or 0x3D)
* **Active buzzer module (3-pin)** — VCC / I/O / GND

  > This code assumes **active-LOW** buzzer (LOW = ON). You can flip it via a `#define` (see below).
* **1 × Momentary push button**
* **3 × LEDs** (any color)
* **3 × 220 Ω resistors** (LED current limit)
* Breadboard & jumper wires
* USB cable

---

## 🔌 Wiring

### ESP32 Pins Used

| Part           | Pin        | Notes                                                                        |
| -------------- | ---------- | ---------------------------------------------------------------------------- |
| **OLED SDA**   | **GPIO21** | I²C data                                                                     |
| **OLED SCL**   | **GPIO22** | I²C clock                                                                    |
| **Button**     | **GPIO33** | Button to **GND**, uses internal pull-up                                     |
| **Buzzer I/O** | **GPIO25** | 3-pin buzzer’s signal pin                                                    |
| **LED1**       | **GPIO16** | Through 220 Ω to LED anode                                                   |
| **LED2**       | **GPIO17** | Through 220 Ω to LED anode                                                   |
| **LED3**       | **GPIO18** | Through 220 Ω to LED anode                                                   |
| **3.3 V**      | —          | OLED VCC, buzzer VCC (if 3.3 V), LED anodes via resistors (or use GPIO side) |
| **GND**        | —          | Common ground for all devices                                                |

> OLED address defaults to **0x3C**; code falls back to **0x3D** if needed.

### Button (single)

* One leg to **GPIO33**
* Other leg to **GND**
* No external resistor needed (uses `INPUT_PULLUP`)

### LEDs (x3)

Typical wiring (active-HIGH):
`GPIOx → 220Ω → LED anode, LED cathode → GND`

### Buzzer (active module, 3-pin)

* **VCC** → **3.3 V**
* **I/O** → **GPIO25**
* **GND** → **GND**

**Polarity note:** This sketch assumes **active-LOW** buzzer boards (LOW = sound ON).
If your buzzer is **active-HIGH** (HIGH = ON), change this line in `main.cpp`:

```cpp
#define BUZZER_ACTIVE_LOW 1   // change to 0 for active-HIGH buzzers
```

---

## 🗂 Project Structure

```
.
├─ src/
│  └─ main.cpp      // full game logic (color & dice modes)
├─ platformio.ini
└─ README.md
```

### Suggested `platformio.ini`

```ini
[env:esp32dev]
platform = espressif32@6.12.0
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600

lib_deps =
  adafruit/Adafruit GFX Library @ ^1.12.3
  adafruit/Adafruit SH110X @ ^2.1.14
  tzapu/WiFiManager @ ^2.0.17

; If you previously used src_filter, replace with:
; build_src_filter = +<src/>
```

---

## ▶️ How to Build & Upload (PlatformIO)

1. **Clone** this repo or copy files into a new PlatformIO project.
2. Ensure **USB serial** is selected for your ESP32 board.
3. Click **Build** then **Upload** in your IDE (VS Code + PlatformIO), or run:

   ```
   pio run -t upload
   ```
4. Open **Serial Monitor** at **115200** baud:

   ```
   pio device monitor -b 115200
   ```

---

## 🎮 How to Play

1. **Boot**: You’ll see a start screen (dice art + title).
2. **Short press** → **Mode Select** screen (`> COLOR` / `> DICE`), caret switches every **2s**.
3. **Short press** to choose the highlighted mode.
4. **Play screen**:

   * **Short press**: Start rolling (colors/dice animate).
   * **Short press again**: Stop immediately and show the **CONGRATULATIONS** overlay with the final result layout (boxes/dice in black on white).
   * **Do nothing**: After **7 seconds**, it **auto-stops** and shows the same overlay + celebration.
5. **Long press (≥3s)** at any time after startup → back to **Mode Select**.

---

## 🧩 Customization

* **Change pins**: edit the `#define` pin numbers at the top of `main.cpp`.
* **Buzzer behavior**: set `BUZZER_ACTIVE_LOW` to `0` if your module is active-HIGH.
* **Speeds**: tweak animation pacing and auto-lock time (`AUTO_LOCK_MS = 7000`).
* **OLED address**: defaults to `0x3C`, automatic fallback to `0x3D`. Modify in `setup()` if needed.

---

## 🛠 Troubleshooting

* **Buzzer is constantly ON**
  Your module might be **active-HIGH**. Set:

  ```cpp
  #define BUZZER_ACTIVE_LOW 0
  ```

  Also confirm the buzzer **I/O** is on **GPIO25**, and VCC/GND are correct.

* **OLED shows nothing / scrambled**

  * Confirm **SDA=GPIO21**, **SCL=GPIO22**, and **3.3 V/GND**.
  * Many “0.96in” displays are actually **SH1106**, not SSD1306. Ensure the library is **Adafruit SH110X** and you create `Adafruit_SH1106G`.
  * Try the fallback address **0x3D** (code already tries both).

* **LEDs don’t blink**

  * Check 220 Ω resistors and orientation; try swapping anode/cathode wiring.
  * Verify pins 16/17/18 aren’t used by other peripherals in your setup.

* **Button doesn’t respond**

  * Confirm one leg is on **GPIO33** and the other is on **GND**.
  * Pin mode is `INPUT_PULLUP` (internal pull-up), so idle reads **HIGH**, pressed reads **LOW**.

---

## 📸 Screens / Layout (textual)

* **Header**:
  `COLOR - *|*|*` or `DICE - *|*|*` (updated after each round, e.g., `R|B|G` or `3|5|1`)

* **Color Mode** (during rolling):

  ```
  [  RED  ] [ GREEN ]
      [  BLUE  ]
  ```

* **Dice Mode** (during rolling):
  Three dice only (no boxes); the **“6”** shows **three columns × two rows** of pips.

* **Win Overlay** (both modes):
  Full white background, **CONGRATULATIONS!** at the top, and the final result rendered exactly like the live view (boxes + labels for Color; black dice for Dice).

---

## 📄 License

MIT — do anything you want, just keep the copyright/permission notice.

Copyright (c) 2025 Mat Jerico Sergio

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## 🙌 Credits

* **Code & layout requests** by **JRCSRG**
* Built with **ESP32 Arduino**, **Adafruit GFX**, and **Adafruit SH110X**
