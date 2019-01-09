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

#define VERBOSE 1

char* instrumentName;

//Setup digital outputs
DigitalOut led1(LED1);	//Green running LED
DigitalOut led2(LED2);	//Blue controller LED
DigitalOut led3(LED3);	//Red ethernet LED

static uint32_t swap_endian(uint32_t number){
	uint32_t byte0, byte1, byte2, byte3;
	byte0 = (number & 0x000000FF) >> 0;
	byte1 = (number & 0x0000FF00) >> 8;
	byte2 = (number & 0x00FF0000) >> 16;
	byte3 = (number & 0xFF000000) >> 24;
	return ((byte0<<24) | (byte1 << 16) | (byte2 << 8) | (byte3 << 0));
}

/**
 * Function that processes the OSC message recieved in main
 */
static void osc_dispatch(OSCMessage* msg) {
	
	//Check if message is addressed to Parthenope
	char* token = strtok(msg->address, "/");
	if(strcmp(token, instrumentName) != 0) {
		if(VERBOSE) printf("Message intended for '%s'\r\n", token);		
		return;
	}
	else {
		led2 = 1;

		printf("%s\r\n", msg->address);
		printf("%s\r\n", msg->format);
		
		// Get the desired function call and dispatch the data to it
		token = strtok(NULL, "/");
		if(strcmp(token, "play") == 0) {
			
			if(strcmp(msg->format, ",ii") != 0) {
				printf("Incorrect arguments (%s) for %s()\r\n", msg->format, token);
				return;
			}

			uint32_t pitch, velocity;

			memcpy(&pitch, msg->data, sizeof(uint32_t));
			memcpy(&velocity, msg->data + sizeof(uint32_t), sizeof(uint32_t));

			pitch = swap_endian(pitch);
			velocity = swap_endian(velocity);

			//TODO: play the note
			printf("Pitch: %lu Velocity: %lu\r\n\r\n",pitch,velocity);

		}
		else {
			printf("Unrecognized address %s\r\n", token);
			return;
		}

		led2 = 0;
		return;
	}
}

/**
 * Main function
 */
int main() {

	//set the instrument name
	char name[] = "OSCInstrument";
	instrumentName = name;

	printf("Starting Client\r\n");

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

	//Variable to store the OSC message size or error number
	nsapi_size_or_error_t size_or_error;
	
    while(true) {
        
        //get a new message and the size of the message/error
		size_or_error = osc.receive(msg);

		//check if the recieve function returned an error
		if(size_or_error == NSAPI_ERROR_WOULD_BLOCK) /*do nothing*/;

		//check if the size of the msg is <=0
		else if(size_or_error <= 0) {
			if(VERBOSE) printf("ERROR: %d\r\n", size_or_error);
		}
		
		//process the message
		else
			osc_dispatch(msg);
    }
}

