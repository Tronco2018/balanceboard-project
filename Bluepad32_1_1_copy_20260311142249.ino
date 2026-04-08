#include <Bluepad32.h>

// ## COSTANTI ##
#define N_BIT 8
#define V_MAX_CELL 40000

#define SOGLIA_AVANTI 15000
#define SOGLIA_DIETRO 20000

#define AVANTI 100
#define DIETRO 200

// ## BATTERIE ##
#define MIN_STATUS 0
#define MAX_STATUS 4095

#define FIRST_STEP_PERCENTAGE 0.5
#define LAST_STEP_PERCENTAGE 0.33

#define BATTERY_HALF 1
#define BATTERY_LOW 2
#define BATTERY_HEALTHY 0

// BATTERY_CHECKER => Controllo carica delle batterie
int battery_checker(int v_min, int v_max, int input_pin) {
  int out_val = map(analogRead(input_pin), MIN_STATUS, MAX_STATUS, v_min, v_max);
  
  if (out_val <= v_max * FIRST_STEP_PERCENTAGE && out_val > v_max * LAST_STEP_PERCENTAGE) return BATTERY_HALF; // ~Meta'
  else if (out_val <= v_max * LAST_STEP_PERCENTAGE) return BATTERY_LOW; // 33%
  else return BATTERY_HEALTHY; // >50%
}

// ---

// Accensione/Spegnimento 
#define CTRL_POWER_PIN -1 
#define PH_POWER_PIN -1

// Lettura tensione
#define CTRL_READ_PIN -1
#define PH_READ_PIN -1 

// Pin LED
#define CTRL_OUT_PIN -1
#define PH_OUT_PIN -1

#define V_MAX 3.3
#define V_MIN 0.0

// BATTERCY_CTL_INIT
void battery_ctl_init()
{
  pinMode(CTRL_POWER_PIN, OUTPUT);
  pinMode(PH_POWER_PIN, OUTPUT);

  pinMode(CTRL_READ_PIN, INPUT);
  pinMode(PH_READ_PIN, INPUT);

  pinMode(CTRL_OUT_PIN, OUTPUT);
  pinMode(PH_OUT_PIN, OUTPUT);

  digitalWrite(PH_OUT_PIN, LOW);
  digitalWrite(PH_POWER_PIN, HIGH);

  digitalWrite(CTRL_OUT_PIN, LOW);
  digitalWrite(CTRL_POWER_PIN, HIGH);
}

// MAIN_BATTERY => Funzione principale per il controllo della carica delle batterie
void main_battery() {
  battery_ctl_init();

  // H_BRIDGE PHASE
  uint8_t status = battery_checker(V_MIN, V_MAX, PH_READ_PIN);

  if (status == BATTERY_HALF) digitalWrite(PH_OUT_PIN, HIGH);
  else if (status == BATTERY_LOW) digitalWrite(PH_POWER_PIN, LOW);

  // ESP PHASE
  status = battery_checker(V_MIN, V_MAX, CTRL_READ_PIN);

  if (status == BATTERY_HALF) digitalWrite(CTRL_OUT_PIN, HIGH);
  else if (status == BATTERY_LOW) digitalWrite(CTRL_POWER_PIN, LOW);
}

// ## MOVIMENTO ##
// Definizione pin ponte ad H
const int frontLeftA = 17;
const int frontLeftB = 16;
const int frontRightA = 32;
const int frontRightB = 33;
const int backLeftA = 25;
const int backLeftB = 26;
const int backRightA = 27;
const int backRightB = 14;

// Funzione per movimenti dei motori di sinistra con parametro direzione
void sinistra_dir(int dir) {
  if (dir == AVANTI) {
    analogWrite(frontLeftA, 0);
    analogWrite(frontLeftB, pow(2, N_BIT)-1);
    
    analogWrite(backLeftB, 0);
    analogWrite(backLeftA, pow(2, N_BIT)-1);
  } else if (dir == DIETRO) {
    analogWrite(frontLeftA, pow(2, N_BIT)-1);
    analogWrite(frontLeftB, 0);
    
    analogWrite(backLeftB, pow(2, N_BIT)-1);
    analogWrite(backLeftA, 0);
  } else {
    analogWrite(frontLeftA, 0);
    analogWrite(frontLeftB, 0);
    
    analogWrite(backLeftB, 0);
    analogWrite(backLeftA, 0);
  }
}
// Funzione per movimenti dei motori di destra con parametro direzione
void destra_dir(int dir) {
    if (dir == AVANTI) {
    analogWrite(frontRightA, 0);
    analogWrite(frontRightB, pow(2, N_BIT)-1);
    
    analogWrite(backRightB, 0);
    analogWrite(backRightA, pow(2, N_BIT)-1);
  } else if (dir == DIETRO) {
    analogWrite(frontRightA, pow(2, N_BIT)-1);
    analogWrite(frontRightB, 0);
    
    analogWrite(backRightB, pow(2, N_BIT)-1);
    analogWrite(backRightA, 0);
  } else {
    analogWrite(frontRightA, 0);
    analogWrite(frontRightB, 0);
    
    analogWrite(backRightB, 0);
    analogWrite(backRightA, 0);
  }
}

//## FUNZIONE MOVIMENTO ##
void handle_movement(int left_direction, int right_direction) {
  // DIR 100 => AVANTI, 200 => DIETRO
  // FRONT => A = DIETRO, B = AVANTI
  // BACK => A = AVANTI, B = DIETRO
  sinistra_dir(left_direction);
  destra_dir(right_direction);
}


ControllerPtr myControllers[BP32_MAX_GAMEPADS];

String parseMACAddr(ControllerProperties properties) {
  uint8_t* arr = properties.btaddr;
  char outstring[24];
  snprintf(outstring, sizeof(outstring), "%x:%x:%x:%x:%x:%x", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
  return String(outstring);
}
String parseMACAddrUint(ControllerProperties properties) {
  uint8_t* arr = properties.btaddr;
  char outstring[24];
  snprintf(outstring, sizeof(outstring), "%u:%u:%u:%u:%u:%u", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
  return String(outstring);
}

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
      // Additionally, you can get certain gamepad properties like:
      // Model, VID, PID, BTAddr, flags, etc.
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                    properties.product_id);
      myControllers[i] = ctl;
      foundEmptySlot = true;
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("CALLBACK: Controller connected, but could not found empty slot");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  bool foundController = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
      myControllers[i] = nullptr;
      foundController = true;
      break;
    }
  }

  if (!foundController) {
    Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
  }
}


// ## MAIN FUNCTION BOARD ##
uint8_t BALANCE_MAC[6] = {0x00, 0x26, 0x59, 0xE1, 0xA3, 0x52};

void dumpBalanceBoard(ControllerPtr ctl) {
  ControllerProperties properties = ctl->getProperties();

  uint16_t top_left = ctl->topLeft();
  uint16_t top_right = ctl->topRight();

  uint16_t bottom_left = ctl->bottomLeft();
  uint16_t bottom_right = ctl->bottomRight();

  // Normalizzazione di valori speciali
  if (top_left - 1000 < 0) top_left = 0;
  else top_left -= 1000;

  if (bottom_right - 1000 < 0) bottom_right = 0;
  else bottom_right -= 1000;

  long long left_sum = (long)(top_left - bottom_left);
  long long right_sum = (long)(top_right - bottom_right);

  uint8_t speed_left = 0;
  uint8_t speed_right = 0;


  int left_direction = 0;
  int right_direction = 0;

  // Calcolo verso sinistra
  if (left_sum - SOGLIA_AVANTI > 0) { // Se positivo e maggiore di soglia
    left_sum = left_sum - SOGLIA_AVANTI;
    speed_left = map(left_sum, 0, V_MAX_CELL, 0, pow(2, N_BIT)-1);
    left_direction = AVANTI;
  } else if (left_sum + SOGLIA_DIETRO < 0) {  // Se negativo e maggiore di soglia
    left_sum = (left_sum * -1) - SOGLIA_DIETRO;
    speed_left = map(left_sum, 0, V_MAX_CELL, 0, pow(2, N_BIT)-1);
    left_direction = DIETRO;
  } else {
    speed_left = 0;
  }

  // Calcolo verso destra
  if (right_sum - SOGLIA_AVANTI > 0) { // Se positivo e maggiore di soglia
    right_sum = right_sum - SOGLIA_AVANTI;
    speed_right = map(right_sum, 0, V_MAX_CELL, 0, pow(2, N_BIT)-1);
    right_direction = AVANTI;
  } else if (right_sum + SOGLIA_DIETRO < 0) {  // Se negativo e maggiore di soglia
    right_sum = (right_sum * -1) - SOGLIA_DIETRO;
    speed_right = map(right_sum, 0, V_MAX_CELL, 0, pow(2, N_BIT)-1);
    right_direction = DIETRO;
  } else {
    speed_right = 0;
  }

  Serial.printf("Left_speed:%u(%d)\tRight_speed:%u(%d)\n", speed_left, left_direction,  speed_right, right_direction);
  handle_movement(left_direction, right_direction);
  
}

void processBalanceBoard(ControllerPtr ctl) {
  // See "dumpBalanceBoard" for possible things to query.
  dumpBalanceBoard(ctl);
}

int confronta_array(uint8_t first[6], uint8_t second[6])
{
  //Serial.printf("First Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", first[0], first[1], first[2], first[3], first[4], first[5]);
  //Serial.printf("Second Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", second[0], second[1], second[2], second[3], second[4], second[5]);
  for (int i = 0; i < 6; i++)
  {
    if (first[i] != second[i]) return 0;
  }
  return 1;
}

void processControllers() {
  for (auto myController : myControllers) {
    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isBalanceBoard() && confronta_array(myController->getProperties().btaddr, BALANCE_MAC)) {
        processBalanceBoard(myController);
      } else {
        Serial.println("Unsupported controller");
      }
    }
  }
}

// Arduino setup function. Runs in CPU 1
void setup() {
  Serial.begin(115200);
  Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But it might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  // Enables mouse / touchpad support for gamepads that support them.
  // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
  // - First one: the gamepad
  // - Second one, which is a "virtual device", is a mouse.
  // By default, it is disabled.
  BP32.enableVirtualDevice(false);

  pinMode(25, OUTPUT);
}

// Arduino loop function. Runs in CPU 1.
void loop() {
  // Controllo stato batterie
  //main_battery();

  // This call fetches all the controllers' data.
  // Call this function in your main loop.
  bool dataUpdated = BP32.update();

  if (dataUpdated)
    processControllers();

  // The main loop must have some kind of "yield to lower priority task" event.
  // Otherwise, the watchdog will get triggered.
  // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
  // Detailed info here:
  // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

  //vTaskDelay(1);
  delay(150);
}