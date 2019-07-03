
#include <Wire.h>
#include <I2C_Anything.h>


const uint8_t I2C_MASTER     = 23;
const uint8_t I2C_CONTROLLER = 42;

const uint8_t TRELLIS_COLS = 8;
const uint8_t TRELLIS_ROWS = 4;



/********************************************************************* KEY MAP
*                                    TOP                                     *
*     0         1        2        3        4        5        6        7      *
*   -------  -------  -------  -------  -------  -------  -------  -------   *
* 3                     CLR++    CLR--                                       *
*                                                                            *
* 2                   HUE_FCN           BRT_FCN                              *
*                                                                            *
* 1                                                                          *
*                                                                            *
* 0 ~ESC-L~           ~HUE-F~  ~HUE-M~  ~BRT-F~   ~BRT-M~          ~ESC-R~   *
*   -------  -------  -------  -------  -------  -------  -------  -------   *
*     0         1        2        3        4        5        6        7      *
*                                   BOTTOM                                   *
*****************************************************************************/


/*
0.5

button 1 down

- save orientation - O_START
- save parameter map

every 10 ms

while orientation < 30

  receive 30 / delta

  O_NOW

  O_delta = O_START - O_NOW

 
  P_OLD = get_old_p

  log_P_OLD = log P

  log_P = log_P_old + delta * scalar

  




over 3 seconds at 10ms resolution:
  rotate 30 degrees
*/



//=============================================================== DECLARATIONS

typedef void (*KeyCallback)(float, float);

namespace sillert {

  enum KeyState { UP, DOWN };
  enum KeyAction { PRESS, CLICK, HOLD, RELEASE };
  enum KeyAxis { ROLL, PITCH, YAW };

  //-------------- Key Button Class
  class Key {
    public:
      const uint8_t id;
      const uint8_t row;
      const uint8_t col;

      bool active = false;
      KeyAxis axis = ROLL;
      KeyState state = UP;

      long start_time = 0;
      long start_orient = 0;

      KeyCallback keyPress;
      KeyCallback keyClick;
      KeyCallback keyHold;
      KeyCallback keyRelease;

      Key(uint8_t id, uint8_t row, uint8_t col);
      
      void setCallback(KeyAction action, KeyCallback cb);
      void updateState(KeyState s);
      void setXYZ(int x, int y, int z);

  };

  //================================================================== SILLERT
  
  const uint8_t cols = TRELLIS_COLS;
  const uint8_t rows = TRELLIS_ROWS;
  const uint8_t keys = cols * rows;

  Key*  key[keys];          // key array
  Key*  matrix[rows][cols]; // 2D accessor to keys
  Key*  primary;            // currently active key on major axis
  Key*  secondary;          // currently active key on minor axis

  // Acceleration
  int x;
  int y;
  int z;

  // Orientation
  float roll;
  float pitch;
  float yaw;
  
  void setup();

  //---------- Receiving I2C input from Trellis
  volatile bool haveKey;
  volatile bool haveAcceleration;
  volatile bool haveOrientation;

  volatile uint8_t i2c_key;
  volatile uint8_t i2c_state;
  volatile int i2c_x;
  volatile int i2c_y;
  volatile int i2c_z;
  volatile float i2c_roll;
  volatile float i2c_pitch;
  volatile float i2c_yaw;
  
  void receive(int bytes);
  void process();

}


void sillert::Key::setCallback(KeyAction action, KeyCallback cb) {
  switch (action) {
    case PRESS:   keyPress   = cb; break;
    case CLICK:   keyClick   = cb; break;
    case HOLD:    keyHold    = cb; break;
    case RELEASE: keyRelease = cb; break;
    default: break;
  }
}




/* --------------------------------------------------------- STATE TRANSITIONS
 *  
 *
 *
 */


void sillert::Key::updateState(KeyState s) {
  state = s;
}


void sillert::Key::setXYZ(int x, int y, int z) {


};


//###################################################################### SETUP

void sillert::setup() {

  Wire.begin(I2C_MASTER);
  Wire.setClock(400000L);
//TWBR = 12;
  Wire.onReceive(receive);

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      int k = r * cols + c;
      Key* _key = new Key(k, r, c);
      key[k] = _key;
      matrix[r][c] = _key;
    }
  }
}



//==================================================================== RECEIVE
// Interrupt driven... grab data and GTFO.

void sillert::receive(int bytes) {

  volatile char command;
  I2C_readAnything(command);

  if (command == 'k') {
    I2C_readAnything(i2c_key);
    I2C_readAnything(i2c_state);
    haveKey = true;
  }

  if (command == 'a') {
    I2C_readAnything(i2c_x);
    I2C_readAnything(i2c_y);
    I2C_readAnything(i2c_z);
    haveAcceleration = true;
  }


  if (command == 'o') {
    I2C_readAnything(i2c_roll);
    I2C_readAnything(i2c_pitch);
    I2C_readAnything(i2c_yaw);
    haveOrientation = true;
  }
}



void sillert::process() {

  if (haveKey) {
    key[i2c_key]->updateState(i2c_state ? DOWN : UP);
    haveKey = false;
  }

  if (haveAcceleration) {
    x = i2c_x;
    y = i2c_y;
    z = i2c_z;
    haveAcceleration = false;
  }

}


