#include <iostream>

#include <server.hpp>

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
        return -1;
    }

    try {
        Server server(argv[1]);
        server.run();
    } catch (const exception& error) {
        cerr << "Server error: " << error.what() << endl;
    } catch (...) {
        cerr << "Unknown server error occurred." << endl;
    }

    return 0;
}
