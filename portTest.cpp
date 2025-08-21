#include <iostream>

#include "filecenter.h"

int main()
{
    PortPoolCenter ppl;
    std::cout<< ppl.availablePortCount() <<std::endl;
    std::cout<< ppl.getPort() <<std::endl;
    return 0;
}
