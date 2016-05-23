#ifndef QUEUE_H
#define QUEUE_H

#include <inttypes.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <string.h>
#include <sys/types.h>
#include <vector>

struct Message {
	/*	Message format (in bytes):
	 *
	 *  	|   2    |  1   |  1..PAYLOAD_SIZE |
 	 * 		| plsize | type | payload ...      |
	 *
	 * 	plsize and type are always set, as soon as possible.
	 *	plsize denotes the payload size. 
	 *	Total message size is plsize + Message::HEADER_LENGTH
	 */
	static const uint16_t BUF_SIZE = 2000;
	static const uint16_t PAYLOAD_SIZE = 1997;
	static const uint16_t HEADER_LENGTH = 3;

	static const char DATA = '0';
	static const char CONTROL = '1';

	char buffer[BUF_SIZE];
	char *payload;
	uint16_t payload_length;
	char type;

	Message() : payload(buffer + 3) {}

	void setType(const char type) {
		this->type = type;
		//memcpy(buffer + 2, &type, 1);
		buffer[2] = type;
	}

	void setSize(uint16_t payload_size) {
		// Sets both the length attribute and writes it into the buffer
		payload_length = payload_size;
		uint16_t message_size = htons(payload_size);
		memcpy(buffer, (char*) &message_size, sizeof message_size);
	}

	void parseHeader() {
		// Parse plsize (payload length)
		uint16_t size;
		memcpy((char*) &size, buffer, 2);
		payload_length = ntohs(size);
		// Parse type
		type = buffer[2];
	}

	uint16_t getPayloadSize() {
		uint16_t size;
		memcpy((char*) &size, buffer, 2);
		return ntohs(size);
	}
};


/*
class Queue {
public:
	Queue();
	void enqueue(Message);
	Message &dequeue();
private:
	std::vector<Message> queue;
};
*/

#endif