#ifndef UTIL_H
#define UTIL_H


#include <iostream>
#include <string>
#include <sstream>


inline void debugOut(int level, std::string msg) {
	std::cout << "[d" + std::to_string(level) + "] " << msg << std::endl;
}

inline void errorOut(std::string msg) {
	std::cout << "\033[91m[e] " << msg << "\033[0m" << std::endl;
}


#endif