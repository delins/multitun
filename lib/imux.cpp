#include <iostream>
#include <map>
#include <memory>

#include "imux.h"
#include "util.h"

IMux::IMux(std::shared_ptr<Tun> tun_ptr, int debug) : tun_ptr(tun_ptr), debug(debug) {}


void IMux::attachSocket(std::shared_ptr<Socket> socket) {
	sockets_map.insert({socket->describe(), socket});
	sockets_vector.push_back(socket);
	if (debug >= 2) {debugOut(2,
	std::string("attached to imux ") + socket->describeFull()
	);}
}


void IMux::readTunLoop() {
	while (true) {
		Message msg;
		tun_ptr->receive(msg);	// will block until there's a message.
		handleMessage(msg);
	}
}


void IMux::handleMessage(Message &message) {
	// Just pick the first one in the vector for now.

	if (!sockets_vector[index]->isReady()) {
		// Too bad, I don't feel like doing something smart right now.
		if (debug >= 2) {debugOut(2,
		std::string("") + sockets_vector[index]->describeFull() + " was chosen but is not ready (no peer?). dropping message."
		);}
		return;
	}
	sockets_vector[index]->sendMessage(message);
	if (debug >= 2) {debugOut(2,
	std::string("chose ") + sockets_vector[index]->describeFull() + " to send data"
	);}

	index = (++index) % sockets_vector.size();
	//(*(sockets.begin()->second)).sendMessage(message);
}