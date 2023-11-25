#define LAYOUT_US_ENGLISH

#include <HID-Project.h>
#include <HID-Settings.h>

#include <Bounce2.h>

#include <Wire.h>
#include <Encoder.h>

#define SDA_PIN             2
#define SCI_PIN             3
#define ROW1                4
#define ROW2                5
#define ROW3                6
#define ROW4                7
#define ROW5                8
#define COL14               9
#define COL13               10
#define COL12               16
#define COL11               14
#define COL10               15
#define COL9                18
#define COL8                19
#define ENCODER_PIN_A       20
#define ENCODER_PIN_B       21

#define SLAVE_ADDRESS       0x1
#define MASTER_ADDRESS      0x2

#define ROW                 5
#define COL                 7

#define BUFFER_SIZE         10

#define DEBOUNCE_DELAY      3
#define LOOP_DELAY          200
#define ENCODER_THRESHOLD   1

struct KeyPressReport {
  KeyboardKeycode keysPressed[BUFFER_SIZE];
  KeyboardKeycode keysReleased[BUFFER_SIZE];
};

KeyPressReport incomingReport;

const int cols[] = {COL8, COL9, COL10, COL11, COL12, COL13, COL14};
const int rows[] = {ROW1, ROW2, ROW3, ROW4, ROW5};

Bounce debouncers[ROW][COL];

const KeyboardKeycode QWERTY[ROW][COL] = {
  {DUMMY_KEY,                KEY_6,              KEY_7,         KEY_8,              KEY_9,              KEY_0,                KEY_MINUS},
  {DUMMY_KEY,                KEY_Y,              KEY_U,         KEY_I,              KEY_O,              KEY_P,                KEY_EQUAL},
  {DUMMY_KEY,                KEY_H,              KEY_J,         KEY_K,              KEY_L,              KEY_SEMICOLON,        KEY_RETURN},
  {DUMMY_KEY,                KEY_N,              KEY_M,         KEY_COMMA,          KEY_PERIOD,         KEY_SLASH,            KEY_BACKSLASH},
  {KEY_F14,                  KEY_BACKSPACE,      KEY_F11,       KEY_INSERT,         KEY_DELETE,         KEY_F12,              KEY_RIGHT_CTRL}
};

const KeyboardKeycode COLEMAK[ROW][COL] = {
  {DUMMY_KEY,                KEY_6,              KEY_7,         KEY_8,              KEY_9,              KEY_0,                KEY_MINUS},
  {DUMMY_KEY,                KEY_J,              KEY_L,         KEY_U,              KEY_Y,              KEY_SEMICOLON,        KEY_EQUAL},
  {DUMMY_KEY,                KEY_H,              KEY_N,         KEY_E,              KEY_I,              KEY_O,                KEY_RETURN},
  {DUMMY_KEY,                KEY_K,              KEY_M,         KEY_COMMA,          KEY_PERIOD,         KEY_SLASH,            KEY_BACKSLASH},
  {KEY_F14,                  KEY_BACKSPACE,      KEY_F11,       KEY_INSERT,         KEY_DELETE,         KEY_F12,              KEY_RIGHT_CTRL}
};

const KeyboardKeycode LAYER2[ROW][COL] = {
  {DUMMY_KEY,            KEY_F6,                KEY_F7,              KEY_F8,                  KEY_F9,                 KEY_F10,                  DUMMY_KEY},
  {DUMMY_KEY,            DUMMY_KEY,             DUMMY_KEY,           KEY_UP_ARROW,            DUMMY_KEY,              DUMMY_KEY,                DUMMY_KEY},
  {DUMMY_KEY,            KEY_END,               KEY_LEFT_ARROW,      KEY_DOWN_ARROW,          KEY_RIGHT_ARROW,        KEY_MUTE,                 KEY_RETURN},
  {DUMMY_KEY,            DUMMY_KEY,             DUMMY_KEY,           DUMMY_KEY,               DUMMY_KEY,              DUMMY_KEY,                DUMMY_KEY},
  {KEY_F14,              KEY_BACKSPACE,         KEY_F11,             DUMMY_KEY,               DUMMY_KEY,              KEY_F12,                  KEY_RIGHT_CTRL}
};

KeyboardKeycode keys[ROW][COL];

bool keyboard_type = false;  // True = QWERTY; False = Colemak
bool layer_type = false;

Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);
long oldPosition = -999;

void switch_keyboard() {

  for(size_t i = 0; i < ROW; i++) {
    for(size_t j = 0; j < COL; j++) {
      if (keyboard_type) keys[i][j] = QWERTY[i][j];
      else keys[i][j] = COLEMAK[i][j];
    }
  }
  keyboard_type = !keyboard_type;
}

void switch_layer() {

  for(size_t i = 0; i < ROW; i++) {
    for(size_t j = 0; j < COL; j++) {
      if (!layer_type) keys[i][j] = LAYER2[i][j];
      else if (keyboard_type) 
        keys[i][j] = COLEMAK[i][j];
      else if (!keyboard_type)
        keys[i][j] = QWERTY[i][j];
    }
  }

  layer_type = !layer_type;
}

void scan_matrix(){

  for (int col = 0; col < COL; col++) {
    digitalWrite(cols[col], LOW);

    for (int row = 0; row < ROW; row++) {
      debouncers[row][col].update();


      if (debouncers[row][col].fell()) {

        if (keys[row][col] == KEY_RIGHT_CTRL)  {
          Wire.beginTransmission(SLAVE_ADDRESS);
          Wire.write('S');
          Wire.endTransmission();
          switch_keyboard();
        }

        if (keys[row][col] == KEY_F14) {
          Wire.beginTransmission(SLAVE_ADDRESS);
          Wire.write('F');
          Wire.endTransmission();
          switch_layer();
        }

        if (keys[row][col] == KEY_MUTE) {
          Consumer.write(MEDIA_VOL_MUTE);
        }

        NKROKeyboard.press(keys[row][col]);
      }
      
      if (debouncers[row][col].rose()) {
        if (keys[row][col] == KEY_F14) {
          Wire.beginTransmission(SLAVE_ADDRESS);
          Wire.write('F');
          Wire.endTransmission();
          switch_layer();
        }

        NKROKeyboard.release(keys[row][col]);
      }
    }
    digitalWrite(cols[col], HIGH);
  }
}

void processKeyPressReport() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (incomingReport.keysPressed[i] != DUMMY_KEY && incomingReport.keysPressed[i] != 255 && incomingReport.keysPressed[i] != 0) {
      NKROKeyboard.press(incomingReport.keysPressed[i]);
    }

    if (incomingReport.keysReleased[i] != DUMMY_KEY && incomingReport.keysReleased[i] != 255 && incomingReport.keysReleased[i] != 0) {
      NKROKeyboard.release(incomingReport.keysReleased[i]);
    }
  }

}

void scan_slave() {
  Wire.requestFrom(SLAVE_ADDRESS, sizeof(KeyPressReport));
  Wire.readBytes((char*)&incomingReport, sizeof(KeyPressReport));
  processKeyPressReport();

}

void scan_encoder() {
  long newPosition = myEnc.read();

  if (abs(newPosition - oldPosition) > ENCODER_THRESHOLD) {
    // For example, if the position increases, send a volume up signal
    if (newPosition < oldPosition) {
      //Serial.println(newPosition);
      Consumer.write(MEDIA_VOLUME_UP);
    }
    // If the position decreases, send a volume down signal
    else if (newPosition > oldPosition) {
      //Serial.println(newPosition);
      Consumer.write(MEDIA_VOLUME_DOWN);
    }

    oldPosition = newPosition;
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

  switch_keyboard();

  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCI_PIN, INPUT_PULLUP);

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  Serial.begin(9600);

  Wire.begin();
  NKROKeyboard.begin();
  Consumer.begin();
}

void loop() {
  
  scan_matrix();
  scan_slave();
  scan_encoder();
  delayMicroseconds(LOOP_DELAY);
}