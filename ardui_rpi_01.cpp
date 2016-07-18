/*
 *TMRh20's library, RF24 uses maniacbug's library for RPi
 */

/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class on RPi, communicating to an Arduino running
 * the GettingStarted sketch.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <time.h>
#include <RF24/RF24.h>

using namespace std;
//
// Hardware configuration
// Configure the appropriate pins for your connections

/****************** Raspberry Pi ***********************/

// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939 and the related enumerations for pin information.

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ);

// NEW: Setup for RPi B+
//RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// Setup for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

// RPi generic:
RF24 radio(25,0,8000000); //!!!ATTN!!!: radio(<CE_GPIO#>, <CSN#>), not radio(<CE_pin#>, <CSN_pin#>)

/********** User Config *********/
// Assign a unique identifier for this node, 0 or 1
bool role_ping_out = true, role_pong_back = false;
bool radioNumber = role_pong_back;

/********************************/

// Radio pipe addresses for the 2 nodes to communicate.
const uint8_t pipes[][6] = {"1Node","2Node"};

// Used to control whether this node is sending or receiving
bool role = false;

void setup() {
  cout << "RF24/examples/GettingStarted/\n";

  //Setup
  radio.begin(); //setup and configure rf radio
  radio.setRetries(15,15); //optionally, increase the delay between retries & # of retries
  radio.setPALevel(RF24_PA_LOW); //set power level MIN, LOW, HIGH, MAX
  radio.setDataRate(RF24_2MBPS); //set transmission speed
  radio.printDetails(); //dump the configuration of the rf unit for debugging

  //Role Chooser
  cout << "\n *** Role Setup ***\n";
  cout << "Choose a role: Enter 0 for pong_back, 1 for ping_out (CTRL+C to exit) \n>";
  char input = 0;
  while (1) {
    cin >> input;
    if (input == '0') {
      cout << "\nRole: Pong Back, awaiting transmission \n";
      radioNumber = 0;
      role = role_pong_back;
      break;
    } else if (input == '1') {
      cout << "\nRole: Ping Out, starting transmission \n";
      radioNumber = 1;
      role = role_ping_out;
      break;
    } else {
      cout << "\nERROR: Improper input\n";
      cout << "Choose a role: Enter 0 for pong_back, 1 for ping_out (CTRL+C to exit) \n>";
      delay(100);
    } //if-else
  } //while
  
  /***********************************/
  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.

  if ( !radioNumber )    {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
  } else {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }
	
  radio.startListening();
} //setup

void loop() {
  if (role == role_ping_out) {

    //Ping out
    radio.stopListening(); //First, stop listening so we can talk
    unsigned long start_time = clock(); // Take the time, and send it. This will block until complete
    if (!radio.write( &start_time, sizeof(unsigned long))) {
      printf("failed.\n");
    } //if

    //Listen for Pong back
    radio.startListening(); //Now, continue listening
    unsigned long started_waiting_at = clock(); //Set up a timeout period, get the current microseconds
    bool timeout = false; // Set up a variable to indicate if a response was received or not
    while ( !radio.available() ) { //while nothing is received
      if (clock() - started_waiting_at > 200000) { //If waited longer than 200mx, indicate timeout and exit while loop
	timeout = true;
	break;
      } //if
    } //while

    //Describe the results
    if (timeout) {
      printf("Failed, response timed out.\n");
    } else {
      unsigned long got_time; // Grab the response, compare, and send to debugging spew
      radio.read( &got_time, sizeof(unsigned long) );
      unsigned long end_time = clock();

      //  Spew it
      printf("Sent %lu, Got response %lu, Round-trip delay %lu microseconds\n",start_time, got_time, end_time-start_time);
    } //if-else

    //Try again later
    delay(1000);
  } //if

  //Pong back

  if ( role == role_pong_back ) {
    unsigned long got_time; //Variable for the received timestamp
    
    if ( radio.available() ) { //if there is data ready
      while(radio.available()){ //while there is data ready
	radio.read( &got_time, sizeof(unsigned long) ); //get the payload
      } //while
      
      radio.stopListening(); //First, stop listening so we can talk				
      radio.write( &got_time, sizeof(unsigned long) ); //Send the final one back
      radio.startListening(); //Now, resume listening so we catch the next packets

      // Spew it
      printf("Sent response %lu\n", got_time);
				
      delay(925); //Delay after payload responded to, minimize RPi CPU time				
    } //if
  } //if

} //loop

int main(int argc, char** argv){
  setup();
  while (1) loop();

  return 0;
} //main

