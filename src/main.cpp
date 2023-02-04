#include <Arduino.h>
#include <muddescapes.h>

// In this example project, you will make an LED turn on with one switch and turn off with another. 
// The LED can be turned on or off remotely
// The current state of the LED is sent to the control center

// Declare the functions that I want to be wifi-controlled
void solve_puzzle();
void reset_puzzle();

// Declare any variables I use in this program
bool solved_puzzle = false;
int solve_pin = 33;
int reset_pin = 27;
int led_output_pin = 21;

MuddEscapes &me = MuddEscapes::getInstance(); // Keep this line the same

// Put the functions that you want to be wifi-controlled here. 
// The string in the quotes is what the function is the human-readable name, and the thing after is what it's called in the code (declared above)
muddescapes_callback callbacks[]{{"Solve [your name]'s puzzle", solve_puzzle}, {"Reset [your name]'s puzzle", reset_puzzle}, {NULL, NULL}};

// Put the variables that you want to be monitored here. They MUST be booleans (true/false). 
// The string is the human-readable name, and the thing after is the variable name in this code (don't forget to put & before it)
muddescapes_variable variables[]{{"Current state of [your name]'s puzzle", &solved_puzzle}, {NULL, NULL}};

void solve_puzzle() {
  solved_puzzle = true;
}

void reset_puzzle() {
  solved_puzzle = false;
}

void setup() {
  // put your setup code here, to run once:
  delay(1000);

  pinMode(solve_pin,INPUT);
  pinMode(reset_pin,INPUT);
  pinMode(led_output_pin,OUTPUT);

  me.init("Claremont-ETC", "Cl@remontI0T", "mqtt://broker.hivemq.com", "[your name]", callbacks, variables);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(solve_pin) == HIGH) {
    solved_puzzle = true;
  }
  if (digitalRead(reset_pin) == HIGH) {
    solved_puzzle = false;
  }

  if (solved_puzzle) {
    digitalWrite(led_output_pin, HIGH);
  }
  else {
    digitalWrite(led_output_pin, LOW);
  }

  me.update();
}