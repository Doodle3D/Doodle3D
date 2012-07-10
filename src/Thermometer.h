#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Thermometer {
public:
    
    ofPoint position;
    ofImage mask,good,bad,warmup;
    bool showWarmUp;

    Thermometer() {
        targetTemperature = 220;
        showWarmUp = false;
    }
    
    void setup() {
        position.set(1105,506);
        mask.loadImage("images/thermo-mask.png");
        good.loadImage("images/krul.png");
        bad.loadImage("images/kruis.png");
        warmup.loadImage("images/opwarmen.png");
    }

    void draw() {
        float level = ofMap(ultimaker.temperature, 0, targetTemperature, 40, mask.height-40, true);
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(position);
        ofSetColor(255,0,0);
        ofRect(1,mask.height-level,mask.width-1,level);
        ofSetColor(255);
        mask.draw(0,0);
//        if (ultimaker.temperature < targetTemperature) bad.draw(90,0);
//        else good.draw(90,0);
        if (showWarmUp && ultimaker.temperature < targetTemperature) warmup.draw(-180,-60);
        ofPopMatrix();
        ofPopStyle();
    }
    
};
