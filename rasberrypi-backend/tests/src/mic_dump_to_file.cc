#include "mic.hpp"
#include "transmission.hpp"

#include <iostream>
#include <fstream>

__attribute__((packed)) struct MicPacket {
	uint32_t id;
	int16_t data[128];
	uint32_t crc;
};
char buffer[2048];

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage " << argv[0] << " <file name>\n";
        return -1;
    }
    transmission::MicPacket packet;

    std::fstream file(argv[1], std::ios::out | std::ios::binary);
    Microphone mic;
    while (true) {
        mic.read((char*)&packet, sizeof(packet));
        if (packet.id != 0x31434150 && packet.id != 0x32434150){
            abort();
        }
        file.write(buffer, sizeof(buffer));
    }
}
