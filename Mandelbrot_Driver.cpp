#include <iostream>

extern "C" {
    double mandel(double,double,double,double);
    double putchard(double X) { fputc((char)X,stderr); return 0;}
}

int main() {
    mandel(-2.3, -1.3, 0.05, 0.07);
}
