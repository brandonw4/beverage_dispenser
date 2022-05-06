//Libraries
#include <HX711_ADC.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

//Beverage Class
const int maxMixers = 6; //size of mixer arrays
class Beverage {
    public:
    Beverage(String n, double oz1, double oz2, double oz3, double oz4, double oz5, double oz6);
    Beverage(String n, double oz1, double oz2, double oz3);
    double ozArr[maxMixers];
    String name;
};

Beverage::Beverage(String n, double oz1, double oz2, double oz3) {
  name = n;
  ozArr[0] = oz1;
  ozArr[1] = oz2; 
  ozArr[2] = oz3;
}

Beverage::Beverage(String n, double oz1, double oz2, double oz3, double oz4, double oz5, double oz6){
  name = n;
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

void dispense(double oz, int motorNum);
double convertToScaleUnit(double oz);
void decisionTree(char keyVal);
void createBeverage(Beverage bev);
void runMotor(bool motor, int motorNum);
void dispenseShot(int motor, String bottleName);

//init drinks
Beverage bev1("testDrink1", 1.5, 1.5, 1.5);
Beverage bev2("testDrink2", 1.5, 1.5, 1.5);
Beverage bev3("testDrink3", 1.5, 1.5, 1.5);
Beverage bev4("testDrink4", 1.5, 1.5, 1.5);
Beverage bev5("testDrink5", 1.5, 1.5, 1.5);
Beverage bev6("testDrink6", 1.5, 1.5, 1.5);


//bottles
const String bottle1Name = "b1Name";
const String bottle2Name = "b2Name";
const String bottle3Name = "b3Name";
const String bottle4Name = "b4Name";
const String bottle5Name = "b5Name";
const String bottle6Name = "b6Name";





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
  decisionTree(keypadIn);

  if (cellDataCall) {     //diagnostic: prints scale data to serial console. Controlled by C on, D off.
    Serial.println(LoadCell.getData());
  }


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

void createBeverage(Beverage bev) {
  if (LoadCell.getData() < 4) {
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

  if (bev.ozArr[3] > 0) {  //if there is an oz3 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle4Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[3], 3);
  }

  if (bev.ozArr[4] > 0) {  //if there is an oz3 value dispense that
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + bottle5Name);
    lcd.setCursor(0,1);
    lcd.println("# key to cancel.");
    delay(1500);
    dispense(bev.ozArr[4], 3);
  }

  if (bev.ozArr[5] > 0) {  //if there is an oz3 value dispense that
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
    runMotor(false, 4); //make sure all 3 motors are off
    delay(3000);  
  printReadyMsg = true;
  
}
void decisionTree(char keyVal) {

    //dispense drink 1
    if (keyVal == '1') {
      createBeverage(bev1);
    }
    //dispense drink 2
    else if (keyVal == '2') {
      createBeverage(bev2);
    }
    //dispense drink 3
    else if (keyVal == '3') {
      createBeverage(bev3);
    }
    //dispense drink 4
    else if (keyVal == '4') {
      createBeverage(bev4);
    }
    //dispense drink 5
    else if (keyVal == '5') {
      createBeverage(bev5);
    }
    //dispense drink 6
    else if (keyVal == '6') {
      createBeverage(bev6);
    }
    //dispense shot of bottle 1
    else if (keyVal == '7') {
      dispenseShot(1, bottle1Name);
    }
    //dispense shot of bottle 2
    else if (keyVal == '8') {
      dispenseShot(1, bottle2Name);
    }
    //dispense shot of bottle 3
    else if (keyVal == '9') {
      dispenseShot(1, bottle3Name);
    }
    else if (keyVal == 'C') {   //cellDataCall controls print data to console variable
      cellDataCall = true;
    }
    else if (keyVal == 'D') {
      cellDataCall = false;
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
  
void dispenseShot(int motor, String bottleName) {
  if (LoadCell.getData() < 1) {   //number is smaller due to small plastic solo shot cups
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
  runMotor(false, 4); //make sure all 3 motors are off
  delay(3000);  
  printReadyMsg = true;
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
  

