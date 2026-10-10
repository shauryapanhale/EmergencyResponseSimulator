#include <iostream>
#include "ers/common/Version.h"

int main() {
    std::cout << ers::projectName() << "\n" << ers::disclaimer() << std::endl;
    return 0;
}