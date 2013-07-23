#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Printer {
public:
    float screenToMillimeterScale;
    float wallThickness;
    float zOffset;
    bool useSubLayers;
    float filamentThickness;
    float minimalDistanceForRetraction;
    float retraction;
    float retractionSpeed;
    bool loopAlways;
    bool enableTraveling;
    float speed;
    float travelSpeed;
    float hop;

    Printer() {
        screenToMillimeterScale=.3f;
        speed=35;
        travelSpeed=200;
        retractionSpeed=.1; //guess
        wallThickness=.8f;
        zOffset=0;
        useSubLayers=true;
        filamentThickness=2.89;
        loopAlways=false;
        retraction=0;
        minimalDistanceForRetraction=1;
        hop=0;
    }

    void print(string outputGcodeFilename, string startGcodeFilename, string endGcodeFilename) { //, bool exportOnly=false) {
        cout << "printing to: " << outputGcodeFilename << endl;
        cout << "start code: " << startGcodeFilename << endl;
        cout << "end code: " << endGcodeFilename << endl;

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
                gcode.add("M107"); //fan off
                if (ini.get("firstLayerSlow",true)) gcode.add("M220 S40"); //slow speed
            } else if (layer==2) { ////////LET OP
                gcode.add("M106");      //fan on
                gcode.add("M220 S100"); //normal speed
            }

            enableTraveling = ini.get("enableTraveling",true);

//            cout << "zOffset: " << zOffset << endl;

            int curLayerCommand=0;
            int totalLayerCommands=points.size();

            for (int j=0; j<subpaths.size(); j++) {

                vector<ofSubPath::Command> &commands = subpaths[even ? j : subpaths.size()-1-j].getCommands();

                for (int i=0; i<commands.size(); i++) {
                    int last = commands.size()-1;
                    ofPoint to = commands[(even || isLoop || loopAlways) ? i : last-i].to;
                    float sublayer = (layer==0) ? 0.0 : layer + (useSubLayers ? float(curLayerCommand)/totalLayerCommands : 0);
                    float z = (sublayer+1)*layerHeight+zOffset;
                    //ofxExit();
                    //bool isTraveling = i==0; //!isLoop && i==0 && prev.distance(to)>minimalDistanceForRetraction;
                    bool isTraveling = !isLoop && i==0; ///i==0; //changed 23 july 2013
                    bool doRetract = prev.distance(to)>minimalDistanceForRetraction;
                    
                    if (enableTraveling && isTraveling) {
                        if (doRetract) gcode.addCommandWithParams("G1 E%03f F%03f", extruder-retraction, retractionSpeed*60);
                        gcode.addCommandWithParams("G1 X%.03f Y%.03f Z%.03f F%.03f", to.x, to.y, z + (doRetract ? hop : 0), travelSpeed*60);
                        if (doRetract) gcode.addCommandWithParams("G1 E%03f F%03f", extruder, retractionSpeed*60);
                    }
                    else {
                        extruder += prev.distance(to) * wallThickness * layerHeight / filamentThickness;
                        gcode.addCommandWithParams("G1 X%.03f Y%.03f Z%.03f F%.03f E%.03f", to.x, to.y, z, speed*60, extruder);
                    }

                    curLayerCommand++;
                    prev = to;
                } //for commands
            } //for subpaths

            if (float(layer)/layers > objectHeight/maxObjectHeight) break;

        } //for layers

        gcode.insert(endGcodeFilename);
        gcode.save(outputGcodeFilename);
    }

};
