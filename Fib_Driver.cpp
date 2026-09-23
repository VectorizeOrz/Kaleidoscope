#include <iostream>

extern "C" {
    double fib(double);
    double fibi(double);
}

int main() {
    std::cout << "fib(10) : " << fib(10) << std::endl;
    std::cout << "fibi(10) : " << fibi(10) << std::endl;
}
