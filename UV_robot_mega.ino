#include "PWM.hpp"
#include "pitches.h"
#include "pins.h"

int forward_pwm, turn_pwm, uv_pwm, arm_pwm;
int pwm_min = 820;                          // minimum pwm value for validation
int pwm_max = 2200;                         // maximum pwm value for validation
int switch_on_pwm = 1800;                   // minimum pwm to detect a turned on switch
double max_age = 50000;                        //max timeout age of rc link

float battery_min_voltage = 10.4;           // minimum battery voltage for low alarm
float battery_critical_voltage = 9.8;       //battery voltage for system cutoff


bool failsafe   = false;                    //rc link fail
bool batterylow = false;                    //battery monitor
bool batterycritical = false;               //battery critically low

//detect whether the robot moving or not to monitor the battery level. 
//battery level drops considerably when motors are running.
bool robot_moving  = false;                    


bool uv_on      = false;                    //to differentiate buzzer tones based on the state

PWM ch1(18);                                // Setup pin 18 for input
PWM ch2(19);                                // Setup pin 19 for input
PWM ch3(20);                                // Setup pin 20 for input
PWM ch4(21);                                // Setup pin 21 for input

void setup() {
  Serial.begin(115200);                     // Serial for debug

  ch1.begin(true);                          // ch1 on pin 2 reading PWM HIGH duration
  ch2.begin(true);                          // ch2 on pin 3 reading PWM HIGH duration
  ch3.begin(true);                          // ch3 on pin 18 reading PWM HIGH duration
  ch4.begin(true);                          // ch4 on pin 19 reading PWM HIGH duration

  pinMode(buzz_pin,OUTPUT);                 //connected to the buzzer
  pinMode(system_led, OUTPUT);              //system okay led

  pinMode(uv_relay_pin,OUTPUT);             //connected to the uv relay
  pinMode(led_relay_pin, OUTPUT);           //connected to the led relay

  digitalWrite(uv_relay_pin,HIGH);
    
  pinMode(bat_led1, OUTPUT);                //battery monitor bar led lowest level
  pinMode(bat_led2, OUTPUT);
  pinMode(bat_led3, OUTPUT);
  pinMode(bat_led4, OUTPUT);
  pinMode(bat_led5, OUTPUT);

  startup_tone();
  delay(4000);

  //check battery voltage
  battery_voltage_monitor();
  while(batterylow == true){
    battery_voltage_monitor();
    low_battery_notification();
    delay(10);
  }


  //verify the uv swtich is turned off 
  rc_read();
  while(uv_pwm > switch_on_pwm){
    rc_read();
    uv_on_warning_at_startup(); //buzzer tone
    delay(10);
  }



  //uv is turned off and battery voltage is good play the tone
  delay(1000);
  system_okay_melody();

}

void loop() {

if (robot_moving == false){
  battery_voltage_monitor();    //monitor the battery when robot is not moving
}
      rc_read();
      //rc_validate();

      //if rc link okay proceed to next system functions
      if (failsafe == false){
        if (arm_pwm > switch_on_pwm){      //if arm switch is on

            motor_control();
            uv_light_control();
        }
        else
        {
            system_cutoff();              //if system is not armed
        }
      }

//when failsafe on
      else{
        failsafe_cutoff();
      }

  //clearing pwm values
  forward_pwm = 0;
  turn_pwm    = 0;
  arm_pwm     = 0;
  uv_pwm      = 0;

  delay(10);
}

//read radio reciever values
void rc_read() {
  
  forward_pwm = ch3.getValue();
  turn_pwm    = ch4.getValue();
  arm_pwm     = ch2.getValue();
  uv_pwm      = ch1.getValue();

  double age1 = ch1.getAge();
  double age2 = ch2.getAge();
  double age3 = ch3.getAge();
  double age4 = ch4.getAge();

  //Serial.print(age1);Serial.print(" ");
  //Serial.print(age2);Serial.print(" ");
  //Serial.print(age3);Serial.print(" ");
  //Serial.print(age4);Serial.println(" ");
  
  
  if ((age1 > max_age) and (age2 > max_age) and (age3 > max_age) and (age4 > max_age)){
    //Serial.println("inside this");
    failsafe = true;
  }

  else{
    failsafe = false;
  }


}

//validate radio receiver pwms
void rc_validate() {

  bool forward_okay, turn_okay, arm_okay, uv_okay;

  if (forward_pwm > pwm_min and forward_pwm < pwm_max)   forward_okay = true;
  else  forward_okay = false;

  if (turn_pwm > pwm_min and turn_pwm < pwm_max)         turn_okay = true;
  else  turn_okay = false;

  if (arm_pwm > pwm_min and arm_pwm < pwm_max)           arm_okay = true;
  else  arm_okay = false;

  if (uv_pwm > pwm_min and uv_pwm < pwm_max)             uv_okay = true;
  else  uv_okay = false;

  if (forward_okay and turn_okay and arm_okay and uv_okay)  failsafe = false;
  else failsafe = true;

//blink the system led if the rc link is okay
  if (failsafe == false)  {
    digitalWrite(system_led,HIGH);
  }
  else digitalWrite(system_led,LOW); 
}

void motor_control() {

  //----------------------------------------------------------------------------
  //go forward full forward
  //----------------------------------------------------------------------------
  if (forward_pwm > 1800 & forward_pwm < 2500 & turn_pwm > 1300 & turn_pwm < 1800) {

    digitalWrite(m1a, HIGH);
    digitalWrite(m2a, HIGH);
    digitalWrite(m1b, LOW);
    digitalWrite(m2b, LOW);
    analogWrite(m1en, forward_speed);
    analogWrite(m2en, forward_speed);

    robot_moving = true;
    buzzer_alarm();

  }


  //----------------------------------------------------------------------------
  //turn left
  //----------------------------------------------------------------------------
  else if (forward_pwm > 1400 & forward_pwm < 1650 & turn_pwm < 1300) {

    digitalWrite(m1a, LOW);
    digitalWrite(m2a, HIGH);
    digitalWrite(m1b, HIGH);
    digitalWrite(m2b, LOW);
    analogWrite(m1en, low_speed);
    analogWrite(m2en, turn_speed);

    robot_moving = true;
     buzzer_alarm();

  }

  //----------------------------------------------------------------------------
  //turn right
  //----------------------------------------------------------------------------
  else if (forward_pwm > 1400 & forward_pwm <1650 & turn_pwm > 1750) {

    digitalWrite(m1a, HIGH);
    digitalWrite(m2a, LOW);
    digitalWrite(m1b, LOW);
    digitalWrite(m2b, HIGH);
    analogWrite(m1en, turn_speed);
    analogWrite(m2en, low_speed);

    robot_moving = true;
     buzzer_alarm();

  }

  //----------------------------------------------------------------------------
  //turn right while moving forward
  //----------------------------------------------------------------------------
  else if (forward_pwm > 1800 & turn_pwm > 1750) {

    digitalWrite(m1a, HIGH);
    digitalWrite(m2a, HIGH);
    digitalWrite(m1b, LOW);
    digitalWrite(m2b, LOW);
    analogWrite(m1en, forward_speed);
    analogWrite(m2en, low_speed);

    robot_moving = true;
     buzzer_alarm();
  }


  //----------------------------------------------------------------------------
  //turn left while moving forward
  //----------------------------------------------------------------------------
  else if (forward_pwm > 1800 & turn_pwm < 1300) {

    digitalWrite(m1a, HIGH);
    digitalWrite(m2a, HIGH);
    digitalWrite(m1b, LOW);
    digitalWrite(m2b, LOW);
    analogWrite(m1en, low_speed);
    analogWrite(m2en, forward_speed);

    robot_moving = true;
     buzzer_alarm();

  }
  //---------------------------------------------------------------------------
  //reverse low speed
  //---------------------------------------------------------------------------
  else if (forward_pwm < 1200 & turn_pwm > 1300 & turn_pwm < 1800) {

    digitalWrite(m1a, LOW);
    digitalWrite(m2a, LOW);
    digitalWrite(m1b, HIGH);
    digitalWrite(m2b, HIGH);
    analogWrite(m1en, forward_speed);
    analogWrite(m2en, forward_speed);

    robot_moving = true;
     buzzer_alarm();
  }

  else {
    digitalWrite(m1a, LOW);
    digitalWrite(m2a, LOW);
    digitalWrite(m1b, LOW);
    digitalWrite(m2b, LOW);
    digitalWrite(m1en, LOW);
    digitalWrite(m2en, LOW);

    robot_moving = false;
  }

}

void system_cutoff() {
  //motor cutoff--------------------------
  digitalWrite(m1a, LOW);
  digitalWrite(m2a, LOW);
  digitalWrite(m1b, LOW);
  digitalWrite(m2b, LOW);
  digitalWrite(m1en, LOW);
  digitalWrite(m2en, LOW);

  digitalWrite(uv_relay_pin, HIGH);          //turn off uv uv_relay_pin
  digitalWrite(led_relay_pin, HIGH);         //turn on green color led stripes
}


void failsafe_cutoff() {
  //motor cutoff--------------------------
  digitalWrite(m1a, LOW);
  digitalWrite(m2a, LOW);
  digitalWrite(m1b, LOW);
  digitalWrite(m2b, LOW);
  digitalWrite(m1en, LOW);
  digitalWrite(m2en, LOW);

  digitalWrite(uv_relay_pin, HIGH);          //turn off uv uv_relay_pin

  digitalWrite(led_relay_pin, HIGH);         //turn on green color led stripes
  delay(1000);
  digitalWrite(led_relay_pin, LOW);         //turn on green color led stripes
  delay(1000);
}

void uv_light_control(){
  
  if (uv_pwm > switch_on_pwm and batterycritical == false){
    digitalWrite(uv_relay_pin, LOW);       //turn on the ups and the uv uv_light_control
    digitalWrite(led_relay_pin, LOW);      //switch the green led stripe to red
    uv_on = true;                           //turn on the uv_on flag
    buzzer_alarm();                         //buzzer notification for uv on
  }

  else if (uv_pwm < switch_on_pwm - 150){
    digitalWrite(uv_relay_pin, HIGH);        //turn off the ups and the uv uv_light_control
    digitalWrite(led_relay_pin, HIGH);       //switch the red led stripe to green
    uv_on = false;                          //turn off the uv_on flag
  }

}
//battery level display from the bar led and battery low flag adjust
void battery_voltage_monitor(){

  float voltage = 0;
  
  for (int i=0;i<5;i++){
    int v_read = analogRead(bat_monitor_pin);
    float voltage_temp = (v_read*0.005)*(res_high+res_low)/res_low;
    voltage = voltage + voltage_temp;
  }

  voltage = voltage / 5;
  Serial.println(voltage);

  if (voltage < battery_min_voltage){ //battery low detection
    batterylow = true;

    low_battery_notification();     //buzzer and battery level indicator
  }

  else if (voltage > battery_min_voltage + 0.2){
    batterylow = false;
  }

  if (voltage < battery_critical_voltage){
    system_cutoff();
    batterycritical = true;
  }

  else if (voltage > battery_critical_voltage + 2.0){
    batterycritical = false;
  }

  if (voltage > battery_min_voltage and voltage < battery_min_voltage + 1.35){        //battery level 1
    digitalWrite(bat_led1, HIGH);
    digitalWrite(bat_led2, LOW);
    digitalWrite(bat_led3, LOW);
    digitalWrite(bat_led4, LOW);
    digitalWrite(bat_led5, LOW);
  }

  if (voltage > battery_min_voltage + 1.4 and voltage < battery_min_voltage + 1.95){  //battery level 2
    digitalWrite(bat_led1, HIGH);
    digitalWrite(bat_led2, HIGH);
    digitalWrite(bat_led3, LOW);
    digitalWrite(bat_led4, LOW);
    digitalWrite(bat_led5, LOW);
  }

  if (voltage > battery_min_voltage + 2.0 and voltage < battery_min_voltage + 2.65){ //battery level 3
    digitalWrite(bat_led1, HIGH);
    digitalWrite(bat_led2, HIGH);
    digitalWrite(bat_led3, HIGH);
    digitalWrite(bat_led4, LOW);
    digitalWrite(bat_led5, LOW);
  }

  if (voltage > battery_min_voltage + 2.7 and voltage < battery_min_voltage + 3.15){ //battery level 4
    digitalWrite(bat_led1, HIGH);
    digitalWrite(bat_led2, HIGH);
    digitalWrite(bat_led3, HIGH);
    digitalWrite(bat_led4, HIGH);
    digitalWrite(bat_led5, LOW);
  }

  if (voltage > battery_min_voltage + 3.2 ) {                                          //battery level 5
    digitalWrite(bat_led1, HIGH);
    digitalWrite(bat_led2, HIGH);
    digitalWrite(bat_led3, HIGH);
    digitalWrite(bat_led4, HIGH);
    digitalWrite(bat_led5, HIGH);
  }

}

void buzzer_alarm(){

  //when the uv light is on buzzer sound beep---beep---beep---beep---
  /*
  if (uv_on == true){
    if (millis()%1000 < 50){
      tone(buzz_pin, NOTE_C7);
      delay(5);
    }
    if (millis()%500 < 50){
      noTone(buzz_pin);
      digitalWrite(buzz_pin,HIGH);
    }
  }

  //when moving the robot buzzer sound bip------bip------bip------bip------
  else{
    if (millis()%1000 < 50){
      tone(buzz_pin, NOTE_C7);
      delay(5);
    }
    if (millis()%200 < 50){
      noTone(buzz_pin);
      digitalWrite(buzz_pin,HIGH);
    }
  }
*/
}

//blink bar led level 1 and buzzer beep beep
void low_battery_notification(){

    digitalWrite(bat_led2, LOW);
    digitalWrite(bat_led3, LOW);
    digitalWrite(bat_led4, LOW);
    digitalWrite(bat_led5, LOW);
    
  digitalWrite(bat_led1, HIGH);
  tone(buzz_pin,NOTE_A6);
  delay(200);

  digitalWrite(bat_led1, LOW);
  noTone(buzz_pin);
  digitalWrite(buzz_pin,HIGH);
  delay(50);

  digitalWrite(bat_led1, HIGH);
  tone(buzz_pin,NOTE_A6);
  delay(300);

  digitalWrite(bat_led1, LOW);
  noTone(buzz_pin);
  digitalWrite(buzz_pin,HIGH);
  delay(450);

}


//play this at powering up the robot
void startup_tone() {
  tone(buzz_pin, NOTE_C5);
  delay(200);
  tone(buzz_pin, NOTE_E5);
  delay(200);
  tone(buzz_pin, NOTE_G5);
  delay(200);
  tone(buzz_pin, NOTE_C6);
  delay(200);
  noTone(buzz_pin);
  digitalWrite(buzz_pin, HIGH);
}


//play this upon success of initial system checkup at start
void system_okay_melody(){

  tone(buzz_pin, NOTE_C5);
  delay(600);
  tone(buzz_pin, NOTE_C6);
  delay(300);



  noTone(buzz_pin);
  digitalWrite(buzz_pin, HIGH); 
}

//Play this tone when the uv switch of the ground controller is on
//when turning on the robot 
void uv_on_warning_at_startup(){

  tone(buzz_pin, NOTE_G6);
  delay(150);

  noTone(buzz_pin);
  digitalWrite(buzz_pin, HIGH);

   delay(500);

}
