#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Printer {
public:
    float screenToMillimeterScale;
    float feedrate;
    float travelrate;
    float wallThickness;
    float zOffset;
    float useSubLayers;
    float filamentThickness;
    
    void setup() {
        screenToMillimeterScale=ini.get("screenToMillimeterScale",.3f);
        feedrate = ini.get("speed",35)*60;
        travelrate = ini.get("travelrate",250)*60;
        wallThickness = ini.get("wallThickness",.8f);
        zOffset = ini.get("zOffset",0.0f);
        useSubLayers = ini.get("useSubLayers",true);
        twists = ini.get("twists",0.0f);
        filamentThickness = ini.get("filamentThickness",2.89f)/10; ////waarom /10 ????
    }
    
    void print() {
        cout << "print" << endl;
                
        gcode.lines.clear();
        gcode.insert("gcode/start.gcode");
        
        int layers = objectHeight / layerHeight;
        float extruder = 0;

        for (int layer=0; layer<layers; layer++) {
            ofPath p = path;
            vector<ofSubPath> &subpaths = p.getSubPaths();
            vector<ofPoint*> points = ofxGetPointsFromPath(p);
        
            if (points.size()<2) return;
            bool even = (layer%2==0);
            float progress=float(layer)/layers;
            float layerScale = scaleFunction(float(layer)/layers); //hier nog corrigeren voor objectHeight vs maxObjectHeight?
            bool isLoop = points.front()->distance(*points.back())<10;
            
            p.translate(-ofxGetCenterOfMass(points));
            p.scale(screenToMillimeterScale,-screenToMillimeterScale);
            p.scale(layerScale,layerScale);
            p.rotate(twists*progress*360,ofVec3f(0,0,1));
                        
            if (layer==0) {
                gcode.add("M220 S80"); //slow speed
            } else if (layer==1) {
                gcode.add("M106");      //fan on
                gcode.add("M220 S100"); //normal speed
                gcode.add("G92 Z"+ofToString(layerHeight)); //??
            }
            
            for (int j=0; j<subpaths.size(); j++) {
                
                vector<ofSubPath::Command> &commands = subpaths[even ? j : subpaths.size()-1-j].getCommands();
                
                for (int i=0; i<commands.size(); i++) {
                    int last = commands.size()-1;
                    
                    ofPoint from,to;
                                        
                    if (isLoop && i==last) continue; //prevent double action. FIXME: geeft problemen bijv een low res cirkel
                    
                    if (even || isLoop) {
                        to = commands[i].to; //deze
                        from = commands[i>0?i-1:0].to; //vorige
                    } else {
                        to = commands[last-i].to; //deze achterwaarts
                        from = commands[i>0?last-i+1:last].to; //volgende
                    }
                    
                    extruder += from.distance(to) * filamentThickness * wallThickness * layerHeight;
                    
                    float sublayer = layer==0 ? 0 : layer + (useSubLayers ? float(i)/commands.size() : 0);
                    
                    float z = sublayer*layerHeight+zOffset;
                    float speed = !isLoop && i==0 ? travelrate : feedrate;
                    
                    gcode.addCommandWithParams("G1 X%03f Y%03f Z%03f F%03f E%03f", to.x, to.y, z, speed, extruder);
                }
            }
        }
        
        gcode.insert("gcode/end.gcode");
        gcode.save("gcode/output.gcode");
        
        //send generated gcode to printer
        cout << "ultimaker.isBusy: " << ultimaker.isBusy << endl;
        cout << "frame: " << ofGetFrameNum() << endl;
        if (ofGetFrameNum()<210 || ultimaker.isBusy) return;
        ultimaker.load("gcode/output.gcode");
        ultimaker.startPrint();
    }
};