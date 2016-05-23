#ifndef SOCKET_H
#define SOCKET_H

#include <memory>
#include <stdexcept>
#include <string>

#include "idemux.h"
#include "queue.h"

// http://stackoverflow.com/questions/28828957/enum-to-string-in-modern-c-and-future-c17
enum class SocketType {TCP, UDP};


static SocketType string2SocketType(std::string s) {
	if (s == "TCP") {
		return SocketType::TCP;
	} else if (s == "UDP") {
		return SocketType::UDP;
	} else {
		throw std::invalid_argument(s);
	}
}


static std::string socketType2String(SocketType type) {
	switch (type) {
		case SocketType::TCP:
			return "TCP";
		case SocketType::UDP:
			return "UDP";
	}
}


struct SocketDescription {
	SocketType type;
	std::string ip;
	int port;
};




class Socket {
public:
	Socket(const SocketDescription &des, int sock_fd, std::shared_ptr<IDeMux> idemux_ptr,
		int sock_type_c, int debug=0);
	Socket(const SocketDescription &des, std::shared_ptr<IDeMux> idemux_ptr, 
		int sock_type_c, int debug=0);
	virtual ~Socket();
	void connectSocket();
	std::string describe();
	std::string describeFull();
	virtual void startReceiving()=0;
	virtual void sendMessage(Message &message)=0;
	virtual bool isReady()=0;
protected:
	int sock_type_c = 0;	// overridden by ctor in subclasses
	SocketType type;
	std::string ip;
	int port;
	int debug;
	int sock_fd;
	std::shared_ptr<IDeMux> idemux_ptr;

	struct addrinfo *servinfo; // Freed in destructor
};


class ClientUDPSocket : public Socket {
public:
	ClientUDPSocket(const SocketDescription &des, std::shared_ptr<IDeMux> idemux_ptr, 
		int debug=0);
	virtual ~ClientUDPSocket() = default;
	void startReceiving();
	void sendMessage(Message &message);
	bool isReady() {return true;};
};


class ServerUDPSocket : public Socket {
public:
	ServerUDPSocket(const SocketDescription &des, std::shared_ptr<IDeMux> idemux_ptr, 
		int debug=0);
	void startReceiving();
	void sendMessage(Message &message);
	bool isReady() {return knows_peer;};
private:
	bool knows_peer = false;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_length = sizeof peer_addr;
};


class TCPSocket : public Socket {
public:
	TCPSocket(const SocketDescription &des, int sock_fd, std::shared_ptr<IDeMux> idemux_ptr,
		int debug=0);
	TCPSocket(const SocketDescription &des, std::shared_ptr<IDeMux> idemux_ptr, int debug=0);
	virtual ~TCPSocket() = default;
	void startReceiving();
	void sendMessage(Message &message);
	bool isReady() {return true;};
private:
	int readAll(char* buf, int n);
};	




/*
class Socket {
public:
	Socket(
		const SocketDescription &des, 
		int sock_fd, 
		std::shared_ptr<IDeMux> idemux_ptr,
		int debug=0);
	Socket(
		const SocketDescription &description, 
		std::shared_ptr<IDeMux> idemux_ptr,
		int debug=0);
	virtual ~Socket();
	void startReceiving();
	std::string describe();
	std::string describeFull();
	int getFD() {return sock_fd;};
	void sendMessage(Message &message);
protected:
	int writeAll(int fd, char *buf, int n);
	int readAll(char* buf, int n);
	SocketType type;
	std::string ip;
	int port;
	int debug;

	struct addrinfo *servinfo; // Freed in destructor
	bool do_free_addrinfo = true;
	int sock_fd;
	std::shared_ptr<IDeMux> idemux_ptr;
};


class ClientSocket : public Socket {
public:
	ClientSocket(
		const SocketDescription &description, 
		std::shared_ptr<IDeMux> idemux_ptr,
		int debug=0);
	void connectSocket();
};
*/





class ServerTCPSocket {
public:
	ServerTCPSocket(
		const SocketDescription &des,
		std::shared_ptr<IDeMux> idemux_ptr,
		int debug=0);
	std::shared_ptr<Socket> acceptPeerConnection();
	std::string describe();
	std::string describeFull();
	int getFD() {return sock_fd;};
private:
	SocketType type;
	std::string ip;
	int port;
	int debug;
	int sock_fd;
	std::shared_ptr<IDeMux> idemux_ptr;
};





/*
class ServerSocket : public Socket {
public:
	ServerSocket(
		const SocketDescription &description, 
		std::shared_ptr<IDeMux> idemux_ptr,
		int debug=0);
	void listenSocket();
	std::shared_ptr<Socket> acceptPeerConnection();
};
*/

class SocketException : public std::exception {
private:
	std::string errorMsg;
public:
	SocketException(const std::string &msg) 
		: errorMsg(msg) {}
	~SocketException() throw() {};
	virtual const char* what() const throw() {
		return errorMsg.c_str();
	}

};


#endif