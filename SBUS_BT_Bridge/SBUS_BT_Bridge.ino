#include <Bluepad32.h>
#include "sbus.h"

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

/* SBUS object, reading SBUS */
bfs::SbusRx sbus_rx(&Serial0, D7, D6, true);
/* SBUS object, writing SBUS */
bfs::SbusTx sbus_tx(&Serial0, D7, D6, true);
/* SBUS data */
bfs::SbusData data;

unsigned long StartTime = 0;
unsigned long CurrentTime = 0;
unsigned long ElapsedTime = 0;

struct SBUS_DATA{
  int32_t Lx;
  int32_t Ly;
  int32_t Rx;
  int32_t Ry;
  int32_t CH5;
  int32_t CH6;
  int32_t CH7;
  int32_t CH8;
};


// Button State Flags:
bool A_Flag = 0;
bool B_Flag = 0;
bool X_Flag = 0;
bool Y_Flag = 0;
bool L1_Flag = 0;
bool L2_Flag = 0;
bool R1_Flag = 0;
bool R2_Flag = 0;
bool D_UP_Flag = 0;
bool D_DOWN_Flag = 0;
bool D_LEFT_Flag = 0;
bool D_RIGHT_Flag = 0;
bool START_Flag = 0;
bool SELECT_Flag = 0;
bool XBOX_Flag = 0;
bool SHARE_Flag = 0;


// DOG State Flags:
bool HIGH_STAND_Flag = 1;
bool OBSTACLE_Flag = 0;
int8_t WALK_FLAG = 0;


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

void dumpGamepad(ControllerPtr ctl) {


    Serial.printf(
        "idx:%d,dpad:%2d,buttons:%3d,Lx:%4d,Ly:%4d,Rx:%4d,Ry:%4d,brake:%4d,throttle:%4d,misc:%2d\n",
        ctl->index(),        // Controller Index
        ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->brake(),        // (0 - 1023): brake button
        ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons()  // bitmask of pressed "misc" buttons
    );
}

void writeSBUS(SBUS_DATA my_data){
  
  data.ch[0] = my_data.Rx;
  data.ch[1] = my_data.Ry;
  data.ch[2] = my_data.Ly;
  data.ch[3] = my_data.Lx;
  data.ch[4] = my_data.CH5; // CH5 default 992
  data.ch[5] = my_data.CH6; // CH6
  data.ch[6] = my_data.CH7; // CH7
  data.ch[7] = my_data.CH8; // CH8
  data.ch[8] = 1690;
  data.ch[9] = 154;

  sbus_tx.data(data);
  sbus_tx.Write();

}

void processGamepad(ControllerPtr ctl) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...

    // JoyStick States:
    int32_t Lx = map(ctl->axisX(), -512, 511, 432, 1552); 
    int32_t Ly = map(ctl->axisY(), 511, -512, 192, 1792);
    int32_t Rx = map(ctl->axisRX(), -512, 511, 432, 1552);  
    int32_t Ry = map(ctl->axisRY(), -512, 511, 432, 1552);

    // DPAD Button States:
    bool DPAD_UP, DPAD_DOWN, DPAD_RIGHT, DPAD_LEFT;
    int8_t dpadState = ctl->dpad();
    DPAD_UP = (dpadState & 0b0001) != 0;
    DPAD_DOWN = (dpadState & 0b0010) != 0;
    DPAD_RIGHT = (dpadState & 0b0100) != 0;
    DPAD_LEFT = (dpadState & 0b1000) != 0;

    bool XBOX, START, SELECT, SHARE;
    int8_t miscState = ctl->miscButtons();
    XBOX = ctl->miscSystem();
    START = ctl->miscStart();
    SELECT = ctl->miscSelect();
    SHARE = ctl->miscCapture();

    bool A, B, X, Y, L1, L2, R1, R2;
    int32_t buttonsState = ctl->buttons();
    A = ctl->a();
    B = ctl->b();
    X = ctl->x();
    Y = ctl->y();
    L1 = ctl->l1();
    L2 = ctl->l2();
    R1 = ctl->r1();
    R2 = ctl->r2();
    

    // MODE FLAGS:
    int32_t MODE1 = 992;
    int32_t MODE2 = 192;
    int32_t MODE3 = 1792;

    int32_t _LOW = 192;
    int32_t _MID = 992;
    int32_t _HIGH = 1792;

    int32_t CH5_STATE = MODE1;
    int32_t CH6_STATE = _LOW;
    int32_t CH7_STATE = _LOW;
    int32_t CH8_STATE = _MID;

    // Store Values Here to write to SBUS TX
    SBUS_DATA tx_data;

    tx_data.Lx = Lx;
    tx_data.Ly = Ly;
    tx_data.Rx = Rx;
    tx_data.Ry = Ry;
    tx_data.CH5 = CH5_STATE;
    tx_data.CH6 = CH6_STATE;
    tx_data.CH7 = CH7_STATE;
    tx_data.CH8 = CH8_STATE;
    
    // START BUTTON - Unlock
    if(START == true){
      if(START_Flag == 0){
        CH5_STATE = MODE1;
        CH7_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE1;
        CH7_STATE = _HIGH;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE1;
        CH7_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        START_Flag = 1;

        return;      
      }
      else{}
    }
    else if(START_Flag = 1 && START == 0){
      START_Flag = 0;
    }

    // SELECT BUTTON - Damping State
    if(SELECT == 1){
      if(SELECT_Flag == 0){
        CH5_STATE = MODE1;
        CH6_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE1;
        CH6_STATE = _HIGH;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE1;
        CH6_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        SELECT_Flag = 1;

        return;      
      }
      else{}
    }
    else if(SELECT_Flag = 1 && SELECT == 0){
      SELECT_Flag = 0;
    }

    // DPAD BUTTONS
    // DPAD UP BUTTON - Fall Recover
    if(DPAD_UP == 1){

      if(D_UP_Flag == 0){
        CH5_STATE = MODE2;
        CH7_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE2;
        CH7_STATE = _HIGH;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE2;
        CH7_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        D_UP_Flag = 1;

        return;
        }  
    }
    else if(DPAD_UP == 0 && D_UP_Flag == 1){
      D_UP_Flag = 0;
    }
    else{}

    // DPAD DOWN BUTTON - STAND Toggle
    if (DPAD_DOWN == true) {
      if(D_DOWN_Flag == 0){

        if(HIGH_STAND_Flag == 0){

          CH5_STATE = MODE1;
          CH8_STATE = _MID;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE1;
          CH8_STATE = _HIGH;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE1;
          CH8_STATE = _LOW;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          D_DOWN_Flag = 1;
          HIGH_STAND_Flag = 1;

          return;

        }
        else if (HIGH_STAND_Flag == 1) {

          CH5_STATE = MODE1;
          CH8_STATE = _MID;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE1;
          CH8_STATE = _LOW;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE1;
          CH8_STATE = _MID;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          D_DOWN_Flag = 1;
          HIGH_STAND_Flag = 0;

          return;

        }
              
      } 
    }
    else if(DPAD_DOWN == 0 && D_DOWN_Flag == 1){
      D_DOWN_Flag = 0;
    }
    else{}

    // DPAD RIGHT BUTTON - Light Toggle
    if (DPAD_RIGHT == true) {
      if(D_RIGHT_Flag == 0){

        CH5_STATE = MODE3;
        CH8_STATE = _MID;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH8 = CH8_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH8_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH8 = CH8_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH8_STATE = _MID;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH8 = CH8_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        D_RIGHT_Flag= 1;

        return;
      } 
    }
    else if(DPAD_RIGHT == 0 && D_RIGHT_Flag == 1){
      D_RIGHT_Flag = 0;
    }
    else{}

    // DPAD LEFT BUTTON - Continous Movement
    if (DPAD_LEFT == 1) {
      if(D_LEFT_Flag == 0){

        CH5_STATE = MODE2;
        CH6_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE2;
        CH6_STATE = _HIGH;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE2;
        CH6_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        D_LEFT_Flag= 1;

        return;
      } 
    }
    else if(DPAD_LEFT == 0 && D_LEFT_Flag == 1){
      D_LEFT_Flag = 0;
    }
    else{}

    // SHOULDER Buttons:
    // R1 Button - Walking Gait Toggle 
    if (R1 == true) {
      if(R1_Flag == 0){

        CH5_STATE = MODE3;
        CH8_STATE = _MID;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH8 = CH8_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH8_STATE = _HIGH;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH8 = CH8_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH8_STATE = _MID;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH8 = CH8_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        R1_Flag= 1;

        return;
      } 
    }
    else if(R1 == 0 && R1_Flag == 1){
      R1_Flag = 0;
    }
    else{}

  
    // FACE Buttons:
    // X BUTTON - Obstacle Avoidance Toggle
    if (X == 1) {
      if(X_Flag == 0){

        if(OBSTACLE_Flag == 0){

          CH5_STATE = MODE2;
          CH8_STATE = _MID;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE2;
          CH8_STATE = _LOW;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE2;
          CH8_STATE = _MID;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          X_Flag = 1;
          OBSTACLE_Flag = 1;

          return;
        }
        else if (OBSTACLE_Flag == 1) {

          CH5_STATE = MODE2;
          CH8_STATE = _MID;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE2;
          CH8_STATE = _HIGH;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          CH5_STATE = MODE2;
          CH8_STATE = _MID;
          tx_data.CH5 = CH5_STATE;
          tx_data.CH8 = CH8_STATE;

          writeSBUS(tx_data);
          delayMicroseconds(600);

          X_Flag = 1;
          OBSTACLE_Flag = 0;

          return;
        }
      } 
    }
    else if(X_Flag == 1 && X == 0){
      X_Flag = 0;
    }
    else{}

    // Y BUTTON - Dance
    if(Y == 1){
      if(Y_Flag == 0){
        CH5_STATE = MODE3;
        CH7_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH7_STATE = _HIGH;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH7_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH7 = CH7_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        Y_Flag = 1;

        return;      
      }
      else{}
    }
    else if(Y_Flag = 1 && Y == 0){
      Y_Flag = 0;
    }

    // A BUTTON - Jump
    if (A == 1) {
      if(A_Flag == 0){

        CH5_STATE = MODE3;
        CH6_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH6_STATE = _HIGH;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        CH5_STATE = MODE3;
        CH6_STATE = _LOW;
        tx_data.CH5 = CH5_STATE;
        tx_data.CH6 = CH6_STATE;

        writeSBUS(tx_data);
        delayMicroseconds(600);

        A_Flag= 1;

        return;
      } 
    }
    else if(A == 0 && A_Flag == 1){
      A_Flag = 0;
    }
    else{}

    data.ch[0] = Rx;
    data.ch[1] = Ry;
    data.ch[2] = Ly;
    data.ch[3] = Lx;
    data.ch[4] = CH5_STATE; //CH5 default 992
    data.ch[5] = CH6_STATE; //CH6
    data.ch[6] = CH7_STATE; //CH7
    data.ch[7] = CH8_STATE;
    data.ch[8] = 1690;
    data.ch[9] = 154;

    sbus_tx.data(data);
    sbus_tx.Write();


    Serial.printf(
        "idx:%d,dpad:%2d,buttons:%3d,Lx:%4d,Ly:%4d,Rx:%4d,Ry:%4d,brake:%4d,throttle:%4d,misc:%2d\n\r",
        ctl->index(),        // Controller Index
        ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        Lx,        // (-511 - 512) left X Axis
        Ly,        // (-511 - 512) left Y axis
        Rx,       // (-511 - 512) right X axis
        Ry,       // (-511 - 512) right Y axis
        ctl->brake(),        // (0 - 1023): brake button
        ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons()  // bitmask of pressed "misc" buttons

    );

    //dumpGamepad(ctl);
}

void processControllers() {
    for (auto myController : myControllers) {
        //if (myController && myController->isConnected() && myController->hasData()) {
        if (myController && myController->isConnected()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } 
            else {
                Serial.println("Unsupported controller");
            }
        }
    }
}

void readRC(){
  unsigned long StartTime = 0;
  unsigned long CurrentTime = 0;
  unsigned long ElapsedTime = 0;

  StartTime = micros();
  if (sbus_rx.Read()) {
    CurrentTime = micros();
    ElapsedTime = CurrentTime - StartTime;

    /* Grab the received data */
    data = sbus_rx.data();
    /* Display the received data */ 
    for (int8_t i = 0; i < data.NUM_CH; i++) {
      Serial.print(data.ch[i]);
      Serial.print("\t");
    }
    /* Display lost frames and failsafe data */
    Serial.print(data.lost_frame);
    Serial.print("\t");
    Serial.print(data.failsafe);
    Serial.print("\t");
    Serial.println(ElapsedTime);
    /* Set the SBUS TX data to the received data */
    sbus_tx.data(data);
    /* Write the data to the servos */
    sbus_tx.Write();
  }
}

// Arduino setup function. Runs in CPU 1
void setup() {
    Serial.begin(115200);
    //while (!Serial) {}  // Waits for serial terminal to be connected - DEBUGGING ONLY
    
    /* Begin the SBUS communication */
    sbus_rx.Begin();
    sbus_tx.Begin();

    //Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    //Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

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
}

// Arduino loop function. Runs in CPU 1.
void loop() {
    // This call fetches all the controllers' data.
    // Call this function in your main loop.
    bool dataUpdated = BP32.update();
    //if (dataUpdated)

    processControllers();
    //readRC(); // Reads SBUS data from RC Controller and prints all channels to terminal 

    // The main loop must have some kind of "yield to lower priority task" event.
    // Otherwise, the watchdog will get triggered.
    // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
    // Detailed info here:
    // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

    vTaskDelay(1);
    delayMicroseconds(600);
}
