#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Btn {
public:
    
    int hitColor;
    
    void setup(int hitColor) {
        this->hitColor = hitColor;
    }
    
    bool hitTest(int x, int y) {
        return ::mask.getColor(x,y).getHex()==hitColor;
    }
    
};