#ifndef IMUX_H
#define IMUX_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "queue.h"
#include "socket.h"
#include "tun.h"

class IMux {
public:
	IMux(std::shared_ptr<Tun> tun, int debug=0);
	virtual ~IMux() = default;
	void attachSocket(std::shared_ptr<Socket> socket);
	void readTunLoop();
	//oid detachSocket(Socket &socket);
private:
	void handleMessage(Message &message);
	std::map<std::string, std::shared_ptr<Socket>> sockets_map;
	std::vector<std::shared_ptr<Socket>> sockets_vector;
	std::shared_ptr<Tun> tun_ptr;
	int debug;
	int index = 0;
};


#endif