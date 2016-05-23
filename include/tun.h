#ifndef SIMPLETUN_H
#define SIMPLETUN_H

#include <exception>
#include <memory>
#include <string>

#include "options.h"
#include "queue.h"

class Tun {
public:
	Tun(const std::string &if_name, 
		const std::string &clone_dev,
		int debug=0);
	virtual ~Tun();
	void run();
	int readBytes(int fd, char *buf, int n);
	int readAll(int fd, char *buf, int n);
	int writeAll(char *buf, int n);
	int sendSimple(const std::string &s);
	int sendSimple(char buffer[], int count);
	void receive(Message &msg);
	void writeMessage(Message &msg);
	std::string describeFull();
protected:
	std::string if_name;
	int tun_fd;
	int debug;
};


class TunException : public std::exception {
private:
	std::string errorMsg;
public:
	TunException(const std::string &msg) 
		: errorMsg(msg) {}
	~TunException() throw() {};
	virtual const char* what() const throw() {
		return errorMsg.c_str();
	}

};


#endif
