#ifndef OPTIONS_H
#define OPTIONS_H


#include <string>
#include <vector>
#include "socket.h"


class Options {
public:
	Options();
	void parse(int argc, char* argv[]);
	void printHelp(std::ostream &out);
	void printOptions(std::ostream &out);
	void printVersion(std::ostream &out);
	std::string prog_name;
	bool help_flag = false;
	bool version_flag = false;
	bool server_flag = false;
	bool client_flag = false;
	std::string if_name;
	int debug_level = 0;
	std::string clone_dev;
	bool options_flag = false;
	bool close_tun = false;
	std::vector<SocketDescription> sock_des;
	int major_version = @SIMPLETUN_VERSION_MAJOR@;
	int minor_version = @SIMPLETUN_VERSION_MINOR@;
	int patch_version = @SIMPLETUN_VERSION_PATCH@;
};


class OptionsParseException : public std::exception {
private:
	std::string errorMsg;
public:
	OptionsParseException(const std::string &msg) 
		: errorMsg(std::string("Parse error: ") + msg) {}
	~OptionsParseException() throw() {};
	virtual const char* what() const throw() {
		return errorMsg.c_str();
	}

};


#endif