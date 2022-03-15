#ifndef GLOBALFUNC_H
#define GLOBALFUNC_H
#include <Arduino.h>
#include <HX711_ADC.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

void(* resetFunc) (void) = 0; //reboot function

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

//Weight scale
HX711_ADC LoadCell(22, 23); 
long t;
bool cellDataCall = false;
const double SCALE_OZ_FACTOR = 20.525; 

//LCD Display
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
bool printReadyMsg = true; //used to prevent the loop function from spamming lcd


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


void dispense(double oz, int motorNum);
double convertToScaleUnit(double oz);
void decisionTree(char keyVal);
void createBeverage(double oz1, double oz2, double oz3, String bevName);
void runMotor(bool motor, int motorNum);
void dispenseShot(int motor, String bottleName);

#endif