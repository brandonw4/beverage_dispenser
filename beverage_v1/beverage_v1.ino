//Libraries
#include <HX711_ADC.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

//LCD Display
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
bool printReadyMsg = true; //used to prevent the loop function from spamming lcd



//Weight scale
HX711_ADC LoadCell(22, 23); 
long t;
bool cellDataCall = false;
const double SCALE_OZ_FACTOR = 20.525;

//4x4 Keypad
const byte ROWS = 4; 
const byte COLS = 4; 
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {53, 52, 51, 50}; 
byte colPins[COLS] = {49, 48, 47, 46}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void(* resetFunc) (void) = 0; //reboot function

void dispense(double oz, int motorNum);
double convertToScaleUnit(double oz);
void decisionTree(char keyVal);
void createBeverage(double oz1, double oz2, double oz3, String bevName);
void runMotor(bool motor, int motorNum);

//init drinks
String drink1Name = "TEST DRINK 1";
double d1oz1 = 1.5;
double d1oz2 = 1.5;
double d1oz3 = 1.5;
String drink2Name = "dr2";
double d2oz1 = 1.5;
double d2oz2 = 1;
double d2oz3 = 1;
String drink3Name = "dr3";
double d3oz1 = 1.5;
double d3oz2 = 1;
double d3oz3 = 1;
String drink4Name = "dr4";
double d4oz1 = 1.5;
double d4oz2 = 1;
double d4oz3 = 1;
String drink5Name = "dr5";
double d5oz1 = 1.5;
double d5oz2 = 1;
double d5oz3 = 1;
String drink6Name = "dr5";
double d6oz1 = 1.5;
double d6oz2 = 1;
double d6oz3 = 1;

//bottles
String bottle1Name = "Vodka";
String bottle2Name = "Rum";
String bottle3Name = "Fireball";




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


//LED control
  //temp leds for motor
  pinMode(2, OUTPUT); //red 2
  pinMode(3, OUTPUT); //blue 3
  pinMode(4, OUTPUT); //yellow 4
  


}

void loop() {
  LoadCell.update();
  if (printReadyMsg) {   //purpose explanation with declaration
    lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Place empty cup,");
      lcd.setCursor(0,1);
      lcd.print("make selection.");
  }
  printReadyMsg = false; //prevent spamming of lcd, after it has printed once
  //keypad input
  char keypadIn = customKeypad.getKey();
  if (keypadIn) {
    Serial.println(keypadIn);
  }
  decisionTree(keypadIn);
  Serial.println("shouldnt print");

}

void dispense(double oz, int motorNum) {
    
    double beforeDispense = LoadCell.getData();
    Serial.print("Before dispense: ");
    Serial.println(beforeDispense);
    double goalDispense = convertToScaleUnit(oz) + beforeDispense;
    Serial.print("Goal dispense: "); 
    Serial.println(goalDispense);  
    double currentDispense = beforeDispense;

    
    Serial.println(LoadCell.getData());
    while (currentDispense < goalDispense) {
      LoadCell.update();
      runMotor(true, motorNum);
      //Serial.println(LoadCell.getData());
      currentDispense = LoadCell.getData();
      
      //not working
      if (customKeypad.getKey() == '#') {
        Serial.println("##!CANCELLED!##");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("##!CANCELLED!##");
        lcd.setCursor(0,1);
        lcd.print("Rebooting...");
        runMotor(false, 4);
        delay(3000);
        resetFunc(); 
      }
    }
    runMotor(false, motorNum);
    
    delay(1000); //check weight again       if it needs more it will dispense another round
    currentDispense = LoadCell.getData();
    while (currentDispense < goalDispense) {
      LoadCell.update();
      runMotor(true, motorNum);
      //Serial.println(LoadCell.getData());
      currentDispense = LoadCell.getData();
      
      //not working
      if (customKeypad.getKey() == '#') {
        Serial.println("##!CANCELLED!##");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("##!CANCELLED!##");
        lcd.setCursor(0,1);
        lcd.print("Rebooting...");
        runMotor(false, 4);
        delay(3000);
        resetFunc(); 
      }
    }   
    
  }

  double convertToScaleUnit(double oz) {
    return oz * SCALE_OZ_FACTOR;
  }

void createBeverage(double oz1, double oz2, double oz3, String bevName) {
  if (LoadCell.getData() < 10) {
        Serial.println("No cup detected. Please place cup and try again.");
        lcd.clear();
        lcd.println("No cup detected.");
        lcd.setCursor(0,1);
        lcd.println("Please try again.");
        delay(1700);
        printReadyMsg = true;
        return;
      }
  lcd.clear();
    lcd.print(bevName);
    lcd.setCursor(0,1);
    lcd.print("selected.");
    delay(2000);
  lcd.clear();
    lcd.println("!DONT TOUCH CUP!");
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(4000);
  if (customKeypad.getKey() == '#') {
        Serial.println("##!CANCELLED!##");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("##!CANCELLED!##");
        lcd.setCursor(0,1);
        lcd.print("Rebooting...");
        runMotor(false, 4);
        delay(3000);
        resetFunc(); 
      }
  lcd.clear();
  if (oz1 > 0) {  //if there is an oz1 value dispense that
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + drink1Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(oz1, 1);
  }

   
  if (oz2 > 0) {  //if there is an oz2 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + drink2Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(oz2, 2);
  }
   
   
  if (oz3 > 0) {  //if there is an oz3 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + drink3Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(oz3, 3);
  }

  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enjoy");
    lcd.setCursor(0, 1);
    lcd.print("responsibly!");
    runMotor(false, 4); //make sure all 3 motors are off
    delay(3000);  
  printReadyMsg = true;
  
}
void decisionTree(char keyVal) {

    //dispense drink 1
    if (keyVal == '1') {
      createBeverage(d1oz1, d1oz2, d1oz3, drink1Name);
    }
    //dispense drink 2
    else if (keyVal == '2') {
      createBeverage(d2oz1, d2oz2, d2oz3, drink2Name);
    }
    //dispense drink 3
    else if (keyVal == '3') {
      createBeverage(d3oz1, d3oz2, d3oz3, drink3Name);
    }
    //dispense drink 4
    else if (keyVal == '4') {
      createBeverage(d4oz1, d4oz2, d4oz3, drink4Name);
    }
    //dispense drink 5
    else if (keyVal == '5') {
      createBeverage(d5oz1, d5oz2, d5oz3, drink5Name);
    }
    //dispense drink 6
    else if (keyVal == '6') {
      createBeverage(d6oz1, d6oz2, d6oz3, drink6Name);
    }

      
    
    else if (keyVal == '#') {
      lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("##!CANCELLED!##");
          lcd.setCursor(0,1);
          lcd.print("Rebooting...");
          delay(3000);
          resetFunc(); 
    }
    
  }
  
  void runMotor(bool motorRun, int motorNum) { //implementation of motor controller, motor 4 does not exist but results in a full shutdown of all 3 regardless of bool
    if(motorNum == 1) {
      if (motorRun) {
        digitalWrite(2, HIGH);
        //motor 1 on controller would be here
    }
      else {
      digitalWrite(2, LOW);
      //motor 1 off controller would be here
      }
    }

    else if(motorNum == 2) {
      if (motorRun) {
        digitalWrite(3, HIGH);
        //motor 2 on controller would be here
    }
      else {
      digitalWrite(3, LOW);
      //motor 2 off controller would be here
      }
    }
    
    else if(motorNum == 3) {
      if (motorRun) {
        digitalWrite(4, HIGH);
        //motor 3 on controller would be here
    }
      else {
      digitalWrite(4, LOW);
      //motor 3 off controller would be here
      }
    }

    else if(motorNum == 4) {
      //shutdown all 3 motors
      digitalWrite(2, LOW);
      digitalWrite(3, LOW);
      digitalWrite(4, LOW);
    }
        


    
  }
  

