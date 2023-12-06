#define LAYOUT_US_ENGLISH

#include <HID-Project.h>
#include <HID-Settings.h>

#include <Bounce2.h>
#include <Wire.h>
// #include <Encoder.h>

#define SDA_PIN 2
#define SCI_PIN 3
#define ROW1 4
#define ROW2 5
#define ROW3 6
#define ROW4 7
#define ROW5 8
#define COL1 9
#define COL2 10
#define COL3 16
#define COL4 14
#define COL5 15
#define COL6 18
#define COL7 19
#define ENCODER_PIN_A 20
#define ENCODER_PIN_B 21

#define SLAVE_ADDRESS 0x1

#define ROW 5
#define COL 7

#define BUFFER_SIZE 10

struct KeyPressReport {
    KeyboardKeycode keysPressed[BUFFER_SIZE];
    KeyboardKeycode keysReleased[BUFFER_SIZE];
};

KeyPressReport outgoingReport;

const int cols[] = {COL1, COL2, COL3, COL4, COL5, COL6, COL7};
const int rows[] = {ROW1, ROW2, ROW3, ROW4, ROW5};

#define DEBOUNCE_DELAY 3
#define ENCODER_THRESHOLD 1

Bounce debouncers[ROW][COL];

const KeyboardKeycode QWERTY[ROW][COL] = {
    {KEY_TILDE,       KEY_1,        KEY_2,      KEY_3,        KEY_4,        KEY_5,        DUMMY_KEY},
    {KEY_TAB,         KEY_Q,        KEY_W,      KEY_E,        KEY_R,        KEY_T,        DUMMY_KEY},
    {KEY_ESC,         KEY_A,        KEY_S,      KEY_D,        KEY_F,        KEY_G,        DUMMY_KEY},
    {KEY_LEFT_SHIFT,  KEY_Z,        KEY_X,      KEY_C,        KEY_V,        KEY_B,        DUMMY_KEY},
    {KEY_LEFT_CTRL,   KEY_LEFT_GUI, KEY_INSERT, KEY_DELETE,   KEY_LEFT_ALT, KEY_SPACE,    KEY_QUOTE}};

const KeyboardKeycode COLEMAK[ROW][COL] = {
    {KEY_TILDE,       KEY_1,        KEY_2,      KEY_3,        KEY_4,        KEY_5,      DUMMY_KEY},
    {KEY_TAB,         KEY_Q,        KEY_W,      KEY_F,        KEY_P,        KEY_G,      DUMMY_KEY},
    {KEY_ESC,         KEY_A,        KEY_R,      KEY_S,        KEY_T,        KEY_D,      DUMMY_KEY},
    {KEY_LEFT_SHIFT,  KEY_Z,        KEY_X,      KEY_C,        KEY_V,        KEY_B,      DUMMY_KEY},
    {KEY_LEFT_CTRL,   KEY_LEFT_GUI, KEY_INSERT, KEY_DELETE,   KEY_LEFT_ALT, KEY_SPACE,  KEY_QUOTE}};

const KeyboardKeycode PUNCUATION_LAYER[ROW][COL] = {
    {KEY_CAPS_LOCK,   KEY_F1,       KEY_F2,       KEY_F3,         KEY_F4,           KEY_F5,             DUMMY_KEY},
    {KEY_TAB,         KEY_7,        KEY_8,        KEY_9,          KEY_0,            DUMMY_KEY,          DUMMY_KEY},
    {KEY_ESC,         KEY_4,        KEY_5,        KEY_6,          KEY_LEFT_BRACE,   KEY_RIGHT_BRACE,    DUMMY_KEY},
    {KEY_LEFT_SHIFT,  KEY_1,        KEY_2,        KEY_3,          DUMMY_KEY,        DUMMY_KEY,          DUMMY_KEY},
    {KEY_LEFT_CTRL,   DUMMY_KEY,    DUMMY_KEY,    DUMMY_KEY,      KEY_LEFT_ALT,     KEY_SPACE,          DUMMY_KEY}};

KeyboardKeycode keys[ROW][COL];

// Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);
// long oldPosition = -999;

bool layer_type = false;
bool puncuation_layer = false;

void switch_keyboard() {

    for (size_t i = 0; i < ROW; i++) {
        for (size_t j = 0; j < COL; j++) {
            if (puncuation_layer)
                keys[i][j] = QWERTY[i][j];
            else
                keys[i][j] = COLEMAK[i][j];
        }
    }
    puncuation_layer = !puncuation_layer;
}

void switch_punLayer() {

    for (size_t i = 0; i < ROW; i++) {
        for (size_t j = 0; j < COL; j++) {
            NKROKeyboard.release(keys[i][j]);
            if (!layer_type) {
                keys[i][j] = PUNCUATION_LAYER[i][j];
            } else if (puncuation_layer) {
                keys[i][j] = COLEMAK[i][j];
            } else if (!puncuation_layer) {
                keys[i][j] = QWERTY[i][j];
            }
        }
    }

    layer_type = !layer_type;
}

void clearOutgoingReport() {
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        outgoingReport.keysPressed[i] = DUMMY_KEY;
        outgoingReport.keysReleased[i] = DUMMY_KEY;
    }
}

void sendKeyPressReport() {
    Wire.write((char *)&outgoingReport, sizeof(KeyPressReport));
}

void scan_matrix() {
    int index_pressed = 0;
    int index_released = 0;

    for (int col = 0; col < COL; col++) {
        digitalWrite(cols[col], LOW);

        for (int row = 0; row < ROW; row++) {
            debouncers[row][col].update();

            // Handle Keys
            if (debouncers[row][col].fell() && index_pressed < BUFFER_SIZE && keys[row][col] != DUMMY_KEY) {
                outgoingReport.keysPressed[index_pressed++] = keys[row][col];
            }

            if (debouncers[row][col].rose() && index_released < BUFFER_SIZE && keys[row][col] != DUMMY_KEY) {
                outgoingReport.keysReleased[index_released++] = keys[row][col];
            }
        }
        digitalWrite(cols[col], HIGH);
    }

    outgoingReport.keysPressed[index_pressed] = DUMMY_KEY;
    outgoingReport.keysReleased[index_released] = DUMMY_KEY;
}

void requestEvent() {
    scan_matrix();
    sendKeyPressReport();
    clearOutgoingReport();
}

void receiveEvent(int howMany) {
    while (Wire.available()) { // loop through all but the last byte
        char c = Wire.read();  // receive byte as a character

        switch (c) {
        case 'S':
          switch_keyboard();
          break;
        case 'F':
          switch_punLayer();
          break;
        case 'N':
          switch_keyboard();
          break;
        
        default: break;
        }
    }
}

void setup() {
    // Initialize column pins as OUTPUT and set them HIGH
    for (int i = 0; i < COL; i++) {
        pinMode(cols[i], OUTPUT);
        digitalWrite(cols[i], HIGH);
    }

    // Initialize row pins as INPUT_PULLUP and setup the debouncers
    for (int row = 0; row < ROW; row++) {
        pinMode(rows[row], INPUT_PULLUP);

        for (int col = 0; col < COL; col++) {
            debouncers[row][col].attach(rows[row]);
            debouncers[row][col].interval(DEBOUNCE_DELAY);
        }
    }

    // Clear outgoingReport
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        outgoingReport.keysPressed[i] = DUMMY_KEY;
        outgoingReport.keysReleased[i] = DUMMY_KEY;
    }

    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);

    switch_keyboard();
    clearOutgoingReport();

    Serial.begin(9600);

    Wire.begin(SLAVE_ADDRESS);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
}

void loop() {
}
