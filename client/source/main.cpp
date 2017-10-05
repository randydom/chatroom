#include <iostream>

#include <client.hpp>

int main(int argc, char** argv) {
	if (argc != 3) {
        cerr << "Usage: " << argv[0] << " [address] [port]" << endl;
        return -1;
	}

	try {
		Client client(argv[1], argv[2]);
		client.run();
	} catch (const exception& error) {
		cerr << error.what() << endl;
	}

	return 0;
}
