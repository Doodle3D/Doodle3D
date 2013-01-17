#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Printer {
public:
    float screenToMillimeterScale;
    float wallThickness;
    float zOffset;
    float useSubLayers;
    float filamentThickness;
    float minimalDistanceForRetraction;
    float retraction;
    float retractionSpeed;
    bool loopAlways;
    bool enableTraveling;
    float speed;
    float travelSpeed;
    
    Printer() {
        screenToMillimeterScale=.3f;
        speed=35;
        travelSpeed=200;
        wallThickness=.8f;
        zOffset=0;
        useSubLayers=true;
        filamentThickness=2.89;
        loopAlways=false;
    }
    
    void print(string startGcodeFilename, string endGcodeFilename, bool exportOnly=false) {
        cout << "print" << endl;
                
        gcode.lines.clear();
        gcode.insert(startGcodeFilename);
        
        int layers = maxObjectHeight / layerHeight; //maxObjectHeight instead of objectHeight
        float extruder = 0;
        ofPoint prev;

        for (int layer=0; layer<layers; layer++) {
            ofPath p = path;
            
            vector<ofSubPath> &subpaths = p.getSubPaths();
            vector<ofPoint*> points = ofxGetPointsFromPath(p);
        
            if (points.size()<2) return;
            bool even = (layer%2==0);
            float progress=float(layer)/layers;
            float layerScale = scaleFunction(float(layer)/layers);
            bool isLoop = points.front()->distance(*points.back())<3;

            p.translate(-ofxGetCenterOfMass(points));
            p.scale(screenToMillimeterScale,-screenToMillimeterScale);
            p.scale(layerScale,layerScale);
            p.rotate(twists*progress*360,ofVec3f(0,0,1));
            
            subpaths = p.getSubPaths();
            points = ofxGetPointsFromPath(p);
            
            if (layer==0) {
                gcode.add("M107");
                if (ini.get("firstLayerSlow",true)) gcode.add("M220 S80"); //slow speed
            } else if (layer==1) {
                gcode.add("M106");      //fan on
                gcode.add("M220 S100"); //normal speed
            }
            
            enableTraveling = ini.get("enableTraveling",true);
            
            int curLayerCommand=0;
            int totalLayerCommands=points.size();
                                    
            for (int j=0; j<subpaths.size(); j++) {
                
                vector<ofSubPath::Command> &commands = subpaths[even ? j : subpaths.size()-1-j].getCommands();
                
                for (int i=0; i<commands.size(); i++) {
                    int last = commands.size()-1;                
                    ofPoint to = commands[(even || isLoop || loopAlways) ? i : last-i].to;
                    float sublayer = layer==0 ? 0 : layer + (useSubLayers ? float(curLayerCommand)/totalLayerCommands : 0);
                    float z = sublayer*layerHeight+zOffset;
                    bool isTraveling = !isLoop && i==0 && prev.distance(to)>minimalDistanceForRetraction;
                    
                    if (enableTraveling && isTraveling) {
                         gcode.addCommandWithParams("G1 E%03f F%03f", extruder-retraction, retractionSpeed);   
                         gcode.addCommandWithParams("G1 X%03f Y%03f Z%03f F%03f", to.x, to.y, z, speed*60);
                         gcode.addCommandWithParams("G1 E%03f F%03f", extruder, retractionSpeed);
                    }
                    else {
                        extruder += prev.distance(to) * wallThickness * layerHeight / filamentThickness;
                        gcode.addCommandWithParams("G1 X%03f Y%03f Z%03f F%03f E%03f", to.x, to.y, z, travelSpeed*60, extruder);
                    }                    
                    
                    curLayerCommand++;
                    prev = to;
                } //for commands
            } //for subpaths
            
            if (float(layer)/layers > objectHeight/maxObjectHeight) break;
            
        } //for layers
        
        gcode.insert(endGcodeFilename);
        string hourMinutes = ofxFormatDateTime(ofxGetDateTime(),"%H.%M");
        string filename = gcodeFolder+files.getFilename()+"_"+hourMinutes+".gcode";
        gcode.save(filename);
                
        if (exportOnly) {
            ofFile f(filename);
            string copyGCodeToPath = ini.get("copyGCodeToPath","");
            if (copyGCodeToPath!="") {
                ofFile sd(copyGCodeToPath);
                string shortName = f.getFileName().substr(0,8);
                shortName = ofxStringBeforeFirst(shortName, "."); //remove ext
                if (sd.exists()) {
                    cout << "copy " << shortName+".gco" << " to " << copyGCodeToPath << endl;
                    f.copyTo(copyGCodeToPath+"/"+shortName+".gco");
                }
            }
        }
        
        if (exportOnly || ini.get("printingDisabled",false)==true) return;
        
        //send generated gcode to printer
        cout << "ultimaker.isBusy: " << ultimaker.isBusy << endl;
        cout << "frame: " << ofGetFrameNum() << endl;
        if (ofGetFrameNum()<210 || ultimaker.isBusy) return;
        
        ultimaker.load(filename);
        ultimaker.startPrint();
    }
    
};