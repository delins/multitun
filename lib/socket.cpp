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
#include <thread>

#include "queue.h"
#include "socket.h"
#include "util.h"



std::string sockaddr2IP(const struct sockaddr *sa) {
	char addr[INET6_ADDRSTRLEN];
	std::string s;
	switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    addr, INET6_ADDRSTRLEN);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    addr, INET6_ADDRSTRLEN);
            break;
        default:
            s = "Unknown AF";
    }
    s = addr;
    return s;
}


int sockaddr2Port(const struct sockaddr *sa) {
	switch (sa->sa_family) {
		case AF_INET:
			return static_cast<int>(ntohs(((struct sockaddr_in *)sa)->sin_port));
		case AF_INET6:
			return static_cast<int>(ntohs(((struct sockaddr_in6 *)sa)->sin6_port));
	}
}


Socket::Socket(	const SocketDescription &des, int sock_fd, std::shared_ptr<IDeMux> idemux_ptr, 
	int sock_type_c, int debug) 
	  : type(des.type), ip(des.ip), port(des.port), sock_fd(sock_fd), 
		idemux_ptr(idemux_ptr), sock_type_c(sock_type_c), debug(debug) {}		


Socket::Socket(const SocketDescription &des, std::shared_ptr<IDeMux> idemux_ptr, 
	int sock_type_c, int debug) 
	  : type(des.type), ip(des.ip), port(des.port), 
	  	idemux_ptr(idemux_ptr), sock_type_c(sock_type_c), debug(debug) {

	// Create socket
	int status;
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints);	// Empty struct
	hints.ai_family = AF_UNSPEC;		// Don't care about IPv4 or IPv6
	hints.ai_socktype = sock_type_c; 	// TCP stream sockets

	// Make servinfo point to a linked list of 1 or more struct addrinfo's.
	status = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &servinfo);
	if (status != 0) {
		//do_free_addrinfo = false; // Don't call freeaddrinfo in destructor!
		throw SocketException(std::string("getaddrinfo error: ") + gai_strerror(status));
	}

	// Create socket
	sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sock_fd == -1) {
		throw SocketException(std::string("Socket error: ") + strerror(errno));
	}

	if (debug >= 2) {debugOut(2,
	"created " + describeFull()
	);}
}


Socket::~Socket() {
	if (debug >= 2) {debugOut(2,
	"destructing " + describeFull()
	);}	

	freeaddrinfo(servinfo);
	close(sock_fd);
}


void Socket::connectSocket() {
	int status;

	status = connect(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (status ==-1) {
		throw SocketException(std::string("connect error: ") + strerror(errno));
	}

	if (debug >= 1) {debugOut(1,
	"connected to " + ip + ":" + std::to_string(port)
	);}
}


std::string Socket::describe() {
	std::string s(socketType2String(type) + ":" + ip + ":" + std::to_string(port));
	return s;
}

std::string Socket::describeFull() {
	std::string s(std::string("socket (fd=") + std::to_string(sock_fd) + ", des=" + describe() +")");
	return s;
}


ClientUDPSocket::ClientUDPSocket(const SocketDescription &des, 
	std::shared_ptr<IDeMux> idemux_ptr, int debug) 
	  : Socket(des, idemux_ptr, SOCK_DGRAM, debug) {}


void ClientUDPSocket::startReceiving() {
	// Good reads:
	// http://www.microhowto.info/howto/listen_for_and_receive_udp_datagrams_in_c.html
	// http://serverfault.com/questions/534063/can-tcp-and-udp-packets-be-split-into-pieces
	int n_read;
	while (true) {
		Message msg;
		// Datagram, so we don't need to look at the message's size. A single recv() call
		// should under normal circumstances give us a full datagram. It may be truncated
		// because the buffer we provide is too smal (is detectable) or because of some
		// system signal interrupting. In any case, the output always starts at the beginning
		// of a datagram.
		n_read = recv(sock_fd, msg.buffer, Message::BUF_SIZE, 0);
		
		msg.parseHeader();	// Read the header and put them in the message's fields.

		if (debug >= 2 && msg.payload_length+3 > n_read) {debugOut(2,
		std::string("msg states payload=" + std::to_string(msg.payload_length)) + "bytes, but couldn't read it all"
		);}

		if (debug >= 3) {debugOut(3,
		std::string("read " + std::to_string(msg.payload_length + 3) + " bytes from " + describeFull())
		);}

		idemux_ptr->handleMessage(msg);	
	}
}


void ClientUDPSocket::sendMessage(Message &message) {
	int n_written;
	int left = Message::HEADER_LENGTH + message.payload_length;
	char *buf = message.buffer;

	// write might return less than the number we told it to send.
	// In that case, try again.
	while (left > 0) {
		n_written = send(sock_fd, buf, left, 0);
		if (n_written < 0) {
			throw SocketException(std::string("Write error: ") + strerror(errno));
		} else {
			left -= n_written;
			buf += n_written;
		}
	}
	if (debug >= 3) {debugOut(3,
	std::string("wrote ") + std::to_string(n_written) + " bytes into " + describeFull()
	);};
}


ServerUDPSocket::ServerUDPSocket(const SocketDescription &des, 
	std::shared_ptr<IDeMux> idemux_ptr, int debug) 
	  : Socket(des, idemux_ptr, SOCK_DGRAM, debug) {

	int status;

	// Bind to port
	status = bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (status == -1) {
		throw SocketException(std::string("bind error: ") + strerror(errno));
	}

	if (debug >= 1) {debugOut(1, 
	std::string("bound to ") + sockaddr2IP(servinfo->ai_addr) + ":" + std::to_string(port)
	);}

}


void ServerUDPSocket::startReceiving() {
	// Good reads:
	// http://www.microhowto.info/howto/listen_for_and_receive_udp_datagrams_in_c.html
	// http://serverfault.com/questions/534063/can-tcp-and-udp-packets-be-split-into-pieces
	int n_read;
	while (true) {
		Message msg;
		// Datagram, so we don't need to look at the message's size. A single recv() call
		// should under normal circumstances give us a full datagram. It may be truncated
		// because the buffer we provide is too smal (is detectable) or because of some
		// system signal interrupting. In any case, the output always starts at the beginning
		// of a datagram. The last thing is also the reason we don't loop to "read it all".
		struct sockaddr_storage addr;
		n_read = recvfrom(sock_fd, msg.buffer, Message::BUF_SIZE, 0, 
			(struct sockaddr*)&addr, &peer_addr_length);
		if (!knows_peer) {
			peer_addr = addr;
			/*
			switch (addr.ss_family) {
				case AF_INET:
					peer_addr_length = sizeof sockaddr_in;
					break;
				case AF_INET6:
					peer_addr_length = sizeof sockaddr_in6;
					break;
			}*/
			knows_peer = true;
		}

		if (n_read < Message::HEADER_LENGTH) {
			// UDP is allowed to send 0 bytes. In general, if we got less than 
			// Message::HEADER_LENGTH, don't bother.
			continue;
		}

		msg.parseHeader();	// Read the header and put them in the message's fields.

		if (debug >= 2 && msg.payload_length+3 > n_read) {debugOut(2,
		std::string("msg states payload=" + std::to_string(msg.payload_length)) + "bytes, but couldn't read it all"
		);}

		if (debug >= 3) {debugOut(3,
		std::string("read " + std::to_string(msg.payload_length + 3) + " bytes from " + describeFull())
		);}

		idemux_ptr->handleMessage(msg);	
	}
}


void ServerUDPSocket::sendMessage(Message &message) {
	int n_written;
	int left = Message::HEADER_LENGTH + message.payload_length;
	char *buf = message.buffer;

	// write might return less than the number we told it to send.
	// In that case, try again.
	while (left > 0) {
		n_written = sendto(sock_fd, buf, left, 0, (sockaddr*)&peer_addr, peer_addr_length);
		if (n_written < 0) {
			throw SocketException(std::string("Write error: ") + strerror(errno));
		} else {
			left -= n_written;
			buf += n_written;
		}
	}
	if (debug >= 3) {debugOut(3,
	std::string("wrote ") + std::to_string(n_written) + " bytes into " + describeFull()
	);};
}


TCPSocket::TCPSocket(const SocketDescription &des, int sock_fd, 
	std::shared_ptr<IDeMux> idemux_ptr, int debug) 
	  : Socket(des, sock_fd, idemux_ptr, SOCK_STREAM, debug) {}		


TCPSocket::TCPSocket(const SocketDescription &des, 
	std::shared_ptr<IDeMux> idemux_ptr, int debug) 
	  : Socket(des, idemux_ptr, SOCK_STREAM, debug) {}


void TCPSocket::startReceiving() {
	int n_read;
	while (true) {
		Message msg;
		// First read the "header'" which states how many bytes follow.
		n_read = readAll(msg.buffer, Message::HEADER_LENGTH);
		msg.parseHeader();

		n_read = readAll(msg.payload, msg.payload_length);

		if (debug >= 3) {debugOut(3,
		std::string("read " + std::to_string(msg.payload_length + 3) + " bytes from " + describeFull())
		);}

		idemux_ptr->handleMessage(msg);	
		//nWrite = writeAll(tun_fd, buffer, n_read);

		if (debug) {
		//	debugOut(std::string("Wrote ") + std::to_string(n_read) + "bytes into " + if_name);
		}
	}
}


void TCPSocket::sendMessage(Message &message) {
	int n_written;
	int left = Message::HEADER_LENGTH + message.payload_length;
	char *buf = message.buffer;

	// write might return less than the number we told it to send.
	// In that case, try again.
	while (left > 0) {
		n_written = write(sock_fd, buf, left);
		if (n_written < 0) {
			throw SocketException(std::string("Write error: ") + strerror(errno));
		} else {
			left -= n_written;
			buf += n_written;
		}
	}
	if (debug >= 3) {debugOut(3,
	std::string("wrote ") + std::to_string(n_written) + " bytes into " + describeFull()
	);};
}


int TCPSocket::readAll(char *buf, int n) {
	int n_read, left = n;

	// read might return less than the number we told it to send.
	// In that case, try again.
	while (left > 0) {
		n_read = read(sock_fd, buf, left);
		if (n_read == 0) {
			throw SocketException(std::string("Socket was closed"));
		} else if (n_read == -1) {
			throw SocketException(std::string("Read error: ") + strerror(errno));
		} else {
			left -= n_read;	// Read nRead bytes, so -nRead bytes still to go
			buf += n_read;	// Increase buf pointer to next location in buffer
		}
	}
	return n;
}


ServerTCPSocket::ServerTCPSocket(const SocketDescription &des, 
	std::shared_ptr<IDeMux> idemux_ptr, int debug) : 
	type(des.type), ip(des.ip), port(des.port), idemux_ptr(idemux_ptr), debug(debug) {
	
	int status;
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints);	// Empty struct
	hints.ai_family = AF_UNSPEC;		// Don't care about IPv4 or IPv6
	int sock_type;
	switch (type) {
		case SocketType::UDP:
			sock_type = SOCK_DGRAM;
			break;
		case SocketType::TCP:
			sock_type = SOCK_STREAM;
			break;
	}
	hints.ai_socktype = sock_type; 	// TCP stream sockets
	struct addrinfo *servinfo;

	// Make servinfo point to a linked list of 1 or more struct addrinfo's.
	status = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &servinfo);
	if (status != 0) {
		//do_free_addrinfo = false; // Don't call freeaddrinfo in destructor!
		throw SocketException(std::string("getaddrinfo error: ") + gai_strerror(status));
	}

	// Create socket
	// SOCK_NONBLOCK makes it explicit that the the socket shouldn't block, which is required
	// for accept(). the select() statement in the Server takes care of blocking.
	sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype | SOCK_NONBLOCK, servinfo->ai_protocol);
	if (sock_fd == -1) {
		throw SocketException(std::string("Socket error: ") + strerror(errno));
	}

	if (debug >= 2) {debugOut(2,
	"created " + describeFull()
	);}

	// Bind to port
	status = bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (status == -1) {
		throw SocketException(std::string("bind error: ") + strerror(errno));
	}

	if (debug >= 1) {debugOut(1, 
	std::string("bound to ") + sockaddr2IP(servinfo->ai_addr) + ":" + std::to_string(port)
	);}

	// Listen for incoming connection(s). The 1 denotes maximally one client can be queued.
	status = listen(sock_fd, 1);
	if (status == -1) {
		throw SocketException(std::string("listen error: ") + strerror(errno));
	}

	if (debug >= 1) {debugOut(1,
	std::string("started listening on ") + describeFull()
	);}

	freeaddrinfo(servinfo);
}


std::shared_ptr<Socket> ServerTCPSocket::acceptPeerConnection() {
	/**
	 * 	Takes the memory address of a dummy socket object and stuffs a new one in there.
	 */
	struct sockaddr client_addr;		// Keeps connected client's address info
	int connection_fd;
	
	socklen_t addr_size;
	addr_size = sizeof client_addr;

	connection_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &addr_size);

	if (connection_fd == -1) {
		throw SocketException(std::string("accept error: ") + strerror(errno));
	}

	if (debug >= 1) {debugOut(1,
	std::string("got incoming connnection from ") + sockaddr2IP(&client_addr) + std::to_string(sockaddr2Port(&client_addr))
	);}

	SocketDescription des = {
		.type=type, 
		.ip=sockaddr2IP(&client_addr), 
		.port=sockaddr2Port(&client_addr)};

	std::shared_ptr<Socket> socket_ptr(
	new TCPSocket(des, connection_fd, idemux_ptr, debug));
	return socket_ptr;
}


std::string ServerTCPSocket::describe() {
	std::string s(socketType2String(type) + ":" + ip + ":" + std::to_string(port));
	return s;
}

std::string ServerTCPSocket::describeFull() {
	std::string s(std::string("server socket (fd=") + std::to_string(sock_fd) + ", des=" + describe() +")");
	return s;
}
