#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ==== OLED ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
const uint8_t OLED_ADDR = 0x3C;

// ==== Pins ====
#define BUTTON_PIN 33   // single control button (INPUT_PULLUP; avoid GPIO0)
#define BUZZER_PIN 25   // active buzzer I/O (3-pin: VCC, I/O, GND)
#define LED1      16
#define LED2      17
#define LED3      18

// ==== Buzzer polarity ====
#define BUZZER_ACTIVE_LOW 1
inline void buzzerOn()  { digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_LOW ? LOW  : HIGH); }
inline void buzzerOff() { digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_LOW ? HIGH : LOW ); }
static void tick(uint16_t ms=6){ buzzerOn(); delay(ms); buzzerOff(); }

// ==== Data ====
const char* COLORS[6] = {"RED","GREEN","BLUE","YELLOW","WHITE","PINK"};
static char colorInitialOf(int idx){
  switch(idx){ case 0:return 'R'; case 1:return 'G'; case 2:return 'B';
               case 3:return 'Y'; case 4:return 'W'; case 5:return 'P'; default:return '?'; }
}

// ==== Layout (font size 1 everywhere) ====
// Header line (~8px tall)
const int HEADER_Y = 0;
const int TOP_Y    = 14;  // row of top content
const int V_GAP    = 6;   // gap to bottom
const int BOX_H    = 22;

const int MARGIN = 4;
const int H_GAP  = 4;
const int TOP_W  = (SCREEN_WIDTH - 2*MARGIN - H_GAP) / 2; // 58
const int TOP_X1 = MARGIN;                                // 4
const int TOP_X2 = TOP_X1 + TOP_W + H_GAP;                // 66

const int BOT_W  = 80;
const int BOT_X  = (SCREEN_WIDTH - BOT_W) / 2;
const int BOT_Y  = TOP_Y + BOX_H + V_GAP;

// ==== App state ====
enum Mode     { MODE_COLOR, MODE_DICE };
enum AppState { STATE_STARTUP, STATE_SELECT, STATE_PLAY };
AppState appState = STATE_STARTUP;
Mode     mode     = MODE_COLOR;

// Last summaries for header
#define PH '*'
char lastA = PH, lastB = PH, lastC = PH;   // COLOR: initials; DICE: digits '1'..'6'

// Rolling engine
bool     rolling = false;            // true while animating
uint32_t nextSpinMs = 0;             // next animation update
uint32_t rollingStartMs = 0;         // when rolling began
const uint32_t AUTO_LOCK_MS = 7000;  // auto-stop after 7s
uint8_t  rA=0, rB=0, rC=0;           // current values (0..5 colors, 1..6 dice faces)

// Button handling
uint32_t btnDownAt = 0;
bool     btnWasDown = false;

// ===== Forward declarations =====
void showStartup();
void showSelect(uint8_t caret);

// ===== Helpers: drawing =====
void drawBox(int x,int y,int w,int h){ display.drawRect(x,y,w,h,SH110X_WHITE); }

void drawCenteredText(const char* txt, int y, uint16_t fg=SH110X_BLACK, uint16_t bg=SH110X_WHITE){
  int16_t x1,y1; uint16_t tw,th;
  display.setTextSize(1); display.setTextColor(fg, bg);
  display.getTextBounds(txt,0,0,&x1,&y1,&tw,&th);
  int tx = (SCREEN_WIDTH - (int)tw)/2; if (tx < 0) tx = 0;
  display.setCursor(tx, y);
  display.print(txt);
}

void drawCenteredTextInRect(int x,int y,int w,int h,const char* text, uint16_t fg=SH110X_WHITE, uint16_t bg=SH110X_BLACK){
  int16_t x1,y1; uint16_t tw,th;
  display.setTextSize(1); display.setTextColor(fg, bg);
  display.getTextBounds(text,0,0,&x1,&y1,&tw,&th);
  int tx = x + (w - (int)tw)/2; if(tx < x+1) tx = x+1;
  int ty = y + (h - (int)th)/2; if(ty < y+1) ty = y+1;
  display.setCursor(tx,ty); display.print(text);
}

void drawHeader(){
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, HEADER_Y);
  if (mode == MODE_COLOR) display.print("COLOR - ");
  else                    display.print("DICE - ");
  display.print(lastA); display.print("|"); display.print(lastB); display.print("|"); display.print(lastC);
}

// Dice graphics (with correct "6" as 3 columns x 2 rows)
// Color parameter for lines/pips so we can draw black on white in overlay.
void drawDieColored(int x,int y,int s,int face, uint16_t col){
  int r=3; display.drawRoundRect(x,y,s,s,r,col);
  auto pip=[&](int px,int py){ display.fillCircle(px,py,2,col); };

  int cx=x+s/2, cy=y+s/2;
  int dx=s/4, dy=s/4;
  int lx=x+dx, rx=x+s-dx, mx=cx;
  int ty=y+dy, by=y+s-dy;

  switch(face){
    case 1: pip(cx,cy); break;
    case 2: pip(lx,ty); pip(rx,by); break;
    case 3: pip(lx,ty); pip(cx,cy); pip(rx,by); break;
    case 4: pip(lx,ty); pip(rx,ty); pip(lx,by); pip(rx,by); break;
    case 5: pip(lx,ty); pip(rx,ty); pip(cx,cy); pip(lx,by); pip(rx,by); break;
    case 6: pip(lx,ty); pip(lx,by); pip(mx,ty); pip(mx,by); pip(rx,ty); pip(rx,by); break;
    default: break;
  }
}
inline void drawDie(int x,int y,int s,int face){ drawDieColored(x,y,s,face, SH110X_WHITE); }

// ===== Screens =====
void showStartup(){
  display.clearDisplay();
  // three dice art centered
  int s=18, gap=8, totalW=s*3+gap*2, startX=(SCREEN_WIDTH-totalW)/2, y=6;
  drawDie(startX + 0*(s+gap), y, s, 1);
  drawDie(startX + 1*(s+gap), y, s, 2);
  drawDie(startX + 2*(s+gap), y, s, 3);

  // text (size 1)
  display.setTextSize(1); display.setTextColor(SH110X_WHITE);
  display.setCursor((SCREEN_WIDTH-11*6)/2, 32); display.print("COLOR GAME");
  display.setCursor((SCREEN_WIDTH-9*6)/2,  44); display.print("BY JRCSRG");
  display.setCursor((SCREEN_WIDTH-13*6)/2, 56); display.print("SEPTEMBER 2025");
  display.display();
}

void showSelect(uint8_t caret){ // caret: 0=color, 1=dice
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0); display.print("SELECT MODE");
  display.setCursor(8, 18); display.print(caret==0?"> ":"  "); display.print("COLOR");
  display.setCursor(8, 32); display.print(caret==1?"> ":"  "); display.print("DICE");
  display.display();
}

void renderFrameColorOnly(){
  display.clearDisplay();
  drawHeader();
  // Color mode keeps boxes
  drawBox(TOP_X1, TOP_Y, TOP_W, BOX_H);
  drawBox(TOP_X2, TOP_Y, TOP_W, BOX_H);
  drawBox(BOT_X,  BOT_Y, BOT_W, BOX_H);
  display.display();
}

void renderRollingOrFinal(bool final=false){
  display.clearDisplay();
  drawHeader();

  if (mode == MODE_COLOR){
    // boxes with color names
    drawBox(TOP_X1, TOP_Y, TOP_W, BOX_H);
    drawBox(TOP_X2, TOP_Y, TOP_W, BOX_H);
    drawBox(BOT_X,  BOT_Y, BOT_W, BOX_H);
    drawCenteredTextInRect(TOP_X1, TOP_Y, TOP_W, BOX_H, COLORS[rA], SH110X_WHITE, SH110X_BLACK);
    drawCenteredTextInRect(TOP_X2, TOP_Y, TOP_W, BOX_H, COLORS[rB], SH110X_WHITE, SH110X_BLACK);
    drawCenteredTextInRect(BOT_X , BOT_Y, BOT_W, BOX_H, COLORS[rC], SH110X_WHITE, SH110X_BLACK);
  } else {
    // DICE mode: NO boxes, just dice centered in the same areas
    int sTop = 24; // fits visually in the top regions
    int sBot = 26; // larger for bottom
    int dx1 = TOP_X1 + (TOP_W - sTop)/2;
    int dy1 = TOP_Y  + (BOX_H - sTop)/2;
    int dx2 = TOP_X2 + (TOP_W - sTop)/2;
    int dy2 = TOP_Y  + (BOX_H - sTop)/2;
    int dx3 = BOT_X  + (BOT_W - sBot)/2;
    int dy3 = BOT_Y  + (BOX_H - sBot)/2;

    drawDie(dx1, dy1, sTop, (int)rA);
    drawDie(dx2, dy2, sTop, (int)rB);
    drawDie(dx3, dy3, sBot, (int)rC);
  }

  display.display();

  // LEDs + buzzer tick while rolling
  if (!final){
    digitalWrite(LED1, HIGH); delay(8); digitalWrite(LED1, LOW);
    digitalWrite(LED2, HIGH); delay(8); digitalWrite(LED2, LOW);
    digitalWrite(LED3, HIGH); delay(8); digitalWrite(LED3, LOW);
    tick(5);
  }
}

// ===== Random steppers =====
inline void stepColor(){ rA = random(0,6); rB = random(0,6); rC = random(0,6); }
inline void stepDice() { rA = random(1,7); rB = random(1,7); rC = random(1,7); }

// ===== Button (single) =====
bool buttonShortPressed(){
  bool down = (digitalRead(BUTTON_PIN)==LOW);
  uint32_t now = millis();
  if (down && !btnWasDown){ btnWasDown=true; btnDownAt=now; return false; }
  if (!down && btnWasDown){
    btnWasDown=false;
    if (now - btnDownAt >= 20 && now - btnDownAt < 3000) return true; // short
  }
  return false;
}
bool buttonLongPressed(){                 // fires once per hold
  if (btnWasDown && (millis() - btnDownAt >= 3000)) { btnWasDown=false; return true; }
  return false;
}

// ===== Inverted congratulations overlay rendered as REAL RESULT =====
void showCongratsOverlayResult(){
  // Fill screen white
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);

  // Title (black on white)
  drawCenteredText("CONGRATULATIONS!", 2, SH110X_BLACK, SH110X_WHITE);

  // Reserve a small gap under title, then reuse same layout areas.
  // NOTE: we do NOT draw header here.
  if (mode == MODE_COLOR){
    // draw black boxes + black labels on white
    display.drawRect(TOP_X1, TOP_Y, TOP_W, BOX_H, SH110X_BLACK);
    display.drawRect(TOP_X2, TOP_Y, TOP_W, BOX_H, SH110X_BLACK);
    display.drawRect(BOT_X , BOT_Y, BOT_W, BOX_H, SH110X_BLACK);

    drawCenteredTextInRect(TOP_X1, TOP_Y, TOP_W, BOX_H, COLORS[rA], SH110X_BLACK, SH110X_WHITE);
    drawCenteredTextInRect(TOP_X2, TOP_Y, TOP_W, BOX_H, COLORS[rB], SH110X_BLACK, SH110X_WHITE);
    drawCenteredTextInRect(BOT_X , BOT_Y, BOT_W, BOX_H, COLORS[rC], SH110X_BLACK, SH110X_WHITE);
  } else {
    // draw three black dice (no boxes) on white
    int sTop = 24; int sBot = 26;
    int dx1 = TOP_X1 + (TOP_W - sTop)/2;
    int dy1 = TOP_Y  + (BOX_H - sTop)/2;
    int dx2 = TOP_X2 + (TOP_W - sTop)/2;
    int dy2 = TOP_Y  + (BOX_H - sTop)/2;
    int dx3 = BOT_X  + (BOT_W - sBot)/2;
    int dy3 = BOT_Y  + (BOX_H - sBot)/2;

    drawDieColored(dx1, dy1, sTop, (int)rA, SH110X_BLACK);
    drawDieColored(dx2, dy2, sTop, (int)rB, SH110X_BLACK);
    drawDieColored(dx3, dy3, sBot, (int)rC, SH110X_BLACK);
  }

  display.display();
}

void celebrateOverlayResult(){
  showCongratsOverlayResult();
  for (int j=0; j<3; j++){
    digitalWrite(LED1, HIGH); digitalWrite(LED2, HIGH); digitalWrite(LED3, HIGH);
    buzzerOn(); delay(120);
    digitalWrite(LED1, LOW);  digitalWrite(LED2, LOW);  digitalWrite(LED3, LOW);
    buzzerOff(); delay(120);
  }
}

// ===== Finalization =====
void stopAndLock(){
  rolling = false;

  // Update header summary for next round
  if (mode == MODE_COLOR){
    lastA = colorInitialOf(rA);
    lastB = colorInitialOf(rB);
    lastC = colorInitialOf(rC);
  } else {
    lastA = '0' + rA;
    lastB = '0' + rB;
    lastC = '0' + rC;
  }

  // (Optional) brief final frame; then full overlay result view
  renderRollingOrFinal(true);
  celebrateOverlayResult();
}

void startRolling(){
  rolling = true;
  nextSpinMs = 0;
  rollingStartMs = millis();
}

// ===== Setup / Loop =====
void setup(){
  Wire.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED1,OUTPUT); pinMode(LED2,OUTPUT); pinMode(LED3,OUTPUT);

  // keep buzzer OFF at boot
  if (BUZZER_ACTIVE_LOW) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT); buzzerOff();

  if (!display.begin(OLED_ADDR, true)) { display.begin(0x3D, true); }
  display.clearDisplay(); display.setRotation(0);

  randomSeed(esp_random());

  // Boot: show startup
  lastA = lastB = lastC = PH;
  mode = MODE_COLOR;
  appState = STATE_STARTUP;
  showStartup();
}

void loop(){
  // Long press (>=3s) always goes to SELECT (except during STARTUP before any press)
  if (appState != STATE_STARTUP && buttonLongPressed()){
    appState = STATE_SELECT;
    static uint8_t caret=0; caret=0;
    showSelect(caret);
    delay(250);
    return;
  }

  // STARTUP -> SELECT on short press
  if (appState == STATE_STARTUP){
    if (buttonShortPressed()){
      appState = STATE_SELECT;
      static uint8_t caret=0; caret=0;
      showSelect(caret);
    }
    return;
  }

  // SELECT: caret toggles every 2s, short press chooses
  if (appState == STATE_SELECT){
    static uint8_t caret = 0;
    static uint32_t lastSwap = 0;
    uint32_t now = millis();
    if (now - lastSwap >= 2000){
      caret ^= 1; lastSwap = now; showSelect(caret);
    }
    if (buttonShortPressed()){
      mode = (caret==0) ? MODE_COLOR : MODE_DICE;
      // seed first values
      if (mode == MODE_COLOR){ stepColor(); }
      else                   { stepDice();  }
      appState = STATE_PLAY;
      rolling = false;
      // show initial static view
      renderRollingOrFinal(true);
    }
    return;
  }

  // PLAY
  if (buttonShortPressed()){
    if (!rolling){
      // start rolling
      startRolling();
    } else {
      // manual stop overrides auto 7s: lock now + overlay result
      stopAndLock();
    }
  }

  // Animate if rolling
  if (appState == STATE_PLAY && rolling){
    uint32_t now = millis();
    if (now >= nextSpinMs){
      if (mode == MODE_COLOR) stepColor();
      else                    stepDice();
      renderRollingOrFinal(false);
      // pacing
      static uint32_t start=0; if (nextSpinMs==0) start=now;
      uint16_t step = (now - start < 800) ? 70 : 110;
      nextSpinMs = now + step;
    }
    // Auto-stop after 7 seconds -> lock + overlay result
    if (now - rollingStartMs >= AUTO_LOCK_MS){
      stopAndLock();
    }
  }
}
