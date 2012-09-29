#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Files {
public:
    
    int cur;
    ofDirectory dir;
    ofDirectory monitor;
    
    Files() {
        cur=-1;
    }
    
    void setup() {
        listDir();
        
        //auto load image
        string autoLoadImage = ini.get("autoLoadImage","");
        if (autoLoadImage!="") {
            for (cur=0; cur<dir.numFiles(); cur++) {
                if (dir.getName(cur)==autoLoadImage || dir.getPath(cur)==autoLoadImage) {
                    load(dir.getPath(cur));
                    break;
                }
            }
        }
        
        string monitorFolder = ini.get("monitorFolder","");
        if (monitorFolder!="") {
            monitor.reset();
            monitor.listDir(monitorFolder);
            cout << "monitor numFiles: " << monitor.numFiles() << endl;
        }

    }
        
    void listDir() {
        dir.reset();
        string monitorFolder = ini.get("monitorFolder","");
        dir.listDir(monitorFolder); //"doodles/");
    }
    
    void loadPrevious() {
        if (dir.numFiles()==0) return;
        cur = (cur-1+dir.numFiles()) % dir.numFiles();
        load(dir.getPath(cur));
    }
    
    void loadNext() {
        if (dir.numFiles()==0) return;
        cur = (cur+1+dir.numFiles()) % dir.numFiles();
        load(dir.getPath(cur));
    }
        
    void load() {
        ofFileDialogResult result = ofSystemLoadDialog();
        if (result.bSuccess) {
            unloadFile();
            return load(result.getPath());
        }
    }
    
    void load(string filename) {
        //if (!path) return;
        if (!ofxFileExists(filename)) return; //file not existing or removed   
        path.clear();
        vector<string> lines = ofxLoadStrings(filename);
        for (int i=0; i<lines.size(); i++) {
            vector<string> coords = ofSplitString(lines[i], " ");
            for (int j=0; j<coords.size(); j++) {
                vector<string> tuple = ofSplitString(coords[j], ",");
                if (tuple.size()!=2) return; //error in textfile
                float x = ofToFloat(tuple[0]);
                float y = ofToFloat(tuple[1]);
                ofPoint p = ofPoint(x,y); // + ofPoint(bounds.x, bounds.y);
                p += ini.get("loadOffset",ofPoint());
                if (j==0) path.moveTo(p.x,p.y);
                else path.lineTo(p.x,p.y);
            }
        }
        
        //temp
        vector<ofPoint*> points = ofxGetPointsFromPath(path);
        if (points.size()<2) return;
        bool isLoop = points.front()->distance(*points.back())<25;
        cout << filename << ", loop=" << isLoop << endl;
//        ofxSimplifyPath(path);
    }

    void deleteCurrentFile() {
        ofFile f(dir.getPath(cur));
        f.remove();
        listDir();
        load(dir.getPath(cur)); //reload the thing on place cur
    }

    void save() {
        if (cur>-1 && cur<dir.numFiles()) {
            save(dir.getName(cur));
        } else {
            saveAs();
        }
    }
    
    void saveAs() {
        //ofSetFullscreen(false);
        ofFileDialogResult result = ofSystemSaveDialog("doodle.txt","Je tekening wordt altijd opgeslagen in de doodles map.");
        if (result.bSuccess) save(result.getName());
        //ofSetFullscreen(true);
    }
    
    void save(string filename) {
        vector<string> lines;
        vector<ofSubPath> &subpaths = path.getSubPaths();
        for (int i=0; i<subpaths.size(); i++) {
            string line;
            vector<ofSubPath::Command> &commands = subpaths[i].getCommands();
            for (int j=0; j<commands.size(); j++) {
                ofPoint p = commands[j].to; // - ofPoint(bounds.x, bounds.y);
                line+=ofToString(p.x) + "," + ofToString(p.y) + " ";
            }
            lines.push_back(line);
        }
        ofxSaveStrings("doodles/" + filename,lines); //will only save file in data/doodles/ folder
        listDir();
        cout << "saved: " << filename << endl;
    }
    
    void unloadFile() {
        cur=-1;
    }
    
    string getFilename() {
        if (cur<0 || cur>=dir.numFiles()) return "noname";
        else {
//            cout << cur << endl;
            return dir.getName(cur);   
        }
    }
};