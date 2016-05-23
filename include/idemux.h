#ifndef IDEMUX_H
#define IDEMUX_H

#include <memory>

#include "queue.h"
//#include "tun.h"

class Tun;	// Forward declaration to Tun to handle circular dependencies.
			// tun.h is included in idemux.cpp to get the rest.

class IDeMux{
public:
	IDeMux() = default;
	IDeMux(std::shared_ptr<Tun> tun, int debug=0);
	virtual ~IDeMux() = default;
	void handleMessage(Message &msg);
private:
	std::shared_ptr<Tun> tun_ptr;
	int debug;
};


#endif