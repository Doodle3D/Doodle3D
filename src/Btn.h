#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Btn {
public:
    
    int hitColor;
    bool selected;
    
    void setup(int hitColor) {
        selected = false;
        this->hitColor = hitColor;
    }
    
    bool hitTest(int x, int y) {
        return ::mask.getColor(x,y).getHex()==hitColor;
    }
    
};