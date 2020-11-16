#include "transmission.hpp"

#include <iostream>

int main(int argc, char * argv[]) {
    if (argc != 2) {
        std::cout << "Usage ./" << argv[0] << " <file name>\n";
        return -1;
    }

    transmission::DataTransmitter transmitter;
    transmitter.transmit(argv[1]);
    return 0;
}