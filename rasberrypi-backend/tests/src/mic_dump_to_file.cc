#include "mic.hpp"

#include <iostream>
#include <fstream>

char buffer[2048];

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage ./" << argv[0] << " <file name>\n";
        return -1;
    }

    std::fstream file(argv[1], std::ios::out | std::ios::binary);
    Microphone mic;
    while (true) {
        mic.read(buffer, sizeof(buffer));
        file.write(buffer, sizeof(buffer));
    }
}
