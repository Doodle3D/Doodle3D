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
    
    void setup() {
        screenToMillimeterScale=ini.get("screenToMillimeterScale",.3f);
        feedrate = ini.get("speed",35)*60;
        travelrate = ini.get("travelrate",250)*60;
        wallThickness = ini.get("wallThickness",.8f);
        zOffset = ini.get("zOffset",0.0f);
        useSubLayers = ini.get("useSubLayers",true);
        twists = ini.get("twists",0.0f);
    }
    
    void print() {
        cout << "make: frame=" << ofGetFrameNum() << endl;
        
        int layers = objectHeight / layerHeight;
        float extruder = 0;
        ofPath p = path; //tmpPath = path;
        
        gcode.lines.clear();
        gcode.insert("gcode/start.gcode");
        
        vector<ofSubPath> &subpaths = p.getSubPaths();
        vector<ofPoint*> points = ofxGetPointsFromPath(p);
        
        if (points.size()<2) return;
        
        bool isLoop = points.front()->distance(*points.back())<10;
        
        cout << "ultimaker.isBusy: " << ultimaker.isBusy << endl;
        
        for (int layer=0; layer<layers; layer++) {
            float progress=float(layer)/layers;
            
            //reset
            p = path;
            
            points = ofxGetPointsFromPath(p);
            
            //scale to machine dimensions
            p.translate(-ofxGetCenterOfMass(points));
            p.scale(screenToMillimeterScale,-screenToMillimeterScale);
            
            float layerScale = scaleFunction(float(layer)/layers);
            
            p.scale(layerScale,layerScale); //1+layer*scaleStep, 1+layer*scaleStep);
            p.rotate((twists*360)*progress,ofVec3f(0,0,1));
            
            bool even = (layer%2==0);
            
            if (layer==0) {
                gcode.add("M220 S80");
            }
            if (layer==1) {
                gcode.add("M106");
                gcode.add("M220 S100");
                gcode.add("G92 Z"+ofToString(layerHeight));
            }
            
            for (int j=0; j<subpaths.size(); j++) {
                
                vector<ofSubPath::Command> &commands = subpaths[even ? j : subpaths.size()-1-j].getCommands();
                
                for (int i=0; i<commands.size(); i++) {
                    int last = commands.size()-1;
                    
                    ofPoint from,to;
                    
                    if (isLoop) even = true; //overrule, don't go backwards
                    
                    if (isLoop && i==last) continue; //prevent double action. FIXME geeft problemen
                    
                    if (even) {
                        to = commands[i].to; //deze
                        from = commands[i>0?i-1:0].to; //vorige
                    } else {
                        to = commands[last-i].to; //deze achterwaarts
                        from = commands[i>0?last-i+1:last].to; //volgende
                    }
                    
                    extruder += (from.distance(to) * screenToMillimeterScale) * wallThickness * layerHeight;
                    
                    float sublayer = layer==0 ? 0 : layer + (useSubLayers ? float(i)/commands.size() : 0);
                    
                    gcode.addCommandWithParams("G1 X%03f Y%03f Z%03f F%03f E%03f",
                                               to.x, to.y,
                                               sublayer*layerHeight+zOffset,
                                               !isLoop && i==0 ? travelrate : feedrate,
                                               extruder);
                }
            }
        }
        
        gcode.insert("gcode/end.gcode");
        gcode.save("gcode/output.gcode");
        
        if (ofGetFrameNum()<210 || ultimaker.isBusy) return;
        ultimaker.load("gcode/output.gcode");
        ultimaker.startPrint();
        
        p = path;
    }
};