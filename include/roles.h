#ifndef ROLES_H
#define ROLES_H

#include <map>
#include <memory>
#include <thread>

#include "options.h"
#include "imux.h"
#include "idemux.h"
#include "socket.h"
#include "tun.h"


class Endpoint {
public:
	Endpoint(const Options &options);
	virtual ~Endpoint() = default;
protected:								// This order is imporant!
	std::shared_ptr<Tun> tun_ptr;		// first Tun
	std::shared_ptr<IMux> imux_ptr;		// IMux uses Tun
	std::shared_ptr<IDeMux> idemux_ptr;	// IDeMux uses tun as well
	std::map<std::string, std::thread> sockets_t;
	int debug;
};


class Client : public Endpoint {
public:
	/**
		Reads options and configures the sockets as requested. Doesn't connect yet.
	*/
	Client(const Options &options);
	virtual ~Client() = default;

	/**
		Walks through the sockets and connects them, spawning a new thread per socket
		to listen with. Then feeds the sockets into its imux and idemuxes.
	*/
	void start();
private:
	std::map<std::string, std::shared_ptr<Socket>> socket_ptrs;
	std::map<std::string, std::thread> threads;

};


class Server : public Endpoint {
public:
	/**
		Reads options and configures the sockets as requested. Doesn't connect yet.
	*/
	Server(const Options &options);
	virtual ~Server() = default;

	/**
		Walks through the sockets and connects them, spawning a new thread per socket
		to listen with. Then feeds the sockets into its imux and idemuxes.
	*/
	void start();
	void performListening();
private:
	std::map<std::string, std::unique_ptr<ServerTCPSocket>> listen_socket_ptrs;
	std::map<std::string, std::shared_ptr<Socket>> socket_ptrs;
	std::map<std::string, std::thread> threads;
};


#endif