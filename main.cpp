/**
 * main.cpp
 *
 * OSC-Ethernet Example Code
 *
 * Written for and tested on the STM32 Nucleo F767ZI
 *
 * To communicate with the MPR Lab Multi Solenoid Moddule
 *     version 12.6.2 or later
 *
 *
 * @author  Michael Sidler
 * @since   11/20/2018
 * @version 1
 *
 * Changelog:
 * - 1.0 Initial commit
 */

#include "mbed.h"
#include "EthernetInterface.h"
#include "osc_client.h"
#include <string>

#define VERBOSE 1

char* instrumentName;

//Setup digital outputs
DigitalOut led1(LED1);	//Green running LED
DigitalOut led2(LED2);	//Blue controller LED
DigitalOut led3(LED3);	//Red ethernet LED

/**
 * Main function
 */
int main() {

	//set the instrument name
	char name[] = "OSCInstrument";
	instrumentName = name;

	printf("Starting Instrunemt\r\n");

	//Turn on all LEDs
	led1 = 1;
	led2 = 2;
	led3 = 3;

	//Setup ethernet interface
	EthernetInterface eth;
	eth.connect();
	if(VERBOSE) printf("Client IP Address:     %s\r\n", eth.get_ip_address());
	led3 = 0;	//turn off red LED

	//Setup OSC client
	OSCClient osc(&eth, instrumentName);
	osc.connect();
	if(VERBOSE) printf("OSC controller IP Address: %s\r\n", osc.get_controller_ip());
	led2 = 0;	//turn off blue LED
	
	//Create a pointer to an OSC Message
	OSCMessage* msg = (OSCMessage*) malloc(sizeof(OSCMessage));

	//Variable to store the OSC message size
	unsigned int size;
	
	//Blocking Example
    while(true) {
        
        //get a new message and the size of the message
		osc.waitForMessage(msg);

		//Check that the message is for this instrument
		if(strcmp(osc.getInstrumentName(msg), instrumentName) == 0) {

			//printf("Message for this instrument\r\n");

			//Process the message based on type
			char* messageType = osc.getMessageType(msg);

			 // Play a specific note
			if(strcmp(messageType, "play") == 0) {

				//printf("Is a play message\r\n");

				// Check that the messages is the right format
				if(strcmp(msg->format, ",ii") == 0) { // two ints
					
					uint32_t pitch 	  = osc.getIntAtIndex(msg, 0);
					uint32_t velocity = osc.getIntAtIndex(msg, 1);

					printf("Pitch %d, velocity %d\r\n",pitch,velocity);

					//TODO: play the note
					led2 = 1;
					wait(0.1);
					led2 = 0;
				}
			}
			// Turn off all notes
			else if(strcmp(messageType, "allNotesOff") == 0) {
				//doesn't matter what the data is, turn all notes off

				//TODO: All notes off

			}
		}
		else {
			//Not intended for this instrument
		}

		printf("\r\n");
    }
}
