
/*
 * Getting Started example sketch for nRF24L01+ radios
 * This is a very basic example of how to send data from one node to another
 * Updated: Dec 2014 by TMRh20
 */

#include <SPI.h>
#include "RF24.h"
#include <stdio.h>

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 0;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
//CE, CSN
RF24 radio(7,10);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

// Used to control whether this node is sending or receiving
bool role_ping_out = true, role_pong_back = false;
bool role = role_pong_back;

void setup() {
  Serial.begin(115200);
  Serial.println(F("RF24/examples/GettingStarted"));
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  //Setup
  radio.begin(); //setup and configure rf radio
  radio.setRetries(15,15); //optionally, increase the delay between retries & # of retries
  radio.setPALevel(RF24_PA_LOW); //set power level MIN, LOW, HIGH, MAX
  radio.setDataRate(RF24_2MBPS); //set data rate
  Serial.print(F("Data Rate\t= "));
  Serial.print(radio.getDataRate());
  Serial.println("MPBS");
//  radio.printDetails(); //dump the configuration of the rf unit for 

  //Role Chooser
  Serial.print("\n *** Role Setup ***\n");
  Serial.print("Choose a role: Enter 0 for pong_back, 1 for ping_out \n>");
  char input = 0;
  while (1) {
    while (!Serial.available());
    input = Serial.read();
    if (input == '0') {
      Serial.println("\nRole: Pong Back, awaiting transmission \n");
      radioNumber = 0;
      role = role_pong_back;
      break;
    } else if (input == '1') {
      Serial.println("\nRole: Ping Out, starting transmission \n");
      radioNumber = 1;
      role = role_ping_out;
      break;
    } else {
      Serial.println("\nERROR: Improper input");
      Serial.print("Choose a role: Enter 0 for pong_back, 1 for ping_out \n>");
    } //if-else
  } //while
}

void loop() {
  //Open a writing and reading pipe on each radio, with opposite addresses
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  } //if-else
  
  radio.startListening();

  /****************** Ping Out Role ***************************/  
  if (role == role_ping_out)  {

    //Ping out
    radio.stopListening(); // First, stop listening so we can talk.
    unsigned long start_time = micros(); // Take the time, and send it.  This will block until complete
    if (!radio.write( &start_time, sizeof(unsigned long) )){
      Serial.println(F("failed"));
    } //if

    //Listen for Pong back
    radio.startListening(); // Now, continue listening
    unsigned long started_waiting_at = micros(); // Set up a timeout period, get the current microseconds
    bool timeout = false; // Set up a variable to indicate if a response was received or not   
    while ( !radio.available() ) { // While nothing is received
      if (micros() - started_waiting_at > 200000 ){ // If waited longer than 200ms, indicate timeout and exit while loop
  timeout = true;
  break;
      } //if
    } //while

    //Describe the results
    if ( timeout ){
      Serial.println(F("Failed, response timed out."));
    }else{
      unsigned long got_time; // Grab the response, compare, and send to debugging spew
      radio.read( &got_time, sizeof(unsigned long) );
      unsigned long end_time = micros();

      // Spew it
      Serial.print(F("Sent ")); Serial.print(start_time);
      Serial.print(F(", Got response ")); Serial.print(got_time);
      Serial.print(F(", Round-trip delay ")); Serial.print(end_time-start_time); Serial.println(F(" microseconds"));
    } //if-else

    // Try again 1s later
    delay(1000);
  }



  /****************** Pong Back Role ***************************/

  if ( role == role_pong_back ) {
    unsigned long got_time; //Variable for the received timestamp

    if( radio.available()){ //if there is data ready
      while (radio.available()) { // While there is data ready
  radio.read( &got_time, sizeof(unsigned long) ); // Get the payload
      } //while

      radio.stopListening(); // First, stop listening so we can talk   
      radio.write( &got_time, sizeof(unsigned long) ); // Send the final one back.      
      radio.startListening(); // Now, resume listening so we catch the next packets.     

      //Spew it
      Serial.print(F("Sent response "));
      Serial.println(got_time);  
    } //if
  } //if




  /****************** Change Roles via Serial Commands ***************************/

  if ( Serial.available() )
    {
      char c = toupper(Serial.read());
      if ( c == '1' && role == 0 ){      
  Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- ENTER '0' TO SWITCH BACK"));
  role = 1;                  // Become the primary transmitter (ping out)
  radioNumber = 1;

      }else
  if ( c == '0' && role == 1 ){
    Serial.println(F("*** CHANGING TO RECEIVE ROLE -- ENTER '1' TO SWITCH BACK"));      
    role = 0;                // Become the primary receiver (pong back)
    radioNumber = 0;
    radio.startListening();

  }
    }

} // Loop
