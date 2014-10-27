
#include "application.h"

extern "C" {
       
    
int foo() {
    RGB.control(true);
    RGB.color(255,0,127);
    Serial.println("Hello modular world!");    
    return 42;
}
}
