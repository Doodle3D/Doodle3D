#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Thermometer {
public:
    
    ofPoint position;
    ofImage bg,mask,good,bad,warmup;
//    bool showWarmUp;
    bool visible;
    float temperature;

    Thermometer() {
        targetTemperature = 220;
//        showWarmUp = false;
        visible = true;
//        showWarmUp = false;
    }
    
    void setup() {
        position.set(1105,506);
        bg.loadImage("thermobg.png");
        mask.loadImage("thermomask.png");
        good.loadImage("krul.png");
        bad.loadImage("kruis.png");
        //warmup.loadImage("images/opwarmen.png");
    }

    void draw() {
        if (!visible) return;
        //int temperature = 0; //ultimaker.temperature; FIXME
        float level = ofMap(temperature, 0, targetTemperature, 40, mask.height-40, true);
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(position);
        ofSetColor(255);
        bg.draw(0,0);
        ofSetColor(255,0,0);
        ofRect(1,mask.height-level,mask.width-1,level);
        ofSetColor(255);
        mask.draw(0,0);
//        if (temperature < targetTemperature) bad.draw(90,0); // && ultimaker.isBusy && !ultimaker.isPrinting) bad.draw(90,0);
//        else good.draw(90,0);
        
        //if (showWarmUp && ultimaker.isBusy && !ultimaker.isPrinting && temperature < targetTemperature) warmup.draw(-180,-60);
        //ofSetColor(0);
        //if (debug) ofDrawBitmapString(ofToString(ultimaker.temperature),50,230);
        ofPopMatrix();
        ofPopStyle();
    }
    
};
