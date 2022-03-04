//Libraries
#include <HX711_ADC.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include 'bev_func.h'

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

//reboot function
void(* resetFunc) (void) = 0;

void setup() {

}

void loop() {
    
}