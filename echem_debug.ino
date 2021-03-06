/* Arduino test code for Shaun Houlihan/Nerds For Nature E-Chem toxic gas sensor board.
Creative commons non-commercial/by attribution license
written by Ken McGary and others where noted
v0.1  1/21/2014 Initial testing of board, do I2C devices work?
v0.2  1/30/14 Update for Leonardo board, integrate alphasense inputs and add csv output

*/


#include <Wire.h>

#include <Adafruit_ADS1015.h>
#include <Wire.h>
//#include "LMP91000.h"

#define sampleDelay 1000


/*
LM91000.h header info, which is here now but should go into a library someday.
/ Created by: Sindre Søpstad
*/

//#define LMP91000_SLV_ADDR_WRITE 0x48
//#define LMP91000_SLV_ADDR_READ 0x48
#define LMP91000_Slave_Address 0x48

#define LMP91000_V2 1154.0 // TEMP_MAX_MV from LMP91000 temp data table 50C
#define LMP91000_T2 50.0   // TEMP_MAX in degC
#define LMP91000_V1 1480.0 // TEMP_MIN_MV from LMP91000 temp data table 10C
#define LMP91000_T1 10.0   // TEMP_MIN in degC


#define STATUS 0x00
#define LOCKCN 0x01

#define TIACN 0x10
#define REFCN 0x11
#define MODECN 0x12

#define Fet_Short_Enabled 0x80
#define Deep_sleep 0x00
#define Two_lead_gnd 0x01
#define Standby 0x02
#define Three_Lead_Amperometric 0x03
#define Temperature_meas_TIA_Off 0x06
#define Temperature_meas_TIA_On 0x07


#define Write_TIACN_REFCN 0x00
#define Read_TIACN_REFCN 0x01


#define Gain_External_Resistance 0x00
#define Gain_2p75KOhm 0x04
#define Gain_3p5KOhm 0x08
#define Gain_7KOhm 0x0C
#define Gain_14KOhm 0x10
#define Gain_35KOhm 0x14
#define Gain_120KOhm 0x18
#define Gain_350KOhm 0x1C

#define Rload_10_Ohm 0x00
#define Rload_33_Ohm 0x01
#define Rload_50_Ohm 0x02
#define Rload_100_Ohm 0x03


#define Internal_Vref 0x00
#define External_Vref 0x80

#define Internal_Zero_20 0x00
#define Internal_Zero_50 0x20
#define Internal_Zero_67 0x40
#define Internal_Zero_Bypassed 0x60

#define Bias_Negative 0x00
#define Bias_Positive 0x10

#define Bias_0_Percent 0x00
#define Bias_1_Percent 0x01
#define Bias_2_Percent 0x02
#define Bias_4_Percent 0x03
#define Bias_6_Percent 0x04
#define Bias_8_Percent 0x05
#define Bias_10_Percent 0x06
#define Bias_12_Percent 0x07
#define Bias_14_Percent 0x08
#define Bias_16_Percent 0x09
#define Bias_18_Percent 0x0A
#define Bias_20_Percent 0x0B
#define Bias_22_Percent 0x0C
#define Bias_24_Percent 0x0D



boolean tempFlag = false;  //true if reading temperature from LM91000, otherwise false
unsigned long loopCount = 0;
double volttemp = 0;
double ppm = 0;
double ppmUUT;
//double isbCh2 = 0;
#define isbWsens 545e-9  //  nA/ppm
#define isbVsens 436  // mv/ppm
#define isbWzero 329  // mV
#define isbAuxzero 353  // mV



Adafruit_ADS1115 ads(0x49);  /* Use this for the 16-bit version */
// Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */

void setup(void) 
{
  Serial.begin(9600);
  while (!Serial) ; //added to wait for for leonardo startup serial port / USB swap
  Serial.println("Hello!");
  
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range:0.0 to +6.144V (1 bit = 0.1875mV)");
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
   ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
//  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
 //  ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
  ads.begin();
double chiptemp = 0;


/* Temp calculation equation derivation from TI LM91000 datasheet, pp 14/15:

http://www.ti.com/lit/ds/symlink/lmp91000.pdf

Although the temperature sensor is very linear, its response does have a slight downward parabolic shape. This
shape is very accurately reflected in Table 1. For a linear approximation, a line can easily be calculated over the
desired temperature range from Table 1 using the two-point equation:

V-V1=((V2–V1)/(T2–T1))*(T-T1)
Where V is in mV, T is in °C, T1 and V1 are the coordinates of the lowest temperature, T2 and V2 are the
coordinates of the highest temperature.
For example, if we want to determine the equation of a line over a temperature range of 20°C to 50°C, we would
proceed as follows:
*/



Wire.begin();
LMP_CFG();

}

void loop(void) 
{
  int16_t adc0, adc1, adc2, adc3;
  double volt0, volt1, volt2, volt3;
  double chiptemp = 0;

// wait here for next sample time
while (millis() < loopCount*sampleDelay){
delay(1);
}

// here we go
loopCount++;
tempFlag=false;
  LMP_MODE();

  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  
  volt0 = adc0 * 0.1875;
  volt1 = adc1 * 0.1875;
  volt2 = adc2 * 0.1875;
  volt3 = adc3 * 0.1875;
   //ppm_COa = volt0*   // calculate actual ppm 
   
// calculate actual temp from raw value   
tempFlag=true;
  LMP_MODE();
  adc0 = ads.readADC_SingleEnded(0);
  volttemp = adc0 * 0.1875;
  chiptemp = ((volttemp-LMP91000_V1)/((LMP91000_V2-LMP91000_V1)/(LMP91000_T2-LMP91000_T1)))+LMP91000_T1;

// calculate actual gas concentrations based on gain/offset
  ppm = ((volt3-isbWzero)-(volt2-isbAuxzero))/isbVsens;  // this is the alphasense reference sensor on the extra a/d inputs
  ppmUUT = ((volt0-2550)*.02311)+0.2; // this is the sensor in the breakout board socket, with hard gain/offset value plugged in
 
 // print raw a/d values and fixed values to serial port
  Serial.print(loopCount);Serial.print(","); Serial.print(volt0,2);
  Serial.print(","); Serial.print(volt1,2);
  Serial.print(","); Serial.print(volt2,2);
  Serial.print(","); Serial.print(volt3,2);
  Serial.print(","); Serial.print(chiptemp,2);
  Serial.print(","); Serial.print(ppm,2);
Serial.print(","); Serial.println(ppmUUT,2);

 /* Serial.print("AIN0-3: "); Serial.print(adc0);
  Serial.print(" "); Serial.print(adc1);
  Serial.print(" "); Serial.print(adc2);
  Serial.print(" "); Serial.println(adc3);*/
/* Serial.print("AIN0: "); Serial.println(adc0);
  Serial.print("AIN1: "); Serial.println(adc1);
  Serial.print("AIN2: "); Serial.println(adc2);
  Serial.print("AIN3: "); Serial.println(adc3);*/
//  Serial.println(" ");
  
  }
  // LM91000 test code below copied from TI forum  http://e2e.ti.com/support/interface/etc_interface/f/146/t/258263.aspx


//  delay(sampleDelay);
//Serial.print(analogRead(A8));
//Serial.print(",");
//delay(sampleDelay);


// MOSFET control logic and test measurements
//delay(sampleDelay);
//Serial.print(analogRead(A6));
//Serial.print(",");
//Serial.println(analogRead(A5));
//Serial.println(analogRead(A2)*5.0/1024);



/*                                               */
/* Configure LMP91000 for the intended operation */
/*                                               */
void LMP_MODE(void)
{ 
/* MODECN register: Configure device mode to Three Lead Amperometric*/

//Wire.beginTransmission(LMP91000_Slave_Address);
/* LOCKN register: Unlock for configuring TIACN and REFCN */
//Wire.write(LOCKCN);
//Wire.write(Write_TIACN_REFCN); // Unlock 
//Wire.endTransmission();
if (tempFlag==true) {
// Serial.print(tempFlag);
 Wire.beginTransmission(LMP91000_Slave_Address);
  Wire.write(MODECN);
  Wire.write(Temperature_meas_TIA_On); // temperature output mode on
  Wire.endTransmission();
}
else if (!tempFlag) {
//Serial.print(tempFlag);
//  Wire.beginTransmission(LMP91000_Slave_Address);
//  Wire.write(MODECN);
//  Wire.write(Temperature_meas_TIA_Off); // temperature output mode off
//  Wire.endTransmission();
delay (10);
  Wire.beginTransmission(LMP91000_Slave_Address);
  Wire.write(MODECN);
  Wire.write(Three_Lead_Amperometric); // 3-wire galvanic mode
  Wire.endTransmission();
}

}

/*                                               */
/* Configure LMP91000 for the intended operation */
/*                                               */
void LMP_CFG(void)
{ 
/* MODECN register: Configure device mode to Three Lead Amperometric*/

Wire.beginTransmission(LMP91000_Slave_Address);
/* LOCKN register: Unlock for configuring TIACN and REFCN */
Wire.write(LOCKCN);
Wire.write(Write_TIACN_REFCN); // Unlock 
Wire.endTransmission();
//  Wire.beginTransmission(LMP91000_Slave_Address);
//  Wire.write(MODECN);
//  Wire.write(Temperature_meas_TIA_Off); // temperature output mode
//  Wire.endTransmission();
 
  Wire.beginTransmission(LMP91000_Slave_Address);
  Wire.write(MODECN);
  Wire.write(Three_Lead_Amperometric); // 3-wire galvanic mode
  Wire.endTransmission();




/* Sets the Reference Control Register to select the following parameters:
* External reference voltage | Internal zero selection = VREF*0.2 | 
* Bias polarity = positive | BIAS selection = VREF*0.24 */
Wire.beginTransmission(LMP91000_Slave_Address);
Wire.write(REFCN);
Wire.write(Internal_Vref | Internal_Zero_50 | Bias_Negative | Bias_0_Percent);
//Wire.write(Internal_Vref | Internal_Zero_20 | Bias_Positive | Bias_0_Percent); 
Wire.endTransmission();

/* Sets the TIA Control Register to select 350kohm gain resistance
* and load resistance (100 ohm)*/
Wire.beginTransmission(LMP91000_Slave_Address);
Wire.write(TIACN);
Wire.write(Gain_350KOhm | Rload_10_Ohm); 
Wire.endTransmission();
