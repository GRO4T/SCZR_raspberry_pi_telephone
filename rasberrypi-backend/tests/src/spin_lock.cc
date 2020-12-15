#include <iostream>
#include "ipc.hpp"

int main() {
  spin_locked_resource<int> resource;

  *resource.lock() = 1;
}
