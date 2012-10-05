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
    float minimalDistanceForRetraction;
    float retraction;
    float retractionSpeed;
    bool loopAlways;
    
    void setup() {
        //settings loaded in loadSettings
    }
    
    void print(bool exportOnly=false) {
        cout << "print" << endl;
                
        gcode.lines.clear();
        gcode.insert("gcode/start.gcode");
        
        int layers = maxObjectHeight / layerHeight; //maxObjectHeight instead of objectHeight

        cout << "objectHeight = " << objectHeight << endl;
        cout << "layerHeight = " << layerHeight << endl;
        cout << "numLayers = " << layers << endl;
        
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

            float simplifyIterationsDuringPrint = ini.get("simplifyIterationsDuringPrint",10);
            float simplifyDistanceDuringPrint = ini.get("simplifyDistanceDuringPrint",0.0f);
            if (simplifyDistanceDuringPrint!=0) ofxSimplifyPath(p,simplifyIterationsDuringPrint,10,simplifyDistanceDuringPrint); //10 iterations, leave minimal 10 points, otherwise max distance of ...mm
            
            subpaths = p.getSubPaths();
            points = ofxGetPointsFromPath(p);
            
            if (layer==0) {
                if (ini.get("firstLayerSlow",true)) gcode.add("M220 S80"); //slow speed
            } else if (layer==1) {
                gcode.add("M106");      //fan on
                gcode.add("M220 S100"); //normal speed
                //gcode.add("G92 Z"+ofToString(layerHeight)); //??
            }
            
            int curLayerCommand=0;
            int totalLayerCommands=points.size();
                        
            //cout << layerScale << endl;
            
            for (int j=0; j<subpaths.size(); j++) {
                
                vector<ofSubPath::Command> &commands = subpaths[even ? j : subpaths.size()-1-j].getCommands();
                
                for (int i=0; i<commands.size(); i++) {
                    int last = commands.size()-1;
                    
                    //ofPoint from,
                    ofPoint to;
                                        
                    //if (isLoop && i==last) continue; //prevent double action. FIXME: geeft problemen bijv een low res cirkel
                    
                    if (even || isLoop || loopAlways) {
                        to = commands[i].to; //deze
                        //from = commands[i>0?i-1:0].to; //vorige
                    } else {
                        to = commands[last-i].to; //deze achterwaarts
                        //from = commands[i>0?last-i+1:last].to; //volgende
                    }
                    
                    //if (!isLoop && i==0) cout << prev.distance(to) << endl;
                                        
                    float sublayer = layer==0 ? 0 : layer + (useSubLayers ? float(curLayerCommand)/totalLayerCommands : 0); //
                    float z = sublayer*layerHeight+zOffset;
                    bool isTraveling = !isLoop && i==0 && prev.distance(to)>minimalDistanceForRetraction;
                    float speed = isTraveling ? travelrate : feedrate;
                    
                    if (isTraveling) {
                         gcode.addCommandWithParams("G1 E%03f F%03f", extruder-retraction, retractionSpeed);   
                         gcode.addCommandWithParams("G1 X%03f Y%03f Z%03f F%03f", to.x, to.y, z, speed);
                         gcode.addCommandWithParams("G1 E%03f F%03f", extruder, retractionSpeed);
                    }
                    else {
                        extruder += prev.distance(to) * filamentThickness * wallThickness * layerHeight;   
                        gcode.addCommandWithParams("G1 X%03f Y%03f Z%03f F%03f E%03f", to.x, to.y, z, speed, extruder);
                    }                    
                    
                    curLayerCommand++;
                    prev = to;
                } //for commands
            } //for subpaths
            
            if (float(layer)/layers > objectHeight/maxObjectHeight) break;
            
        } //for layers
        
        gcode.insert("gcode/end.gcode");
        
        string hourMinutes = ofxFormatDateTime(ofxGetDateTime(),"%H.%M");
        
        cout << hourMinutes << endl;
        
        string filename = "gcode/"+files.getFilename()+"_"+hourMinutes+".gcode";

        gcode.save(filename);
        
        cout << "exported " << filename << endl;
        
        if (exportOnly) {
            ofFile f(filename);
            string copyGCodeToPath = ini.get("copyGCodeToPath","");
            if (copyGCodeToPath!="") {
                ofFile sd(copyGCodeToPath);
                string shortName = f.getFileName().substr(0,8);
                shortName = ofxStringBeforeFirst(shortName, "."); //remove ext
                //cout << shortName << endl;
                if (sd.exists()) {
                    cout << "copy " << shortName+".gco" << " to " << copyGCodeToPath << endl;
                    f.copyTo(copyGCodeToPath+"/"+shortName+".gco");
                }
            }
        }
        
        if (ini.get("printingDisabled",false)==true) return;
        
        if (exportOnly) return;

        //send generated gcode to printer
        cout << "ultimaker.isBusy: " << ultimaker.isBusy << endl;
        cout << "frame: " << ofGetFrameNum() << endl;
        if (ofGetFrameNum()<210 || ultimaker.isBusy) return;
        ultimaker.load(filename);
        ultimaker.startPrint();
    }
};