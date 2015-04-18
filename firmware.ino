/*    
    Copyright (C) 2015  Ivan Tasner

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
*/

#include <Arduino.h>
#include "Dac.h"
#include <LiquidCrystal.h>
#include <avr/wdt.h>

LiquidCrystal lcd(8, 7, 2, 3, 4, 5);

const int inputPinLoad = A0;

const int dacUpdownPin = 9;
const int settingPin = 6; //ako je logic 0 onda ide setup struje i napona
const int inputPinPotCurrent = A4;
const int inputPinPotVoltage = A3;
const int UDacPin = A1;
const float loadResistor = 1.21;
const int divider = 6; //otporno djelilo
const float delta = 0.02; //mA

float outputVoltage = 0.00;
float setVoltage = 0.00;  
float loadVoltage = 0.00;
float loadCurrent = 0.00;

float measuredCurrent;
float vantedCurrent=1.00;
float vantedVoltage=0.00;

int rawCurrent=0;
int rawVoltage=1;

int setTmp=0;
int myloop = 0;

const int relayPin = 10; // za toggle polariteta
int relayState = 0;

unsigned long relayTick=0;

Dac dac(dacUpdownPin, UDacPin);

void setup() {
  lcd.begin(16,2); //x*y
  pinMode(settingPin,INPUT);
  pinMode(relayPin,OUTPUT);
  digitalWrite(settingPin, HIGH);
  lcd.clear();
  lcd.print("Firmware  v1.31");
  lcd.setCursor(0,1);
  lcd.print(" Grizlieureka  ");
  delay(2500);
  setValues();
  delay(2500);
  wdt_enable(WDTO_8S);
  lcd.clear();
  PrintLCD();
}

void PrintLCD(){
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print("     ");
  lcd.setCursor(2,0);
  lcd.print(outputVoltage);
  lcd.setCursor(7,0);
  lcd.print("V");
  lcd.setCursor(9,0);
  lcd.print("(");
  lcd.print(vantedVoltage);
  lcd.print(")");
  lcd.setCursor(0,1);
  lcd.print("I=");
  lcd.print(loadCurrent);
  lcd.setCursor(6,1);
  lcd.print("mA");
  lcd.print("      "); //obrisem ostala polja
  lcd.setCursor(9,1);
  lcd.print("(");
  lcd.print(vantedCurrent);
  lcd.print(")");
  lcd.print(" ");
}

void getLoad(){
  int readTmp;
  readTmp = analogRead(inputPinLoad);
  lcd.setCursor(0,1);
  loadVoltage = ((float)readTmp/1024)*5; //1.1; // mV : 0 -5 V na teretu od 1k21, 5V je 5mA
//  loadCurrent = (float)loadVoltage/loadResistor; // ImA=mV/R : 
  loadCurrent = (float)loadVoltage/loadResistor; // ImA = V*1.21 [mA]
  measuredCurrent = loadCurrent;
}

void getOutputVoltage(){
  int rawData;
  rawData = analogRead(UDacPin);
  outputVoltage = (float)rawData/1024*5.00*divider;
}

void setOutputVoltage(){
  setTmp = (vantedCurrent - measuredCurrent)*15 + setTmp;
  if (setTmp > rawVoltage){
    setTmp = rawVoltage;
  }
  if (setTmp > 1024) setTmp = 1024;
  if (setTmp < 0) setTmp = 0;
  dac.write(setTmp);
  delay(1);
  dac.write(setTmp);
}

int needCalibration(){
  if(abs(measuredCurrent - vantedCurrent) > delta){
    return 1;
  } else {
    return 0;
  }
}

void setValues(){
  rawCurrent = analogRead(inputPinPotCurrent);
  rawVoltage = analogRead(inputPinPotVoltage);
  vantedCurrent = 4.00*(rawCurrent/1024.0); // 0 - 4 mA
  vantedVoltage = 5.00*divider*(rawVoltage/1024.0); // 0 - 5*divider Volts
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print(vantedCurrent);
  lcd.setCursor(4,1);
  lcd.print("mA");
  lcd.setCursor(10,1);
  lcd.print(vantedVoltage);
  lcd.print("V");
}

void relayToggle(){
   if (relayState == 1){
    digitalWrite(relayPin,LOW);
    relayState = 0;
  }else{
    digitalWrite(relayPin,HIGH);
    relayState = 1;
  }
}

void loop() {
  if (digitalRead(settingPin) == LOW ){ //ako je postavljen setup mode
    delay(100);
    if (digitalRead(settingPin) == LOW ){
      dac.write(setTmp);
      delay(1);
      dac.write(setTmp);
      lcd.setCursor(0,0);
      lcd.print("** Setup Mode **");
      setValues();
      delay(20);
    }
  }
  else{
    
    if (myloop > 256){
      getOutputVoltage();
      PrintLCD();
      myloop = 0;
    }
 
    getLoad();
    setOutputVoltage();
    myloop = myloop + 1;
  }
  if (relayTick > 21942){ // 512 je 7 sec, 
    relayTick = 0;
    relayToggle();
  }
  relayTick = relayTick + 1;
  delay(1);
  wdt_reset();
}
