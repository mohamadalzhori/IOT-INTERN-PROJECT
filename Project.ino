#define BLYNK_TEMPLATE_ID "TMPL6kyB5rSz1"
#define BLYNK_TEMPLATE_NAME "Home Automation"
#define BLYNK_AUTH_TOKEN "wK4OqglkPc9EasX809ZAl1Oac9TfF1Np"


// Including libraries for Blynk
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>

// Including Libraries for the LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define ON                      1
#define OFF                     0


// TMPERATURE SYSTEM MACROS-------
#define LDR_Sensor              A1
#define Garden_Light            3 // PIN 3 HAS PWM
#define Heater                  5
#define Cooler                  4  
//-------------------------------

#define Temperature_Pin         A0



#define Cooler_Pin              V0
#define Temperature_Sensor      V1
#define Heater_Pin              V2
#define Volume                  V3
#define Inlet_Valve             V4
#define Outlet_Valve            V5
#define Notification            V6




// SERIAL TANK MACROS---------------
// DIGITAL OUTPUTS
#define   INLET_VALVE           0X00
#define   OUTLET_VALVE          0X01

// DIGITAL INPUTS
#define   HIGH_FLOAT            0X10
#define   LOW_FLOAT             0X11

// ANALOG INPUTS
#define   VOLUME                0X30

// INSTRUCTIONS 

#define ENABLE                  0X01
#define DISABLE                 0X00
//----------------------------------



unsigned int Brightness;
bool heater_sw, cooler_sw, inlet_sw, outlet_sw;

unsigned int volume_value;
unsigned char valueh, valuel;

char auth[] = BLYNK_AUTH_TOKEN;

BlynkTimer timer;

// Create an LCD object with I2C address of 0x27, with 16 columns and 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);


// Read the value of the Cooler virtual pin and control the cooler accordingly, + notifications on LCD
BLYNK_WRITE(Cooler_Pin){ // this method is called only on state change
  cooler_sw = param.asInt();
  if (cooler_sw)
  {
    cooler_ctrl(ON);
    lcd.setCursor(7, 0);
    lcd.print("CO_LR ON ");
    
  }
  else
  {
    cooler_ctrl(OFF);
    lcd.setCursor(7, 0);
    lcd.print("CO_LR OFF");
  }
  
}


// Read the value of the Heater virtual pin and control the Heater accordingly, + notifications on LCD
BLYNK_WRITE(Heater_Pin){ // this method is called only on state change
  heater_sw = param.asInt();
  if (heater_sw)
  {
    heater_ctrl(ON);
    lcd.setCursor(7, 0);
    lcd.print("HE_TR ON ");
  }
  else
  {
    heater_ctrl(OFF);
    lcd.setCursor(7, 0);
    lcd.print("HE_TR OFF");
  }
  
}


// Read the value of the Inlet virtual pin and control the Inlet accordingly
BLYNK_WRITE(Inlet_Valve){ // this method is called only on state change
  inlet_sw = param.asInt();
  if (inlet_sw)
  {
    enable_Inlet();
    lcd.setCursor(7, 1);
    lcd.print("IN_FL ON ");
  }
  else
  {
    disable_Inlet();
    lcd.setCursor(7, 1);
    lcd.print("IN_FL OFF ");
  }

}

// Read the value of the Outlet virtual pin and control the Outlet accordingly
BLYNK_WRITE(Outlet_Valve){ // this method is called only on state change
  outlet_sw = param.asInt();
  if (outlet_sw)
  {
    enable_outlet();
    lcd.setCursor(7, 1);
    lcd.print("OT_FL ON ");
  }
  else
  {
    disable_outlet();
    lcd.setCursor(7, 1);
    lcd.print("OT_FL OFF ");

  }

}






void setup() 
{
  Blynk.begin(auth);
  // Initialize_LCD
  Init_LCD();

  // Initilize Garden_Lights_Controller
  Init_Garden_Lights();

  // Initialize_Temperature_System
  Init_Temp_Sys();

// Initialize_Serial_Tank
  Init_Serial_Tank();

  
  timer.setInterval(500L, Update_Temp_Vol_Guage);
  timer.setInterval(5000L, TidyUP);// Not Requied, it's just to clean up the Dashboard

}

void loop() 
{
  Blynk.run();
  timer.run();

  // Read the temp and display on screen
  lcd.setCursor(2,0);
  lcd.print(String(Read_Temperature(),2));
  
  volume_value = get_volume();
  lcd.setCursor(2,1);
  lcd.print(volume_value);
  lcd.setCursor(6,1); // Had to add it to avoid a bug in the LCD
  lcd.print(" ");

  Brightness_Control(); // A function to control the brightness of the Garden Lights
  handle_temp();// Conpare the temperature to a threshold and do something
  handle_tank();// Compare the volume to a threshold and do something

  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.setCursor(0,1);
  lcd.print("V:");

  
  
  delay (500);
  
  

  
 

  
  
  

}















// Garden Light Functions ---------------------------------------------

void Init_Garden_Lights(){
  // Initilize Garden_Lights_Controller
  pinMode(Garden_Light, OUTPUT);
}

void Brightness_Control(){

  Brightness = analogRead(LDR_Sensor);// Read the Brightness value from the LDR sensor
  Brightness = map(Brightness, 0, 1023, 0, 255); // convert the Value form 0-1023 to 0-255
  Brightness = 255 - Brightness;// Flip the Brightness value so that the led is brighter at night 

  analogWrite(Garden_Light, Brightness);

}

// --------------------------------------------------------------------

// Temperature System Functions ---------------------------------------

void Init_Temp_Sys(){
  // Initialize_Temperature_System
  pinMode(Heater, OUTPUT);
  pinMode(Cooler, OUTPUT);
}

float Read_Temperature(){
  float temp = analogRead(Temperature_Pin);
  temp = temp * (5.0/1024)/0.01;// Sensor give 0-1024, we convert it to 0-5V, then divide by 0.01V= 10mv for each Degree Celsius
  return temp;
}

void cooler_ctrl(bool state){

  if(state){
    digitalWrite(Cooler, ON);
  }else{
    digitalWrite(Cooler, OFF);
  }
    
}

void heater_ctrl(bool state){
  
  if(state){
    digitalWrite(Heater, ON);
  }else{
    digitalWrite(Heater, OFF);
  }
    
}

void Update_Temp_Vol_Guage(){

  Blynk.virtualWrite(Temperature_Sensor,Read_Temperature()); // both are float
  Blynk.virtualWrite(Volume,get_volume());

}

void handle_temp(void){ // Read the temperature and check with 35 deg

  if((Read_Temperature() > 35.0) && heater_sw){

    heater_sw = 0;// Update heater on arduino side
   
    heater_ctrl(OFF);
    lcd.setCursor(7, 0);
    lcd.print("HE_TR OFF"); 
    
    Blynk.virtualWrite(Notification, "Temperature is above 35 degrees celsius\n Turning OFF the Heater\n");

    Blynk.virtualWrite(Heater_Pin, 0); // To reflect the Heater status on the Cloud
  }
} 

// --------------------------------------------------------------------


// LCD ----------------------------------------------------------------
void Init_LCD(){
  // Initialize_LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.home();


}

// --------------------------------------------------------------------

// SERIAL TANK --------------------------------------------------------

void Init_Serial_Tank(){

  Serial.begin(19200);// set the designers of the Serial Tank
  Serial.write(0xFF);
  Serial.write(0xFF);
  Serial.write(0xFF);
  
}

void enable_Inlet(){
  Serial.write(INLET_VALVE);
  Serial.write(ENABLE);
}

void enable_outlet(){
  Serial.write(OUTLET_VALVE);
  Serial.write(ENABLE);
}

void disable_Inlet(){
  Serial.write(INLET_VALVE);
  Serial.write(DISABLE);
  
}

void disable_outlet(){
  Serial.write(OUTLET_VALVE);
  Serial.write(DISABLE);
  
}

unsigned int get_volume(){ 
  Serial.write(VOLUME); 
  valueh = Serial.read();
  valuel = Serial.read();
  return (valueh << 8 | valuel) ;
} 

void handle_tank(void){ 
  // if volume is less than 2000 and the inlet is off then execute
  if((volume_value < 2000) && !inlet_sw){
    enable_Inlet();

    inlet_sw = 1;// Update inlet on arduino side
   
    lcd.setCursor(7, 1);
    lcd.print("IN_FL ON "); 
    
    Blynk.virtualWrite(Notification, "Volume is below 2000 Liters\n Turning ON the Inlet Valve \n");

    Blynk.virtualWrite(Inlet_Valve, 1); // To reflect the Inlet status on the Cloud
  }

  if((volume_value == 3000) && inlet_sw){
    disable_Inlet();

    inlet_sw = 0;// Update inlet on arduino side
   
    lcd.setCursor(7, 1);
    lcd.print("IN_FL OFF "); 
    
    Blynk.virtualWrite(Notification, "Volume is 3000 Liters\n Turning OFF the Inlet Valve \n");

    Blynk.virtualWrite(Inlet_Valve, 0); // To reflect the Inlet status on the Cloud
  }
} 


// --------------------------------------------------------------------

void TidyUP(){
  lcd.setCursor(7,0);
  lcd.print("         ");
  lcd.setCursor(7,1);
  lcd.print("         ");
}





