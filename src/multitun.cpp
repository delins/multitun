#include <cstdlib>
#include <exception>
#include <iostream>


#include "options.h"
//#include "simpletun.h"
#include "roles.h"
#include "util.h"


int main(int argc, char* argv[]) {
Options options;

	// Parse the options, fail on error.
	try {
		options.parse(argc, argv);
		if (options.help_flag) {
			options.printHelp(std::cout);
			std::exit(0);
		} else if (options.version_flag) {
			options.printVersion(std::cout);
			std::exit(0);
		}
	} catch (OptionsParseException const &e) {
		errorOut(e.what());
		options.printHelp(std::cerr);
		std::exit(1);
	}


	if (options.options_flag) {
		options.printOptions(std::cout);
		exit(0);
	}

	if (options.client_flag) {
		// Client code
		try {
            Client client(options);
                /* Client initiates:
                    - Socket connector object for each thing that is specified
                        - wrapper around socket, because sockets can be either tcp or udp
                          (subclass)
                    - Inverse multiplexer (sender)
                        - is given the socket objects
                        send()
                            - performs some balancing algorithm to call the correct socket's send
                    - Inverse demultiplexer (receiver)
                        - one thread for each socket .receive
                        - when a socket recv()s, it sends this packet to the demux (by subscription)
                            (enqueue())
                        - demux has queue handler (but later). now just forward out to our tun device
                */

			client.start();
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
	} else {
		// Server code
		try {
			Server server(options);
			server.start();
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
	}

}

/*

 +------\     TCP     /------+
tun   socket <---> socket   tun
 +------/             \------+

 +------\     UDP     /------+
tun   socket <---> socket   tun
 +------/             \------+

  +----------\           /------------+
  |        tun1A <---> tun1B          | 
tun0A      tun2A <---> tun2B        tun0B
  |        tun3A <---> tun3B          |
  +----------/           \------------+

Abstraction:

both sides have a tun device
both sides have one or more connections to each other:
  - socket
  - tun
  - tap

-> create socket class
-> create tun class
-> create tap class

outer tun:
  - no actual connection or knowledge of other tun device
  - may be given IPs such that it can reach the other side eventually but still doesnt know about it

socket:
  - does know about the other side, so it directly connected

covert channel tun/tap:
  - our application knows nothing about the device other than that it can get a fd and send and receive data

covert channel internal socket:
  - out application knows its IP, port and TCP/UDP but nothing else. 


Verdict:

One Tun class can be used for everything
How can we use Tap devices? How do we send and receive data from it if we ourselves only give a tun dev?
Socket class can be used for covert channel, no other need

algorithm:
  - from local tun
    - read from local tun
    - choose what link to send to
    - use link's send commmand to send the shit
  - from one of the links:
    - read from link
    - send to local tun

if link fails and needs to be restarted:
  - keep link's object.
  - re-issue connect() command, giving it new device name etc.


classes:
  - Link {sendAll, readAll}
    - Tun
    - Tap
    - Socket
      - TCPSocket
      - UDPSocket

TCPSocket:
  - Needs header that states packet length
    - Is contained it the class and hidden from the outside.



Parallellization

tun0
  - read
    - write to all tunXes whatever style it likes
    - only this tun0 (the listening thead) will write to all tunXes
  - write
    - doesn't have its own thread, obviously
    - get's called by the listening thread from the tunXes
    - should be locked.
    - ACTUALLY NO!
    - this thing is not written to directly: all tunnels may have fragmented packets, as we saw
      with the large ICMP packets. the kernel knows how to reassemble them, but only if they 
      arrive nicely at the same time. 





http://stackoverflow.com/questions/17846089/can-you-write-multiple-ip-packets-in-one-write-to-the-linux-tun-device


Actually multiple packets were returned by the driver because it had been enhanced locally. 
The tun driver that comes with linux only returns one packet per read. 
The driver also expects only one packet per write.

* does this mean that if our local buffer is bigger than the largest packet that we can expect
from the kernel, we get everything in one go? How does this behave if a full packet was written
into the tun0 device by the kernel but we have to issue multiple read()s on it because
the read()s return prematurely? Are we still guaranteed to read only one full packet?

https://ldpreload.com/p/tuntap-notes.txt

http://stackoverflow.com/questions/6263829/receive-packet-by-packet-data-from-tcp-socket

http://stackoverflow.com/questions/18708569/how-to-reconstruct-tcp-stream-from-multiple-ip-packets
*/