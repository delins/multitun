#include <exception>

#include <string>
#include <sstream>
#include <iostream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <vector>

#include "options.h"
#include "socket.h"


Options::Options() {}


void Options::parse(int argc, char* argv[]) {
	int c;
	// Suppress getopt outputting errors of its own (such as "./simpletun: invalid option -- 'e'")
	opterr = 0;
	prog_name = argv[0] ;
	std::stringstream ss;	
	// The leading colon makes sure we're notified of missing arguments to options. (case ':')
	const char *optstring = ":hvscb:f:t:d:o";
	while ((c = getopt (argc, argv, optstring)) != -1) {
		switch (c) {
			case 'h':
				help_flag = true;
				return;
			case 'v':
				version_flag = true;
				return;
			case 's':
				server_flag = true;
				break;
			case 'c':
				client_flag = true;
				break;
			case 'b': {
					std::stringstream stream1((std::string(optarg)));
					std::string config;
					while (std::getline(stream1, config, ',')) {
						std::stringstream stream2(config);
						std::vector<std::string> elems;
						std::string item;
						while (std::getline(stream2, item, ':')) {
							elems.push_back(item);
						}
						if (elems.size() == 3) {
							SocketType st;
							std::string ip;
							int port;
							try {
								st = string2SocketType(elems[0]);
							} catch (std::invalid_argument &e) {
								ss << "invalid socket(s) description: " << optarg;
								throw OptionsParseException(ss.str());
							}
							ip = elems[1];
							port = std::stoi(elems[2]);
							SocketDescription des = {.type=st, .ip=ip, .port=port};
							sock_des.push_back(des);
						}
					}
				}
				break;
			case 'f':
				if_name = optarg;
				break;
			
			case 't':
				clone_dev = optarg;
				break;
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'o':
				options_flag = true;
				break;
			case ':':
				ss << "option requires an argument: '" << static_cast<char>(optopt) << "'";
				throw OptionsParseException(ss.str());
			case '?':
				ss << "invalid option: '" << static_cast<char>(optopt) << "'";
				throw OptionsParseException(ss.str());
			default:
				ss << "unknown error: " << static_cast<char>(c);
				throw OptionsParseException(ss.str());
		}
		
	}

	if (! (client_flag || server_flag)) {
		throw OptionsParseException("no mode specified: '-s' or '-c'");
	}
	if (client_flag && server_flag) {
		throw OptionsParseException("more than one mode specified: '-s' and '-c'");
	}
	if (client_flag && sock_des.empty()) {
		throw OptionsParseException("client mode but no socket descriptions given: '-b'");
	}
	if (if_name.size() > 16) {
		throw OptionsParseException("interface too long (max 16 characters)");		
	}
	if (clone_dev.empty()) {
		clone_dev = "/dev/net/tun";
	}

}


void Options::printHelp(std::ostream &out) {
	out << "Usage:\n"
		<< prog_name << " {-c | -s} -b SOCKET_DES[,..] [-f IF_NAME] [-d LEVEL] [-t CLONE_DEV] [-o]\n"
		<< prog_name << " -h\n" 
		<< "\tSOCKET_DES format: {UDP|TCP}:IP:PORT\n"
		<< "\t-s: Run as server. Excludes '-c'\n"
		<< "\t-c: Run as client. Excludes '-s'\n"
		<< "\t-f: IFNAME: Interface name. Should be a tun device.\n"
		<< "\t-t: CLONE_DEV: Clone device name. Default \"/dev/net/tun\".\n"
		<< "\t-b: Set socket descriptions. Multiple descriptions are comma separated.\n"
		<< "\t-d: Print extra debug information. 0-3. The higher the more debug info. 0=no debug.\n"
		<< "\t-v: Print version info.\n"
		<< "\t-o: Print parsed options. Exits immediately after."
		<< std::endl;
}


void Options::printVersion(std::ostream &out) {
	out << "Version: " 
		<< major_version
		<< "."
		<< minor_version 
		<< "."
		<< patch_version 
		<< std::endl;
}


void Options::printOptions(std::ostream &out) {
	out << "Options:\n"
		<< "\tprogName: " << prog_name << "\n"
		<< "\tRun as client: " << (client_flag ? "yes" : "no") << "\n"
		<< "\tRun as server: " << (server_flag ? "yes" : "no") << "\n"
		<< "\tInterface name: " << if_name << "\n"
		<< "\tClone device name: " << clone_dev << "\n"
		<< "\tClose tun: " << close_tun << "\n"
		<< "\tDebug level: " << debug_level << "\n";
	if (sock_des.empty()) {
		out << "\tSocket descriptions: None\n";
	} else {
		out << "\tSocket descriptions:\n";
	}
	int iteration = 1;
	for (auto it=sock_des.begin(); it<sock_des.end(); it++) {
		out << "\t\t" << std::to_string(iteration) << ") Socket type: " << socketType2String(it->type) << "\n"
			<< "\t\t   " << "IP: " << it->ip << "\n"
			<< "\t\t   " << "Port: " << it->port << "\n";
		++iteration;
	}
	out	<< "\tPrint options: " << (options_flag ? "yes" : "no")
		<< std::endl;
}