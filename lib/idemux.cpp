#include <iostream>

#include "idemux.h"
#include "queue.h"
#include "util.h"
#include "tun.h"

IDeMux::IDeMux(std::shared_ptr<Tun> tun_ptr, int debug) :
	tun_ptr(tun_ptr), debug(debug) {

}


void IDeMux::handleMessage(Message &msg) {
	tun_ptr->writeMessage(msg);
}