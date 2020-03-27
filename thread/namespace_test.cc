#include <iostream>

namespace{
    int a = 555;
    void fun(){
        std::cout << "Hello World\n" << std::endl;
    }
}
int main()
{
    std::cout << ::a << std::endl;
    ::fun();
    return 0;
}

