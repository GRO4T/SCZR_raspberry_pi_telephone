#include <iostream>
#include "ipc.hpp"

int main() {
    fast_deque<char, 10> d;
    for (char c = 'A'; c != 'D'; ++c) {
        *d.push_front() = c;
    }

    auto it = d.pop_back();
    while (d.valid(it)) {
        std::cout << *it << "\n";
        it = d.pop_back();
    }
}
