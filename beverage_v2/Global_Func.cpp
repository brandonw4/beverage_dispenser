#include "GlobalFunc.h"



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

void createBeverage(double oz1, double oz2, double oz3, String bevName) {
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
  dispense(1.5, motor);

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