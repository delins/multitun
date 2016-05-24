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
