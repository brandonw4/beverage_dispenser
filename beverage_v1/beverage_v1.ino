//Libraries
#include <HX711_ADC.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

//Beverage Class
const int maxMixers = 6; //size of mixer arrays
class Beverage {
    public:
    Beverage(String n, bool a, double oz1, double oz2, double oz3, double oz4, double oz5, double oz6);
    Beverage(String n, bool a, double oz1, double oz2, double oz3);
    double ozArr[maxMixers];
    String name;
    bool active; //used to enable/disable drinks. Future!!~Use onboard memory to store active or inactive and control via keypad admin menu.
};

Beverage::Beverage(String n, bool a, double oz1, double oz2, double oz3) {
  name = n;
  active = a;
  ozArr[0] = oz1;
  ozArr[1] = oz2; 
  ozArr[2] = oz3;
}

Beverage::Beverage(String n, bool a, double oz1, double oz2, double oz3, double oz4, double oz5, double oz6){
  name = n;
  active = a;
  ozArr[0] = oz1;
  ozArr[1] = oz2;
  ozArr[2] = oz3;
  ozArr[3] = oz4;
  ozArr[4] = oz5;
  ozArr[5] = oz6;
}


//LCD Display
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
bool printReadyMsg = true; //used to prevent the loop function from spamming lcd



//Weight scale
HX711_ADC LoadCell(22, 23); 
bool cellDataCall = false;
//due to the very small delay with hose liquid lag and motor shutoff,
//  this factor compensates for that
const double HOSE_LAG_VALUE = 1.8;

//for single shots only:
//  to prevent the single shot glass from overfilling, the value on single shot
//  is reduced to a smaller dispense. This value does not effect beverages.
const double SINGLE_SHOT_VALUE = 1.2;
const double SCALE_OZ_FACTOR = 20.525 - HOSE_LAG_VALUE;

const int MOTOR_TIMEOUT_MILLIS = 15000; //how long motor can run before a motor timeout error generated

//4x4 Keypad
const byte ROWS = 4; 
const byte COLS = 4; 
const char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {53, 52, 51, 50}; 
byte colPins[COLS] = {49, 48, 47, 46}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void(* resetFunc) (void) = 0; //reboot function

int dispense(double oz, int motorNum);
double convertToScaleUnit(double oz);
void decisionTree(char keyVal);
void beverageMenu();
void shotMenu();
void createBeverage(Beverage bev);
void settingsMenu();
void auth();
void runMotor(bool motor, int motorNum); //Future~~ Use onboard memory, or switches, to disable motors if a bottle is empty.
void dispenseShot(int motor, String bottleName);
void cancel();
void updateBottleStatus(int mNum, bool status);

//init drinks
Beverage bev1("testDrink1", true, 1.5, 1.5, 1.5);
Beverage bev2("testDrink2", true, 1.5, 1.5, 1.5);
Beverage bev3("testDrink3", true, 1.5, 1.5, 1.5);
Beverage bev4("testDrink4", false, 1.5, 1.5, 1.5);
Beverage bev5("testDrink5", false, 1.5, 1.5, 1.5);
Beverage bev6("testDrink6", false, 1.5, 1.5, 1.5);
Beverage bev7("testDrink6", false, 1.5, 1.5, 1.5);
Beverage bev8("testDrink6", false, 1.5, 1.5, 1.5);
Beverage bev9("testDrink6", false, 1.5, 1.5, 1.5);


//bottles
const String bottle1Name = "b1Name";
const String bottle2Name = "b2Name";
const String bottle3Name = "b3Name";
const String bottle4Name = "b4Name";
const String bottle5Name = "b5Name";
const String bottle6Name = "b6Name";

//motor pin constants
const int MOTOR_ONE_PIN = 1;
const int MOTOR_TWO_PIN = 2;
const int MOTOR_THREE_PIN = 3;
const int MOTOR_FOUR_PIN = 4;
const int MOTOR_FIVE_PIN = 5;
const int MOTOR_SIX_PIN = 6;

//FUTURE: NEED TO SAVE THIS IN THE ARDUINO MEMORY SO IT SAVES WITH SHUTDOWN
const int MOTOR_EEPROM_ADDRESS[6] = {1, 2, 3, 4, 5, 6}; //the pos in the array corresponds to motor num (arr0-5 --> motor1-6)
//motor/bottle "out of stock status" (bool array, true --> instock, false --> out of stock). Can be manually set in admin menu.
bool bottle_status[6];




void setup() {
  Serial.begin(9600); 

  //LCD control
  lcd.begin(16, 2);
    lcd.print("Untitled v1");
    lcd.setCursor(0,1);
    lcd.print("Brandon Wortman");
    delay(2000);

  //load cell
  float calValue = 696;   //calibration value
  LoadCell.begin(9600);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CLEAR THE SCALE!");
  lcd.setCursor(0,1);
  lcd.print("Calibrating....!");
  delay(5000);
  LoadCell.start(2000); //tare precision, can be more precise by adding more seconds of stabilization time
  LoadCell.setCalFactor(calValue);

  //Motor Control Outputs
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

}

char keypadIn; //for use in the loop function 

void loop() {
  LoadCell.update();
  if (printReadyMsg) {   //purpose explanation with declaration
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Main Menu");
      lcd.setCursor(0,1);
      lcd.print("Make selection.");
  }
  printReadyMsg = false; //prevent spamming of lcd, after it has printed once
  //keypad input
  keypadIn = customKeypad.getKey();
  decisionTree(keypadIn);

  if (cellDataCall) {     //diagnostic: prints scale data to serial console. Controlled by C on, D off.
    Serial.println(LoadCell.getData());
  }


}

int dispense(double oz, int motorNum) {
    
    double beforeDispense = LoadCell.getData();
    Serial.print("Before dispense: ");
    Serial.println(beforeDispense);
    double goalDispense = convertToScaleUnit(oz) + beforeDispense;
    Serial.print("Goal dispense: "); 
    Serial.println(goalDispense);  
    double currentDispense = beforeDispense;

    
    Serial.println(LoadCell.getData());
    int startRunTime = millis();
    while (currentDispense < goalDispense) {
      if (customKeypad.getKey() == '#') {
        cancel();
      }
      LoadCell.update();
      runMotor(true, motorNum);
      currentDispense = LoadCell.getData();
      if ((millis() - startRunTime) > MOTOR_TIMEOUT_MILLIS) { //if the motor is running longer than 15 seconds, timeout error 100
        updateBottleStatus(motorNum, false);
        return 1;
      }
    }
    runMotor(false, motorNum);
    
    delay(1000); //check weight again       if it needs more it will dispense another round
    currentDispense = LoadCell.getData();
    while (currentDispense < goalDispense) {
      if (customKeypad.getKey() == '#') {
        cancel();
      }
      LoadCell.update();
      runMotor(true, motorNum);
      currentDispense = LoadCell.getData();
      if ((millis() - startRunTime) > MOTOR_TIMEOUT_MILLIS) { //if the motor is running longer than 15 seconds, timeout error 100
        updateBottleStatus(motorNum, false);
        return 1;
      }
    }
    return 0;   
    
  }

double convertToScaleUnit(double oz) {
  return oz * SCALE_OZ_FACTOR;
}

void createBeverage(Beverage bev) {
  if (!bev.active) {
    Serial.println("Drink unavailable.");
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Drink unavailable.");
      lcd.setCursor(0,1);
      lcd.print("Return main menu");
      delay(1700);
      printReadyMsg = true;
  }
  
  if (LoadCell.getData() < 4) {
        Serial.println("No cup detected. Please place cup and try again.");
        lcd.clear();
        lcd.println("No cup detected.");
        lcd.setCursor(0,1);
        lcd.println("Return main menu");
        delay(1700);
        printReadyMsg = true;
        return;
      }
  lcd.clear();
    lcd.print(bev.name);
    lcd.setCursor(0,1);
    lcd.print("selected.");
    delay(2000);
  lcd.clear();
    lcd.println("!DONT TOUCH CUP!");
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(4000);
  if (customKeypad.getKey() == '#') {
    cancel();
  }
  lcd.clear();
  if (bev.ozArr[0] > 0) {  //if there is an oz1 value dispense that
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle1Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[0], 1);
  }

   
  if (bev.ozArr[1] > 0) {  //if there is an oz2 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle2Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[1], 2);
  }
   
   
  if (bev.ozArr[2] > 0) {  //if there is an oz3 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle3Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[2], 3);
  }

  if (bev.ozArr[3] > 0) {  //if there is an oz4 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle4Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[3], 3);
  }

  if (bev.ozArr[4] > 0) {  //if there is an oz5 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle5Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[4], 3);
  }

  if (bev.ozArr[5] > 0) {  //if there is an oz6 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle6Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[5], 3);
  }

  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enjoy");
    lcd.setCursor(0, 1);
    lcd.print("responsibly!");
    runMotor(false, 0); //make sure all motors are off
    delay(3000);  
  printReadyMsg = true;
  
}
void decisionTree(char keyVal) {
  if (keyVal == 'A') {
    beverageMenu();
  }
  else if (keyVal == 'B') {
    shotMenu();
  }
  else if (keyVal == 'C') {
    settingsMenu();
  }
  else if (keyVal == '#') {
    cancel();
  }
    
  }

void beverageMenu() {
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Drink Menu");
    lcd.setCursor(0,1);
    lcd.print("Make selection.");
  char bevKeyVal = customKeypad.waitForKey();
  Serial.print("DRINK MENU DEBUG bevKeyVal: ");
  Serial.println(bevKeyVal);
  
  if (bevKeyVal == '1') {
    createBeverage(bev1);
  }
  else if (bevKeyVal == '2') {
    createBeverage(bev2);
  }
  else if (bevKeyVal == '3') {
    createBeverage(bev3);
  }
  else if (bevKeyVal == '4') {
    createBeverage(bev4);
  }
  else if (bevKeyVal == '5') {
    createBeverage(bev5);
  }
  else if (bevKeyVal == '6') {
    createBeverage(bev6);
  }
  else if (bevKeyVal == '7') {
    createBeverage(bev7);
  }
  else if (bevKeyVal == '8') {
    createBeverage(bev8);
  }
  else if (bevKeyVal == '9') {
    createBeverage(bev9);
  }
  else if (bevKeyVal == '#') {
    printReadyMsg = true;
    return;
  }
}

void shotMenu() {
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Shots Menu");
    lcd.setCursor(0,1);
    lcd.print("Make selection.");
  char shotKeyVal = customKeypad.waitForKey();
  Serial.print("SHOT MENU DEBUG shotKeyVal: ");
  Serial.println(shotKeyVal);
  
  if (shotKeyVal == '1') {
    dispenseShot(1, bottle1Name);
  }
  else if (shotKeyVal == '2') {
    dispenseShot(2, bottle2Name);
  }
  else if (shotKeyVal == '3') {
    dispenseShot(3, bottle3Name);
  }
  else if (shotKeyVal == '4') {
    dispenseShot(4, bottle4Name);
  }
  else if (shotKeyVal == '5') {
    dispenseShot(5, bottle5Name);
  }
  else if (shotKeyVal == '6') {
    dispenseShot(6, bottle6Name);
  }
  else if (shotKeyVal == '#'){
    printReadyMsg = true;
    return;
  }
}

void settingsMenu(){
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Menu");
    lcd.setCursor(0,1);
    lcd.print("unavailable.");
    delay(1500);
  printReadyMsg = true;
  return;
  // lcd.clear();
  //   lcd.setCursor(0,0);
  //   lcd.print("ADMIN MENU");
  //   lcd.setCursor(0,1);
  //   lcd.print("Enter/Scan PIN");
}

void auth() {
  char keypadMultiEntry[4];
  bool rfidRead = false;
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Auth. Required");
    lcd.setCursor(0,1);
    lcd.print("Tap Card/Enter PIN");
    //in future maybe add a "* to clear." when autoscroll is figured out if possible
  while(!rfidRead) {
    keypadMultiEntry[0] = customKeypad.getKey();
    //check for rfid read
  }
  //maybe use getKeys for this. Also consider some circumstances where waitForKeys()
  

  
}
void dispenseShot(int motor, String bottleName) {
  if (LoadCell.getData() < 1) {   //number is smaller due to small plastic solo shot cups
        Serial.println("No cup detected. Please place cup and try again.");
        lcd.clear();
        lcd.println("No cup detected.");
        lcd.setCursor(0,1);
        lcd.println("Return main menu");
        delay(1700);
        printReadyMsg = true;
        return;
      }
  lcd.clear();
    lcd.setCursor(0, 0);
    //lcd.print("Dispensing 1.5oz shot of " + bottleName);
    lcd.print("Shot of " + bottleName);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
  delay(1500);
  dispense(SINGLE_SHOT_VALUE, motor);

  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enjoy");
    lcd.setCursor(0, 1);
    lcd.print("responsibly!");
  runMotor(false, 0); 
  delay(3000);  
  printReadyMsg = true;
}
  
void runMotor(bool motorRun, int motorNum) { //implementation of motor controller, motor 0 does not exist but results in a full shutdown of all 3 regardless of bool
    if(motorNum == 1) {
      if (motorRun) {
      digitalWrite(MOTOR_ONE_PIN, HIGH);
    }
      else {
      digitalWrite(MOTOR_ONE_PIN, LOW);
      }
    }

    else if(motorNum == 2) {
      if (motorRun) {
        digitalWrite(MOTOR_TWO_PIN, HIGH);
    }
      else {
        digitalWrite(MOTOR_TWO_PIN, LOW);
      }
    }
    
    else if(motorNum == 3) {
      if (motorRun) {
        digitalWrite(MOTOR_THREE_PIN, HIGH);
    }
      else {
        digitalWrite(MOTOR_THREE_PIN, LOW);
      }
    }

    else if(motorNum == 4) {
      if (motorRun) {
        digitalWrite(MOTOR_FOUR_PIN, HIGH);
    }
      else {
        digitalWrite(MOTOR_FOUR_PIN, LOW);
      }
    }

    else if(motorNum == 5) {
      if (motorRun) {
        digitalWrite(MOTOR_FIVE_PIN, HIGH);
    }
      else {
        digitalWrite(MOTOR_FIVE_PIN, LOW);
      }
    }
    
    else if(motorNum == 6) {
      if (motorRun) {
        digitalWrite(MOTOR_SIX_PIN, HIGH);
    }
      else {
        digitalWrite(MOTOR_SIX_PIN, LOW);
      }
    }

    else if(motorNum == 0) {
      digitalWrite(MOTOR_ONE_PIN, LOW);
      digitalWrite(MOTOR_TWO_PIN, LOW);
      digitalWrite(MOTOR_THREE_PIN, LOW);
      digitalWrite(MOTOR_FOUR_PIN, LOW);
      digitalWrite(MOTOR_FIVE_PIN, LOW);
      digitalWrite(MOTOR_SIX_PIN, LOW);
    } 
  }
  
void cancel() {
  Serial.println("##!CANCELLED!##");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("##!CANCELLED!##");
  lcd.setCursor(0,1);
  lcd.print("Rebooting...");
  runMotor(false, 0);
  delay(3000);
  resetFunc(); 
}

void updateBottleStatus(int mNum, bool status) {
  Serial.print("Updating Motor # ");
  Serial.print(mNum);
  Serial.print(" on address ");
  Serial.print(MOTOR_EEPROM_ADDRESS[mNum]);
  Serial.print(" to ");
  Serial.println(status);
  EEPROM.update(MOTOR_EEPROM_ADDRESS[mNum], status);
}
