#include <LiquidCrystal_I2C.h>
#include <RFID.h>
#include <SPI.h>
#include <HX711_ADC.h>
#include <Keypad.h>
#include <EEPROM.h>

//Beverage Class
const int MOTOR_COUNT = 6;
class Beverage {
    public:
    Beverage(String n, bool a, double oz1, double oz2, double oz3, double oz4, double oz5, double oz6);
    Beverage(String n, bool a, double oz1, double oz2, double oz3);
    Beverage(String n, bool a, double oz1, double oz2, double oz3, double oz4, double oz5, double oz6, String a1, String a2, String a3);
    double ozArr[MOTOR_COUNT];
    String name;
    bool active;
    String additionalInstructions[3]; //up to 3 strings avalible to write to lcd screen if needed to provide additional instructions //changed to 3 for allow price
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

Beverage::Beverage(String n, bool a, double oz1, double oz2, double oz3, double oz4, double oz5, double oz6, String a1, String a2, String a3) {
  name = n;
  active = a;
  ozArr[0] = oz1;
  ozArr[1] = oz2;
  ozArr[2] = oz3;
  ozArr[3] = oz4;
  ozArr[4] = oz5;
  ozArr[5] = oz6;
  additionalInstructions[0] = a1;
  additionalInstructions[1] = a2;
  additionalInstructions[2] = a3;

}

//RFID reader
#define pinRST 5
#define pinSS 2
RFID rfid(pinSS, pinRST);
const int RFID_PASSCODE_STORAGE_SIZE = 4;
const int PASSCODE_LENGTH = 4;
String rfid_card_storage[RFID_PASSCODE_STORAGE_SIZE] = {"18914013814055"};
String passcode_storage[RFID_PASSCODE_STORAGE_SIZE] = {"2002"};


//LCD 2.0
LiquidCrystal_I2C lcd(0x27, 20, 4);
bool printReadyMsg = true; //used to prevent the loop function from spamming lcd
const int DISPENSE_MSG_TIME = 1500;



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

const int CHECK_FOR_CUP_TIME = 500; //the time between checking for a drink on the pad (checked multiple times before starting)
const int CHECK_FOR_CUP_COUNT = 2; //the amount of times a cup will be checked for on the pad before permitting dispensing
const int CHECK_FOR_CUP_MIN_WEIGHT = 4; //the minimum weight (measured in arbitary value by on the loadcell) to "detect" a cup

//4x4 Keypad
const byte ROWS = 4; 
const byte COLS = 4; 
const char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {45, 44, 43, 42}; 
byte colPins[COLS] = {49, 48, 47, 46}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void(* resetFunc) (void) = 0; //reboot function

int dispense(double oz, int motorNum);
double convertToScaleUnit(double oz);
int createBeverage(Beverage bev);
void decisionTree(char keyVal);
void beverageMenu();
void shotMenu();
void settingsMenu();
int auth();
void dispenseShot(int motor, String bottleName);
void runMotor(bool motor, int motorNum); 
void cancel();
void updateBottleStatus(int mNum, bool status);
int checkForCup();
int userCheck();


//init drinks
Beverage bev1("testDrink1", true, 1.5, 1.5, 1.5, 0, 0 ,0, "Almost done!", "Garnish with orange", "Add splash of coke");
Beverage bev2("testDrink2", true, 1.5, 1.5, 1.5);
Beverage bev3("testDrink3", true, 1.5, 1.5, 1.5);
Beverage bev4("testDrink4", true, 1.5, 1.5, 1.5);
Beverage bev5("testDrink5", false, 1.5, 1.5, 1.5);
Beverage bev6("testDrink6", false, 1.5, 1.5, 1.5);
Beverage bev7("testDrink6", false, 1.5, 1.5, 1.5);
Beverage bev8("testDrink6", false, 1.5, 1.5, 1.5);
Beverage bev9("testDrink6", false, 1.5, 1.5, 1.5);


//bottles
const String BOTTLE_NAMES[MOTOR_COUNT] = {"b1Name", "b2Name", "b3Name", "b4Name", "b5Name", "b6Name"};
const double BOTTLE_COSTS[MOTOR_COUNT] = {1.6, 1.7, 1.8, 1.9, 2.2, 3.2}; //measured in price per ounce. Indexed at motor index


//motor pin constants
const int MOTOR_ONE_PIN = 7;
const int MOTOR_TWO_PIN = 8;
const int MOTOR_THREE_PIN = 9;
const int MOTOR_FOUR_PIN = 10;
const int MOTOR_FIVE_PIN = 11;
const int MOTOR_SIX_PIN = 12;

//EEPROM
const int MOTOR_EEPROM_ADDRESS[MOTOR_COUNT] = {1, 2, 3, 4, 5, 6}; //the pos in the array corresponds to motor num (arr0-5 --> motor1-6)
//const int AUTH_PASS_EEPROM_ADDRESS[RFID_PASSCODE_STORAGE_SIZE] = {8, 9, 10, 11};
const int SETTINGS_AUTH_DRINK_ADDRESS = 12;
const int SETTINGS_AUTH_SHOTS_ADDRESS = 13;
//const int AUTH_RFID_EEPROM_ADDRESS[RFID_PASSCODE_STORAGE_SIZE] = {14, 15, 16, 17};
//motor/bottle "out of stock status" (bool array, true --> instock, false --> out of stock). Can be manually set in admin menu.
bool bottle_status[MOTOR_COUNT];
bool auth_drink = false;
bool auth_shots = false;




void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.init();


  //LCD control
  //lcd.begin(16, 2);
  lcd.init();
  lcd.init();
  lcd.backlight();
  //lcd.begin(20, 4);
    lcd.setCursor(0,0);
    lcd.print("Untitled");
    lcd.setCursor(0,1);
    lcd.print("Brandon Wortman");
    lcd.setCursor(0,3);
    lcd.print("Booting up.........!");
    delay(2000);

  Serial.println("Running EEPROM read on all motors to check for out of stock motors.");
  int eepromResult = -1;
  for(int i = 0; i < MOTOR_COUNT; i++) {
    eepromResult = EEPROM.read(MOTOR_EEPROM_ADDRESS[i]);
    Serial.print("Motor ");
    Serial.print(i + 1);
    Serial.print(" eeprom read result: ");
    Serial.println(eepromResult);
    if (bottle_status[i] != eepromResult) {
      bottle_status[i] = eepromResult;

    }
  }
  Serial.println("EEPROM Motor Status Read Complete.");

  Serial.println("Reading EEPROM for auth shots/drink settings.");
  eepromResult = -1;
  eepromResult = EEPROM.read(SETTINGS_AUTH_DRINK_ADDRESS);
  Serial.print("Drink EEPROM Auth Setting: ");
  Serial.println(eepromResult);
  if (auth_drink != eepromResult) {
    auth_drink = eepromResult;
  }
  eepromResult = -1;
  eepromResult = EEPROM.read(SETTINGS_AUTH_SHOTS_ADDRESS);
  Serial.print("Shots EEPROM Auth Setting: ");
  Serial.println(eepromResult);
  if (auth_shots != eepromResult) {
    auth_shots = eepromResult;
  }
  Serial.println("EEPROM AUTH Settings Read Complete.");


  //load cell
  float calValue = 696;   //calibration value
  LoadCell.begin(9600);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CLEAR THE SCALE....!");
  lcd.setCursor(0,2);
  lcd.print("Calibrating........!");
  delay(5000);
  LoadCell.start(2000); //tare precision, can be more precise by adding more seconds of stabilization time
  LoadCell.setCalFactor(calValue);

  //Motor Control Outputs
  pinMode(MOTOR_ONE_PIN, OUTPUT);
  pinMode(MOTOR_TWO_PIN, OUTPUT);
  pinMode(MOTOR_THREE_PIN, OUTPUT);
  pinMode(MOTOR_FOUR_PIN, OUTPUT);
  pinMode(MOTOR_FIVE_PIN, OUTPUT);
  pinMode(MOTOR_SIX_PIN, OUTPUT); 

}

char keypadIn; //for use in the loop function 

void loop() {
  LoadCell.update();
  if (printReadyMsg) {   //purpose explanation with declaration
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Main Menu:");
      lcd.setCursor(0,1);
      lcd.print("Make a selection.");
      lcd.setCursor(0,2);
      lcd.print("A) Drink Menu");
      lcd.setCursor(0,3);
      lcd.print("B) Shots Menu");
    printReadyMsg = false; //prevent spamming of lcd, after it has printed once
  }
  //keypad input
  keypadIn = customKeypad.getKey();
  decisionTree(keypadIn);

  if (cellDataCall) {     //diagnostic: controlled in settings menu
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

    
    long startRunTime = millis();
    while (currentDispense < goalDispense) {
      if (customKeypad.getKey() == '#') {
        cancel();
      }
      LoadCell.update();
      runMotor(true, motorNum);
      currentDispense = LoadCell.getData();
      if ((millis() - startRunTime) > MOTOR_TIMEOUT_MILLIS) { //if the motor is running longer than 15 seconds, timeout error 100
        Serial.print("Debugging Error 100. Current time millis(): ");
        Serial.print(millis());
        Serial.print(" and startRunTime: ");
        Serial.print(startRunTime);
        Serial.print(". millis() - startRunTime = ");
        Serial.println(millis() - startRunTime);
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

int createBeverage(Beverage bev) {
   double bevTotalPrice = 0.0;

   if (!bev.active) {
    Serial.println("Drink disabled.");
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Drink disabled.");
      lcd.setCursor(0,1);
      lcd.print("Return main menu");
      delay(2000);
      printReadyMsg = true;
  }
  
  for (int i = 1; i <= MOTOR_COUNT; i++){
    if(bottle_status[i - 1] == 0 && bev.ozArr[i - 1] > 0) {
      Serial.print("ERROR 101. Bottle# ");
      Serial.println(i);
      lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("ERROR 101");
        lcd.setCursor(0,1);
        lcd.print("Bottle# ");
        lcd.print(i);
        delay(2000);
      printReadyMsg = true;
      return 1;
    }
  }
  LoadCell.refreshDataSet(); //BETA Does this effect calibration??

  LoadCell.update();
  if (LoadCell.getData() < 4) {
        Serial.println("No cup detected. Please place cup and try again.");
        lcd.clear();
        Serial.print("createBeverage No Cup Detected Error, Weight Value: ");
        lcd.println("No cup detected.");
        lcd.setCursor(0,1);
        lcd.println("Return main menu");
        Serial.print("createBeverage No Cup Detected Error, Weight Value: ");
        Serial.println(LoadCell.getData());
        delay(1700);
        printReadyMsg = true;
        return;
      }
      
    //  if (checkForCup() != 0) {
    //    Serial.println("Error 102, createBeverage from checkForCup.");
    //    return 1;
    //  }
  
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
    return 1;
  }
  lcd.clear();
  
  for(int i = 0; i < MOTOR_COUNT; i++) { //loop through all beverage oz settings, see if there is a value to dispense
    if(bev.ozArr[i] > 0) {
    lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dispensing " + BOTTLE_NAMES[i]);
      lcd.setCursor(0,1);
      lcd.println("# key to cancel.");
    delay(DISPENSE_MSG_TIME);
    if (dispense(bev.ozArr[0], i + 1) == 1) {
      lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR 100");
        lcd.setCursor(0, 1);
        lcd.print("CANCELLING");
      delay(3000);
      printReadyMsg = true;
      return 1;
    }
      bevTotalPrice += bev.ozArr[i] * BOTTLE_COSTS[i];
      Serial.print("Loop Drink Index: ");
      Serial.print(i);
      Serial.print(" Price running total: ");
      Serial.println(bevTotalPrice);
    }
  }
  lcd.clear();
  for (int i = 0; i < 3; i++) {
    if (bev.additionalInstructions[i] != "") {
      lcd.setCursor(0,i);
      lcd.print(bev.additionalInstructions[i]);
    }
    
    if (i == 2) {
      lcd.setCursor(0,2);
      lcd.print("This drink cost ");
      lcd.print(bevTotalPrice);
      lcd.print(" to make.");
      delay(5500);
      printReadyMsg = true;
      return 0;
    }
  }

  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enjoy");
    lcd.setCursor(0, 1);
    lcd.print("responsibly!");
    runMotor(false, 0); //make sure all motors are off
    delay(3000);  
  printReadyMsg = true;
  return 0;
  
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
  if (auth_drink) {
    if(auth() != 0){
      lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Authorization Failed");
        lcd.setCursor(0,1);
        lcd.print("Returning to");
        lcd.setCursor(0,2);
        lcd.print("main menu.");
        delay(DISPENSE_MSG_TIME);
        return;
    }
  }
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Drink Menu..........");
    lcd.setCursor(0,1);
    lcd.print("Make selection......");
    lcd.setCursor(0,2);
    lcd.print("Check paper menu for");
    lcd.setCursor(0,3);
    lcd.print("available drinks.");
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
  if (auth_shots) {
    if(auth() != 0){
      lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("AUTH FAILED");
        delay(DISPENSE_MSG_TIME);
        return;
    }
  }
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Shots Menu");
    lcd.setCursor(0,1);
    lcd.print("Make selection.");
  char shotKeyVal = customKeypad.waitForKey();
  Serial.print("SHOT MENU DEBUG shotKeyVal: ");
  Serial.println(shotKeyVal);
  
  if (shotKeyVal == '1') {
    dispenseShot(1, BOTTLE_NAMES[0]);
  }
  else if (shotKeyVal == '2') {
    dispenseShot(2, BOTTLE_NAMES[1]);
  }
  else if (shotKeyVal == '3') {
    dispenseShot(3, BOTTLE_NAMES[2]);
  }
  else if (shotKeyVal == '4') {
    dispenseShot(4, BOTTLE_NAMES[3]);
  }
  else if (shotKeyVal == '5') {
    dispenseShot(5, BOTTLE_NAMES[4]);
  }
  else if (shotKeyVal == '6') {
    dispenseShot(6, BOTTLE_NAMES[5]);
  }
  else if (shotKeyVal == '#'){
    printReadyMsg = true;
    return;
  }
}

void settingsMenu(){
  if(auth() != 0) {
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Authorization Failed");
      lcd.setCursor(0,1);
      lcd.print("Returning to");
      lcd.setCursor(0,2);
      lcd.print("main menu.");
      delay(DISPENSE_MSG_TIME);
    return;
  }
  
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ADMIN MENU");
    lcd.setCursor(0,1);
    lcd.print("Selection");
  int settingsKeyIn = customKeypad.waitForKey() - '0';
  Serial.print("Settings Key In Int Selection: ");
  Serial.println(settingsKeyIn);
  char settingsKeyInChar;
  if (settingsKeyIn == 1) {
    while(true) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Bottle Status");
      lcd.setCursor(0,1);
      lcd.print("Manual Override.");
    settingsKeyInChar = customKeypad.waitForKey();
    settingsKeyIn = settingsKeyInChar - '0';
    if(settingsKeyInChar == '#') {
      break;
    }
    for (int i = 1; i <= MOTOR_COUNT; i++) {
      if (i == settingsKeyIn) {
        lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Motor ");
          lcd.print(settingsKeyIn);
          lcd.print(" selected.");
          lcd.setCursor(0,1);
          lcd.print("*Enable,#Disable");
        settingsKeyInChar = customKeypad.waitForKey();
        if (settingsKeyInChar == '*') {
          updateBottleStatus(i, true);
          break;
        }
        else if (settingsKeyInChar == '#') {
          updateBottleStatus(i, false);
          break;
        }

      }
    }
    }
    
  }
  else if (settingsKeyIn == 2) {
    while(true){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("AUTH Required For Drink?");
      lcd.setCursor(0,1);
      lcd.print("1(yes), 2(no)");
      settingsKeyInChar = customKeypad.waitForKey();
      if (settingsKeyInChar == '1') {
        Serial.println("Enabling Auth on Drink");
        auth_drink = true;
        EEPROM.update(SETTINGS_AUTH_DRINK_ADDRESS, auth_drink);
        break;
      }
      else if (settingsKeyInChar == '2') {
        Serial.println("Disabling Auth on Drink");
        auth_drink = false;
        EEPROM.update(SETTINGS_AUTH_DRINK_ADDRESS, auth_drink);
        break;
      }
    }
    
  }
  else if (settingsKeyIn == 3) {
    while(true){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("AUTH Required For Shots?");
      lcd.setCursor(0,1);
      lcd.print("1(yes), 2(no)");
      settingsKeyInChar = customKeypad.waitForKey();
      if (settingsKeyInChar == '1') {
        Serial.println("Enabling Auth on Shots");
        auth_shots = true;
        EEPROM.update(SETTINGS_AUTH_SHOTS_ADDRESS, auth_shots);
        break;
      }
      else if (settingsKeyInChar == '2') {
        Serial.println("Disabling Auth on Shots");
        auth_shots = false;
        EEPROM.update(SETTINGS_AUTH_SHOTS_ADDRESS, auth_shots);
        break;
      }
    }
    
  }
  printReadyMsg = true;
  return;

}

int auth() {
  char keypadMultiEntry[4];
  bool authenticate = true;
  String rfidCard = "";
  String passcode = "";
  char keyEntry;
  bool printPrompt = true;
    //in future maybe add a "* to clear." when autoscroll is figured out if possible
  while (authenticate) {
    if (printPrompt) {
      lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Authorization");
        lcd.setCursor(0,1);
        lcd.print("required.");
        lcd.setCursor(0,2);
        lcd.print("Tap card or * for");
        lcd.setCursor(0,3);
        lcd.print("passcode.");
      printPrompt = false;
    }
    if (rfid.isCard()) {
      /* If so then get its serial number */
      rfid.readCardSerial();
      Serial.println("Card detected:");
      rfidCard = "";
      for(int i=0;i<5;i++)
      {
      rfidCard += String(rfid.serNum[i],DEC);
      //Serial.print(rfid.serNum[i],HEX); //to print card detail in Hexa Decimal format
      }
      Serial.print("RFID Card Read Variable: ");
      Serial.println(rfidCard);
      
      for (int i = 0; i < RFID_PASSCODE_STORAGE_SIZE; i++) {
        if (rfid_card_storage[i] == rfidCard) {
          Serial.print("RFID Authenticated: ");
          Serial.println(rfidCard);
          return 0;
        }
      }
    }

    if (customKeypad.getKey() == '*') {
        lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Auth. Passcode");
          lcd.setCursor(0,1);
          lcd.print("Enter Pin.");
          lcd.setCursor(0,2);
          lcd.print("# to return.");
          //FINISH On larger display add a 1-4 indicator to show entry of passcode. Also clairify prompt.
        lcd.setCursor(0,3);
        lcd.print("----");
        lcd.setCursor(0,3);
        for (int i = 0; i < PASSCODE_LENGTH; i++) {
          keyEntry = customKeypad.waitForKey();
          if (keyEntry == '#') {
            printPrompt = true;
            return 1;
          }
          else {
            passcode += String(keyEntry);
            lcd.print("*");
          }

        }
        
        for (int i = 0; i < RFID_PASSCODE_STORAGE_SIZE; i++) {
          if ((passcode == passcode_storage[i]) && (passcode_storage[i] != "")) {
            Serial.print("Passcode Authenticated: ");
            Serial.println(passcode);
            return 0;
          }
        }

        return 0;
    }

    if (customKeypad.getKey() == '#') {
      return 1;
    }
  }
  

  
}

void dispenseShot(int motor, String bottleName) {
  if (LoadCell.getData() < 1) {   //number is smaller due to small plastic solo shot cups
        Serial.println("No cup detected. Please place cup and try again.");
        lcd.clear();
        lcd.println("No cup detected.");
        lcd.setCursor(0,1);
        lcd.println("Return main menu");
        Serial.print("dispenseShot No Cup Detected Error, Weight Value: ");
        Serial.println(LoadCell.getData());
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
  delay(DISPENSE_MSG_TIME);
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
  Serial.print(MOTOR_EEPROM_ADDRESS[mNum - 1]);
  Serial.print(" to ");
  Serial.println(status);
  bottle_status[mNum - 1] = status;
  EEPROM.update(MOTOR_EEPROM_ADDRESS[mNum - 1], status);
}

int checkForCup() { //finish this implementation and determine if continue use
  int weightCount = 0;
  for (int i = 0; i < CHECK_FOR_CUP_COUNT; i++) {
    weightCount = 0;
    for (int j = 0; j < CHECK_FOR_CUP_COUNT; j++) {
      if (LoadCell.getData() > CHECK_FOR_CUP_MIN_WEIGHT) {
        weightCount++;
        Serial.println(LoadCell.getData());
        delay(500);
      }
    }
    if (weightCount != CHECK_FOR_CUP_COUNT) {
      Serial.println("Error 102, 1 more check");
      delay(1000);
      //FINISH need to create lcd print
    }
    else {
      return 0;
    }
    
  }
  Serial.println("Error 102, timeout return.");
  return 1;
}

int userCheck() {
  char userKey;
  char user4DigitArr[4];
  char userPhone[10];
  lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Enter Last 4 Digits");
    lcd.setCursor(0,1);
    lcd.print("of Phone #.");
    lcd.setCursor(0,2);
    lcd.print("* Undo. # Cancel.");
  
  lcd.setCursor(0,3);
  lcd.print("----");
  lcd.setCursor(0,3);
  for (int i = 0; i < 4; i++) {
    userKey = customKeypad.waitForKey();
    if (userKey == '#') {
      return 1;
    }
    else if (userKey == '*' && i != 0) {
      user4DigitArr[i - 1] = (char)0;
      lcd.setCursor(i - 1,3);
      lcd.print("-");
      lcd.setCursor(0,3);
    }
    else {
      user4DigitArr[i] = userKey;
      lcd.print(userKey);
    }
  }

  //TEMP IF STATEMENT FINISH: If there is no phone number with those 4 digits in the text file, register the user
  if (true) {
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Phone Digits New");
      lcd.setCursor(0,1);
      lcd.print("Please enter full phone #");
      lcd.setCursor(0,2);
      lcd.print("(Only required once)");
      lcd.setCursor(0,3);
      lcd.print("----------");

    lcd.setCursor(0,3);
    for (int i = 0; i < 10; i++) {
      userKey = customKeypad.waitForKey();
      if (userKey == '#') {
        return 1;
      }
      else if (userKey == '*' && i != 0) {
        userPhone[i - 1] = (char)0;
        lcd.setCursor(i - 1,3);
        lcd.print("-");
        lcd.setCursor(0,3);
      }
      else {
        userPhone[i] = userKey;
        lcd.print(userKey);
      }
    }
    //write phone and time to sd card
    lcd.clear();
      lcd.setCursor(0,1);
      lcd.print("Registered.");
      lcd.setCursor(0,2);
      lcd.print("Thank you!");
      delay(1500);
  }
  
  else { //use the found phone number and write that phone number and time to sd card
    //write phone and time to sd card
  }

  return 0;
}

