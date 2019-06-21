#include "osc_client.h"

////////////////////////////////////////////////

/**
 * Calculate the OSC size (next multiple of 4 to the length) of a string
 *
 * @param str	The string
 *
 * @return The padded length to be used for this string in OSC handling
 */
int OSCClient::OSC_SIZE(char* str) {
	int len = strlen(str) + 1;
	if(len % 4 != 0) len += 4 - (len % 4);
	return len;
}

uint32_t OSCClient::swap_endian(uint32_t number) {
	uint32_t byte0, byte1, byte2, byte3;
	byte0 = (number & 0x000000FF) >> 0;
	byte1 = (number & 0x0000FF00) >> 8;
	byte2 = (number & 0x00FF0000) >> 16;
	byte3 = (number & 0xFF000000) >> 24;
	return ((byte0<<24) | (byte1 << 16) | (byte2 << 8) | (byte3 << 0));
}

osStatus OSCClient::freeMessage(OSCMessage* m) {
	return mpool.free(m);
}

void OSCClient::messageRecieverFunction() {

	while(1) {
		// allocate memory for a message
		OSCMessage* msg = mpool.alloc();
		// wait for a message
		this->waitForMessage(msg);
		// add the message to the queue
		messageQueue.put(msg);
	}
}

void OSCClient::initBuffer() {
	messageRecieverThread.start(callback(threadStarter, this));
	messageRecieverThread.set_priority(osPriorityNormal);
}

void OSCClient::threadStarter(const void* p) {
	OSCClient* instancePtr = static_cast<OSCClient*>(const_cast<void*>(p));
	instancePtr->messageRecieverFunction();
}

uint8_t OSCClient::getMessageFromQueue(OSCMessage** m) {
	
	if(!messageQueue.empty()) {
		//printf("trying to get a message\r\n");
		osEvent evt = messageQueue.get();
		if (evt.status == osEventMessage) {
    		*m = (OSCMessage*)evt.value.p;
			//printf("got a message\r\n");
			return 1;
		}
	}
	return 0;
}

/**
 * Create a new OSCMessage with the given values
 *
 * @param address	The dispatch address
 * @param format	The format of the data
 * @param ...		Variadic parameters to be stored in the data array; informed by "format"
 */
OSCMessage* OSCClient::build_osc_message(char* address, char* format, ...) {
	OSCMessage* msg = (OSCMessage*) malloc(sizeof(OSCMessage));

	// Copy in the strings that can be placed immediately
	strcpy(msg->address, address);
	strcpy(msg->format, format);

	// Create two variadic parameters arrays, one for calculating size and the other to populate
	va_list args; va_start(args, format);

	// Populate the data buffer with the variadic arguments
	int i = 0, j = 1; unsigned int k = 0;
	uint32_t int_container; float float_container; char* string_container;

	while(format[j] != '\0') {
		switch(format[j++]) {
		case 'i':
			int_container = (uint32_t) va_arg(args, int);
			memcpy(msg->data + i, &int_container, sizeof(uint32_t));
			i += sizeof(uint32_t);
			break;

		case 'f':
			float_container = (float) va_arg(args, double);
			memcpy(msg->data + i, &float_container, sizeof(float));
			i += sizeof(float);
			break;

		case 's':
			string_container = (char*) va_arg(args, void*);
			unsigned int m = strlen(string_container) + 1, n = OSC_SIZE(string_container);
			memcpy(msg->data + i, string_container, m);
			for(k = m; k < n; k++) {
				msg->data[i + k] = '\0';
			}
			i += n;
			break;
		}
	}
	msg->data_size = i;

	// Free the variadic arguments
	va_end(args);

	return msg;
}

/**
 * Flatten an OSCMessage into a buffer of bytes.
 * 
 * @param msg 		The message to flatten
 * @param len_ptr (out)	The length of the generated buffer is stored here. Always a multiple of 4.
 * 
 * @return A buffer filled with the flattened contents of the given OSCMessage.
 */
byte* OSCClient::flatten_osc_message(OSCMessage* msg, int* len_ptr) {
	// Calculate the length of the buffer to send.
	// Note the use of OSC_SIZE instead of strlen here - it's important!
	// The length is guaranteed to be a multiple of 4 because OSC_SIZE will always return a
	// multiple of 4, and data_size is guaranteed to be a multiple of 4.
	int padded_address_length = OSC_SIZE(msg->address), padded_format_length = OSC_SIZE(msg->format);
	int length = padded_address_length + padded_format_length + msg->data_size;

	// Allocate the buffer and a position pointer
	byte* stream = (byte*) malloc(length);

	// Need to zero out the buffer in case the address or format string need padding bytes
	for(int i = 0; i < length; i++) {
		stream[i] = 0;
	}
	byte* posn = stream;

	// Flatten the OSCMessage into the allocated stream buffer
	strcpy(posn, msg->address);
	posn += padded_address_length;
	strcpy(posn, msg->format);
	posn += padded_format_length;
	for(int i = 0; i < msg->data_size; i++) {
		*(posn++) = msg->data[i];
	}
	*len_ptr = length;
	return stream;
}

//////////////////////////////////////////////////

/**
 * Get the IP address of the controller
 *
 * @return The address of the controller
 */
const char* OSCClient::get_controller_ip() {
	return this->controller.get_ip_address();
}

/**
 * Send an OSC Message over UDP
 *
 * @param msg	The OSCMessage to send out
 *
 * @return The number of bytes sent, or an error code
 */
nsapi_size_or_error_t OSCClient::send(OSCMessage* msg) {
	int length = 0;
	byte* stream = flatten_osc_message(msg, &length);
	if(DEBUG) printf("Sending UDP data\n");
	// Send out the stream and then free it
	nsapi_size_or_error_t out = this->udp.sendto(this->controller, stream, length);
	free(stream);
	if(DEBUG) printf("Done sending UDP data\n");

	return out;
}

/**
 * Receive an OSC message over UDP
 *
 * @param msg	The OSCMessage to populate with incoming data
 *
 * @return The number of bytes received, or an error code
 */
nsapi_size_or_error_t OSCClient::receive(OSCMessage* msg) {
	char buff[OSC_MSG_SIZE]; char* buffer = (char*) buff;
	char* start = buffer;
	unsigned int offset;
	int padding;

	nsapi_size_or_error_t recv = this->udp.recvfrom(&controller, buffer, OSC_MSG_SIZE);
	if(recv <= 0) {
		return recv;
	}

	// Clear the struct
	memset(msg, 0, sizeof(OSCMessage));

	// Copy the buffer directly into address, which will capture everything up to the first null terminator
	int address_length = strlen(buffer) + 1;
	strcpy(msg->address, buffer);
	buffer = buffer + address_length;

	offset = buffer - start;
	padding = 4 - (offset % 4);
	if (offset % 4 != 0) buffer += padding;

	// After advancing to that point, the next string extracted by strcpy will be the type tag
	int format_length = strlen(buffer) + 1;
	strcpy(msg->format, buffer);
	buffer = buffer + format_length;

	offset = buffer - start;
	padding = 4 - (offset % 4);
	if (offset % 4 != 0) buffer += padding;

	// Blindly copy everything else up to the end of the received byte stream
	memcpy(msg->data, buffer, recv - address_length - format_length);
	msg->data_size = recv - address_length - format_length;
	return recv;
}

/** 
 * Checks to see if there is a valid OSC Message waiting
 *
 * Returns the message and the length if there is a message, or 0 if there is no message
 *
 * Non-blocking
 *
 * @param msg The OSCMessage to populate with incoming data
 *
 * @return The number of bytes recieved, or 0 for none
 */
nsapi_size_t OSCClient::checkForMessage(OSCMessage* msg) {
	
	nsapi_size_or_error_t size_or_error = receive(msg);

	//Check to see if the message is valid
	if(size_or_error == NSAPI_ERROR_WOULD_BLOCK) return 0;
	if(size_or_error <= 0) return 0;

	//Return the message size
	return size_or_error;
}

/**
 * Waits for a valid OSC Message
 *
 * Returns the message and the length if there is a message, or 0 if there is no message
 *
 * Blocking
 *
 * @param msg The OSCMessage to populate with incoming data
 *
 * @return The number of bytes recieved, or 0 for none
 */
nsapi_size_t OSCClient::waitForMessage(OSCMessage* msg) {

	nsapi_size_t size = 0;

	while(size == 0) {
		size = checkForMessage(msg);
	}

	return size;
}

/** 
 * Gets the name of the instrument that the message is intended for
 *
 * @param msg The OSC Message
 * 
 * @return pointer to the name
 */
char* OSCClient::getInstrumentName(OSCMessage* msg) {
	return strtok(msg->address, "/");
}

/** 
 * Gets the type of the message
 *
 * @param msg The OSC Message
 * 
 * @return pointer to the type
 */
char* OSCClient::getMessageType(OSCMessage* msg) {
	//char* token = strtok(msg->address, "/");
	return strtok(NULL, "/");
}

/**
 * Gets the format of the message
 *
 * @param msg The OSC Message
 * 
 * @return pointer to the format
 */
char* OSCClient::getMessageFormat(OSCMessage* msg) {
	return msg->format;
}

/**
 * Returns the data at the specified index cast to an int
 *
 * @param msg The OSC Message
 *
 * @return the data as an int
 */
uint32_t OSCClient::getIntAtIndex(OSCMessage* msg, int index) {
	
	uint32_t val;

	memcpy(&val, msg->data + (index * sizeof(uint32_t)), sizeof(uint32_t));

	return swap_endian(val);
}

/**
 * Returns the data at the specified index cast to an float
 *
 * @param msg The OSC Message
 *
 * @return the data as a float
 */
float OSCClient::getFloatAtIndex(OSCMessage* msg, int index){
	
	float val;

	memcpy(&val, msg->data + (index * sizeof(float)), sizeof(float));

	return swap_endian(val);
}

/**
 * Returns the data at the specified index cast to a string
 *
 * @param msg The OSC Message
 *
 * @return the data as a string
 */
char* OSCClient::getStringAtIndex(OSCMessage* msg, int index) {
	//TODO:
	return 0;
}

/** 
 * Register name and supported functions with the central controller 
 */
void OSCClient::connect() {
	// TODO: udp_broadcast socket can probably be a local variable in this function, 
	// instead of being a field of OSCClient

	printf("Starting OSC Connection\r\n");
	
	// For the setup phase, allow the socket to block
	this->udp_broadcast.set_blocking(true);

	// Create and send the OSC message for registering the name of the instrument
	char address[] = "/NoticeMe";
	char format[] = ",ss";
	OSCMessage* msg = build_osc_message(
			address,
			format,
			instrumentName,
			this->address.get_ip_address()
	);

	// Enable broadcasting for the socket	
	this->udp_broadcast.set_broadcast(true);
	SocketAddress broadcast(BROADCAST_IP, OSC_PORT);
	int length = 0;

	byte* msg_buf = flatten_osc_message(msg, &length);

	
	nsapi_size_or_error_t size_or_error = this->udp_broadcast.sendto(broadcast, msg_buf, length);
	if(size_or_error < 0) {
		if(DEBUG) printf("Error sending the registration message! (%d)\r\n", size_or_error);
		exit(1);
	}

	free(msg_buf);
	free(msg);

	// Reset broadcast and blocking for this socket
	this->udp_broadcast.set_broadcast(false);
	this->udp_broadcast.set_blocking(false);
	
	char* buffer = (char*) malloc(OSC_MSG_SIZE);

	// For the setup phase, allow the socket to block
	this->udp.set_blocking(true);
	
	// TODO: this section should keep trying to recieve messages until it gets the correct kind
	// of "acknowledgement" message (just in case)
	int status = this->udp.bind(OSC_PORT+1);
	if(DEBUG) printf("bind status: %d\r\n", status);

	// Get back a request for functions from the controller
	// TODO: use a new SocketAddress here instead of broadcast	
	size_or_error = this->udp.recvfrom(&broadcast, buffer, OSC_MSG_SIZE);	
	if(size_or_error < 0) {
		if(DEBUG) printf("Error finding controller! (%d)\r\n", size_or_error);
		exit(1);
	}

	if(DEBUG) printf("Controller ip addr:%s\r\n", broadcast.get_ip_address());
	// TODO: need to confirm that this moves the broadcast SocketAddress object into
	// this OSCClient object
	this->controller = broadcast;

	// For normal operation, socket should be polled
	this->udp.set_blocking(false);

	// FREEDOM!
	free(buffer);
}