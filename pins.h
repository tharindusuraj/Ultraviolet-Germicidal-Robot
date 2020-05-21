
#define uv_relay_pin 22 //connected to the UPS on/off switch to control uv lights
#define led_relay_pin 24 //connected to red and green led stripes NC - green NO-red
#define buzz_pin 30
#define system_led 32

#define bat_monitor_pin A11 //connected to the voltage devider

//bar graph led pins
#define bat_led1 44
#define bat_led2 46
#define bat_led3 48
#define bat_led4 50
#define bat_led5 52

//motor driver pins-------------------
#define m1a 4
#define m1b 3
#define m2a 5
#define m2b 6
#define m1en 11
#define m2en 10

int forward_speed = 150;
int turn_speed = 120;
int low_speed = 80;

int res_low = 338;     //resistor in voltage devider in negative side
int res_high= 805;     //resistor in voltage devider in positive side
 
