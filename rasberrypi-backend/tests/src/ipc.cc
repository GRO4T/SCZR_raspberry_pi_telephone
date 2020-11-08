#include "ipc.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << argv[0] << " <0/1>\n";
        return -1;
    }
    
    shared_mem_ptr<int> ptr("/shm_test_int");
    if (!strcmp(argv[1], "1")) {
        *ptr = 10;
        getchar();
    } else {
        getchar();
        std::cout << *ptr << "\n";

    } 

}
