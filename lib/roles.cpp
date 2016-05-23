#include <cstring>		// Required for strerror()
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "roles.h"
#include "socket.h"
#include "util.h"


Endpoint::Endpoint(const Options &options) : 
	tun_ptr(new Tun(options.if_name, options.clone_dev, options.debug_level)),
	imux_ptr(new IMux(tun_ptr, options.debug_level)),
	idemux_ptr(new IDeMux(tun_ptr, options.debug_level)),
	debug(options.debug_level) {}

Client::Client(const Options &options) : Endpoint(options) {
	if (debug >= 2) {debugOut(2,
	"setting up client..."
	);}


	// Create the sockets
	for (auto it=options.sock_des.begin(); it<options.sock_des.end(); it++) {
		// Uses unique pointers to avoid destructing the ServerTCPSocket on push_back
		// (freeaddrinfo doesn't play nicely when called twice).

		// UDP
		if (it->type == SocketType::UDP) {
			std::shared_ptr<Socket> sock_ptr(
				new ClientUDPSocket(*it, idemux_ptr, debug));	
			imux_ptr->attachSocket(sock_ptr);
			socket_ptrs.insert({sock_ptr->describe(), sock_ptr});

		// TCP
		} else { // TCP
			std::shared_ptr<Socket> sock_ptr(
				new TCPSocket(*it, idemux_ptr, debug));	
			imux_ptr->attachSocket(sock_ptr);
			socket_ptrs.insert({sock_ptr->describe(), sock_ptr});
		}
	}

	if (debug >= 1) {debugOut(1,
	"succesfully set up the client with " + std::to_string(socket_ptrs.size()) + " sockets"
	);}
}


void Client::start() {	
	// Start socket threads
	for (auto it=socket_ptrs.begin(); it!=socket_ptrs.end(); it++) {
		it->second->connectSocket();
		threads.emplace(
			it->second->describe(), 
			std::thread([it] () {it->second->startReceiving();})
		);
		if (debug >= 2) {debugOut(2,
		"started thread for socket ..."
		);}
	}

	// Start imux thread
	std::thread imux_t([this] () {this->imux_ptr->readTunLoop();});
	
	if (debug >= 2) {debugOut(2,
	"started the imux thread"
	);}

	// Join imux thread
	imux_t.join();

	// Join socket_ptr_threads
	for (auto it=threads.begin(); it!=threads.end(); it++) {
		it->second.join();
	}
}


Server::Server(const Options &options) : Endpoint(options) {
	if (debug >= 2) {debugOut(2,
	"setting up server..."
	);}

	// Create the listening sockets
	for (auto it=options.sock_des.begin(); it!=options.sock_des.end(); it++) {
		// Uses unique pointers to avoid destructing the ServerTCPSocket on push_back
		// (freeaddrinfo doesn't play nicely when called twice).

		// Socket description asks for TCP
		if (it->type == SocketType::TCP) {
			std::unique_ptr<ServerTCPSocket> serv_sock_ptr(
				new ServerTCPSocket(*it, idemux_ptr, debug)
			);	

			listen_socket_ptrs.emplace(serv_sock_ptr->describe(), std::move(serv_sock_ptr));
		// Socket description asks for UDP
		} else { // UDP
			// You can't listen() nor accept() on datagram sockets. Immediately create a
			// UDPSocket. As we're only handling one client, we'll just call connect() the first
			// time someone sends something to us.
			std::shared_ptr<Socket> socket_ptr(
				new ServerUDPSocket(*it, idemux_ptr, debug));
			socket_ptrs.insert({socket_ptr->describe(), socket_ptr});
			imux_ptr->attachSocket(socket_ptr);
		}
	}

	if (debug >= 1) {debugOut(1,
	"succesfully set up the server with " + std::to_string(listen_socket_ptrs.size()) + \
	" TCP sockets, " + std::to_string(socket_ptrs.size()) + " UDP sockets"
	);}
}


void Server::start() {
	// Start a single thread for every UDPServerSocket (which are the only ones in socket_ptrs
	// at this point)
	for (auto it=socket_ptrs.begin(); it!=socket_ptrs.end(); it++) {
		threads.emplace(
			it->second->describe(), 
			std::thread([it] () {it->second->startReceiving();})
		);
		if (debug >= 2) {debugOut(2,
		std::string("started thread for ") + it->second->describeFull()
		);}
	}

	// Start single listening thread for all TCP server sockets
	std::thread listen_t([this] () {this->performListening();});

	if (debug >= 2) {debugOut(2,
	"started the listening thread"
	);}

	// Start imux thread
	std::thread imux_t([this] () {this->imux_ptr->readTunLoop();});
	
	if (debug >= 2) {debugOut(2,
	"started the imux thread"
	);}

	// Join tun thread
	imux_t.join();

	// Join socket_ptr_threads
	for (auto it=threads.begin(); it!=threads.end(); it++) {
		it->second.join();
	}

	// Join TCP server listeners thread
	listen_t.join();
}


void Server::performListening() {
	// Endless loop that uses select() to synchonously handle multiple listening sockets.
	while (true) {
		if (debug >= 1) {debugOut(1,
		"waiting for incoming connections..."
		);}	
		int max_fd = 0;
		int ret;
		fd_set listen_fd_set;	// Will hold all file descriptors that we want to check.
								// The file descriptors belong to the listening sockets.

		FD_ZERO(&listen_fd_set);		// Zero out the set. Required as it's based on bits being set.

		// Fill the fd_set with all socket fd's we know about.
		for (auto it=listen_socket_ptrs.begin(); it!=listen_socket_ptrs.end(); it++) {
			int fd = it->second->getFD();
			max_fd = std::max(fd, max_fd);
			FD_SET(fd, &listen_fd_set);
		}

		// Give us the fd's that have data
		ret = select(max_fd + 1, &listen_fd_set, NULL, NULL, NULL);

		if (ret < 0) {
			// Something serious may be going on, but let's just print it
			errorOut(
			std::string("select error: ") + strerror(errno) + ". ignoring."
			);
			//throw TunException(std::string("select error: ") + strerror(errno));
		}
		//std::cout << std::to_string(ret) << std::endl;
		// For each socket's fd, check whether it's marked as ready in the fd_set.
		// If so, handle it.
		for (auto it=listen_socket_ptrs.begin(); it!=listen_socket_ptrs.end(); it++) {
			if (FD_ISSET(it->second->getFD(), &listen_fd_set)) {
				// A peer connected to us
				try {
					auto peer_socket_ptr = it->second->acceptPeerConnection();
					// Now peer_socket_ptr is the socket that is connected to the peer.
					socket_ptrs.insert({peer_socket_ptr->describe(), peer_socket_ptr});
					imux_ptr->attachSocket(peer_socket_ptr);
					threads.emplace(
						peer_socket_ptr->describe(), 
						std::thread([peer_socket_ptr] () {peer_socket_ptr->startReceiving();})
					);
				} catch (SocketException &e) {
					// Error may occur but there's no reason now to jump through hoops.
					std::cerr << e.what() << std::endl;
					continue;
				}
				
			
			}
		}
	}
}