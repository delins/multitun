#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <algorithm>

#include "tun.h"
#include "queue.h"
#include "util.h"

std::string ioctlErrorToString(int);


Tun::Tun(
	const std::string &name, 
	const std::string &clone_dev,
	int debug) : if_name(name), debug(debug) {
	
	if (if_name.size() > IFNAMSIZ) {
		throw std::invalid_argument(
			"Interface name longer than " + std::to_string(IFNAMSIZ) + " characters"
		);
	}

	struct ifreq ifr;
	int status;

	// Open clone device (default /dev/net/tun).
	if ((tun_fd = open(clone_dev.c_str(), O_RDWR)) < 0) {
		throw TunException(std::string("Tun init error when calling open() on clone device \"") + clone_dev + "\": " + strerror(errno));
	}	

	// Write zeroes to the ifreq struct.
	memset(&ifr, 0, sizeof(ifr)); 

	// Device should be type tun. OFF_NO_PI: TODO
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI | IFF_MULTI_QUEUE;

	// Copy interface name to ifreq struct. 
	// If if_name is empty, this is a nullbyte which tells the kernel to make up a name.
	strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ);

	// Create, or associate with, the tun interface.
	if ((status = ioctl(tun_fd, TUNSETIFF, (void *) &ifr)) < 0) {
		throw TunException(std::string("Tun init error when calling ioctl(): ") + strerror(errno));
	}

	// Set if_name. Only changes something if the kernel was in charge of setting the if name.
	if_name = ifr.ifr_name;

	if (debug >= 2) {debugOut(2,
	"created tun device with name \"" + if_name + "\"."
	);}
}

int Tun::sendSimple(const std::string &s) {
	char buffer[2000];
	memcpy(buffer, s.c_str(), s.size() + 1);
	std::cout << strlen(buffer) << std::endl;
	writeAll(buffer, strlen(buffer));
}

int Tun::sendSimple(char buffer[], int count) {
	std::cout << "writing " << count << " bytes " << std::endl;
	writeAll(buffer, count);
}

int Tun::readBytes(int fd, char *buf, int n) {
	int nRead;
	if ((nRead=read(fd, buf, n)) < 0) {
		throw TunException(std::string("Socket was closed")); 
	}
	return nRead;
}


int Tun::readAll(int fd, char *buf, int n) {
	int nRead, left = n;

	// read might return less than the number we told it to send.
	// In that case, try again.
	while (left > 0) {
		nRead = read(fd, buf, left);
		if (nRead == 0) {
			throw TunException(std::string("Socket was closed"));
		} else if (nRead == -1) {
			throw TunException(std::string("Read error: ") + strerror(errno));
		} else {
			left -= nRead;	// Read nRead bytes, so -nRead bytes still to go
			buf += nRead;	// Increase buf pointer to next location in buffer
		}
	}
	return n;
}


int Tun::writeAll(char *buf, int n) {
	int nWritten, left = n;

	// write might return less than the number we told it to send.
	// In that case, try again.
	while (left > 0) {
		nWritten=write(tun_fd, buf, left);
		if (nWritten < 0) {
			throw TunException(std::string("Write error: ") + strerror(errno));
		} else {
			left -= nWritten;
			buf += nWritten;
		}
	}
	return n;
}


void Tun::writeMessage(Message &msg) {
	try {
		int n_written;
		n_written = writeAll(msg.payload, msg.payload_length);

		if (debug >= 3) {debugOut(3,
		std::string("wrote ") + std::to_string(n_written) + " bytes into " + describeFull()
		);}
	}
	catch (TunException &e) {	
		errorOut(e.what());
	}
}


std::string Tun::describeFull() {
	return std::string("tun (fd=") + std::to_string(tun_fd) + ", name=" + if_name + ")";
}


void Tun::receive(Message &msg) {
	/**
	 *	Continuously reads from the tun device and sends this payload off the the IMux.
	 *	It adds the 
	 */
	int n_read;
	msg.setType(Message::DATA);
	// msg.start_payload points to the location where the payload is supposed to be.
	n_read = read(tun_fd, msg.payload, msg.PAYLOAD_SIZE);

	if (n_read < 0) {
		throw TunException(std::string("tun error: ") + strerror(errno)); 
	}

	msg.setSize(n_read);

	if (debug >= 3) {debugOut(3,
	std::string("read ") + std::to_string(n_read) + " bytes from " + if_name
	);}
}


Tun::~Tun() {
	std::cout << "aaaaaaaaaaaaaaa" << std::endl;
	close(tun_fd);
};

