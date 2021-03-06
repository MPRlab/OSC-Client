/**
 * osc_client.h
 * Definition of a class for connecting to the central controller using OSC over UDP
 *
 * @author  Tanuj Sane, Andrew Walter, Michael Sidler
 * @since   4/20/2018
 * @version 1.1
 *
 * Changelog:
 * - 1.0 Initial commit
 * - 1.1
 * - 2.0 Overhaul that split the original .h file into 4 new files
 * - 2.1 Added new methods to make everything easier
 */

///////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "mbed.h"
#include "UDPSocket.h"
#include "UDPBroadcastSocket.h"
#include "SocketAddress.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////

#define BROADCAST_IP			"192.168.2.255"
#define OSC_PORT				8000
#define OSC_MSG_SIZE			256
#define QUEUE_SIZE				10

#define DEBUG (0)

///////////////////////////////////////////////////////////////////////////////////////////////////////

typedef char byte;

enum {
	OSC_SIZE_ADDRESS = 64,
	OSC_SIZE_FORMAT = 16,
	OSC_SIZE_DATA = 128
};

typedef struct {
	char address[OSC_SIZE_ADDRESS];			// The OSC address indicating where to dispatch
	char format[OSC_SIZE_FORMAT];			// Format specifier for the contained byte array
	byte data[OSC_SIZE_DATA]; 				// The byte array containing the data
	int data_size;							// The size of the data array
} OSCMessage;


class OSCClient {
	private:
		const char* instrumentName;				// The name of the instrument	
		SocketAddress controller;				// The address (IP, port) of the central controller
		SocketAddress address;					// The address (IP, port) of this instrument
		UDPBroadcastSocket udp_broadcast;		// The socket used to broadcast to the controller during discovery.
		UDPSocket udp;							// The socket used to communicate with the controller. 
		//Thread messageRecieverThread;			// Thread to handle recieving the messages
		//Queue<OSCMessage, QUEUE_SIZE> messageQueue;		// Queue for the messages to be put into
		//MemoryPool<OSCMessage, QUEUE_SIZE> mpool;
		static int OSC_SIZE(char* str);
		static uint32_t swap_endian(uint32_t number);
		static OSCMessage* build_osc_message(char* address, char* format, ...);
		static byte* flatten_osc_message(OSCMessage* msg, int* len_ptr);
		void messageRecieverFunction();
		
		
	public:
		template <typename S> OSCClient(S* stack, const char* name)
			:instrumentName(name), controller(BROADCAST_IP, OSC_PORT), 
			 address(stack->get_ip_address(), OSC_PORT), 
			 udp_broadcast(stack), udp() {
				 

			//udp();
			udp.open(stack);
			
		}
		nsapi_size_t checkForMessage(OSCMessage* msg);
		nsapi_size_t waitForMessage(OSCMessage* msg);
		//void initBuffer();
		//osStatus freeMessage(OSCMessage* m);
		//uint8_t getMessageFromQueue(OSCMessage** m);
		//static void threadStarter(void const* p);
		const char* get_controller_ip();
		nsapi_size_or_error_t send(OSCMessage* msg);
		nsapi_size_or_error_t receive(OSCMessage* msg);
		void connect();
		char* getInstrumentName(OSCMessage* msg);
		char* getMessageType(OSCMessage* msg);
		char* getMessageFormat(OSCMessage* msg);
		uint32_t getIntAtIndex(OSCMessage* msg, int index);
		float getFloatAtIndex(OSCMessage* msg, int index);
		char* getStringAtIndex(OSCMessage* msg, int index);

};
