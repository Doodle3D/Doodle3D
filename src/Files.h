#pragma once
#include "ofMain.h"
#include "ofxExtras.h"
#include "ofxOpenCv.h"
#include "ofxMesh.h"
#include "ofxSTLExporter.h"

class Files {
public:
    
    int cur;
    ofDirectory dir;
//    string doodlesFolder;
    
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
    }
        
    void listDir() {
        dir.reset();
        dir.listDir(doodlesFolder);
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
    
    void loadFromStrings(vector<string> lines) {
        cout << "lines: " << lines.size() << endl;
        for (int i=0; i<lines.size(); i++) {
            vector<string> coords = ofSplitString(lines[i], " ");
            for (int j=0; j<coords.size(); j++) {
                if (coords.size()<=1) break; //ignore very small paths
                vector<string> tuple = ofSplitString(coords[j], ",");
                if (tuple.size()!=2) continue; //{ cout << "error in textfile" << endl; return; }
                float x = ofToFloat(tuple[0]);
                float y = ofToFloat(tuple[1]);
                ofPoint p = ofPoint(x,y); // + ofPoint(bounds.x, bounds.y);
//                cout << p << endl;

                p += ini.get("loadOffset",ofPoint());

                ofRectangle bounds(0,0,640,440);
                if (!bounds.inside(p)) continue;
                
                if (j==0) path.moveTo(p.x,p.y);
                else path.lineTo(p.x,p.y);
            }
        }
        
        //temp
        vector<ofPoint*> points = ofxGetPointsFromPath(path);
        if (points.size()<2) return;
        bool isLoop = points.front()->distance(*points.back())<25;
        //cout << filename << ", loop=" << isLoop << endl;
        
        ofxSimplifyPath(path,simplifyIterations,simplifyMinNumPoints,simplifyMinDistance);
    }
    
    void loadFromImage(string filename) {
        //ofPath path;
        ofImage img;
        img.loadImage(filename);
        img.setImageType(OF_IMAGE_COLOR);
        
        float ratio = float(min(img.width,img.height))/float(max(img.width,img.height));
        
        ofLogNotice() << filename << " " << img.width << "x" << img.height << " ratio: " << ratio;
        
        if (img.width>640) {
            if (img.width>img.height) img.resize(640,640*ratio);
            else img.resize(640,640/ratio);
        }
        
        if (img.height>480) {
            if (img.width>img.height) img.resize(480/ratio,480);
            else img.resize(480*ratio,480);
        }
        
        
        ofxCvColorImage	rgb;
        ofxCvGrayscaleImage gray,filtered;
        ofxCvContourFinder contourFinder;
        
        rgb.allocate(img.width,img.height);
        rgb.setFromPixels(img.getPixelsRef());
        gray = rgb;
        gray.brightnessContrast(0, .5);
        //        gray.mirror(true,false);
        contourFinder.findContours(gray, 10, img.width*img.height/2, 200, true);
        
        path.clear();
        
        for (int i=0; i<contourFinder.blobs.size(); i++) {
            vector<ofPoint> &pts = contourFinder.blobs[i].pts;
            path.moveTo(pts[0]);
            for (int j=0; j<pts.size(); j++) {
                path.lineTo(pts[j]);
            }
            if (pts.size()>0) path.lineTo(pts[0]);
        }
    }
    
    void load(string filename) {
        cout << "Files::load: " << filename << endl;
        //if (!path) return;
        if (!ofxFileExists(filename)) return; //file not existing or removed   
        path.clear();
        string ext = ofxGetFileExtension(filename);
        if (ext==".txt") loadFromStrings(ofxLoadStrings(filename));
        else if (ext==".svg") loadFromSvg(filename);
        else if (ext==".jpg" || ext==".png" || ext==".gif") loadFromImage(filename);
    }

    void deleteCurrentFile() {
        cout << "delete: " << cur << endl;
        if (cur<0 || cur>=dir.numFiles()-1) return;
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
        listDir();
        string filename = "doodle"+ofToString(dir.numFiles()+1)+".svg";
        ofFileDialogResult result = ofSystemSaveDialog(filename,"Je tekening wordt altijd opgeslagen in de ~/Documents/Doodle3D/doodles/ map.");
        if (result.bSuccess) {
             save(result.getName());
        }
        //ofSetFullscreen(true);
    }
    
    void save(string filename) {
        if (ofxGetFileExtension(filename)==".ply") {
            saveAsPLY(doodlesFolder+filename);
        } else if (ofxGetFileExtension(filename)==".stl") {
            saveAsSTL(doodlesFolder+filename);
        } else if (ofxGetFileExtension(filename)!=".svg") {
            ofLogError() << "incorrect filename for saveSvg:" << filename;
            return;
        } else {
            saveSvg(resourceFolder+"template.svg",doodlesFolder+filename);
        }
    }
    
    void saveAsText(string filename) {
        vector<string> lines;
        vector<ofSubPath> &subpaths = path.getSubPaths();
        for (int i=0; i<subpaths.size(); i++) {
            string line;
            vector<ofSubPath::Command> &commands = subpaths[i].getCommands();
            for (int j=0; j<commands.size(); j++) {
                ofPoint p = commands[j].to; // - ofPoint(bounds.x, bounds.y);
                p -= ini.get("loadOffset",ofPoint());
                line+=ofToString(p.x) + "," + ofToString(p.y) + " ";
            }
            lines.push_back(line);
        }
        string filePath = doodlesFolder + "/" + filename;
        ofxSaveStrings(filePath,lines); //will only save file in data/doodles/ folder
        
        listDir();
        
        cout << "saved: " << filePath << endl;
    }
    
    void saveSvg(string templateFilename, string outputFilename) { //absolute path is possible
        cout << templateFilename << "\n" << outputFilename << endl;
        
        ifstream f(ofToDataPath(templateFilename).c_str(),ios::in);
        stringstream buf;
        buf << f.rdbuf();
        string str = buf.str();
        f.close();
        
        //create svg path
        string strPath;
        vector<ofSubPath> &subpaths = path.getSubPaths();
        for (int i=0; i<subpaths.size(); i++) {
            string line;
            vector<ofSubPath::Command> &commands = subpaths[i].getCommands();
            for (int j=0; j<commands.size(); j++) {
                ofPoint p = commands[j].to; // - ofPoint(bounds.x, bounds.y);
                p -= ini.get("loadOffset",ofPoint());
                strPath += (j==0 ? "M":"L") + ofToString(p.x) + " " + ofToString(p.y) + " ";
            }
        }
        
        str = ofxReplaceString(str, "{PATH}", strPath);
        
        ofxSaveString(outputFilename, str);
    }
    
    void ofxInvertIndices(ofxMesh &m) {
        for (int i=0; i<m.getNumIndices()/2; i++) {
            ofIndexType &first = m.getIndices()[i];
            ofIndexType &last = m.getIndices()[m.getNumIndices()-1-i];
            swap(first,last);
        }
    }
    
    ofxMesh getMesh(ofPath path) {
        path.setFilled(true);
        
        vector<ofPolyline> outline = path.getOutline();
        ofxMesh *tess = (ofxMesh*) &path.getTessellation();
        ofxMesh top = *tess; //copy
        ofxMesh bottom = top; //copy
        ofxMesh sides;
        ofxMesh mesh;
        
        ofxInvertIndices(top);
        
        int thickness = 40;
        ofVec3f vecOffset(0,0,thickness);
        
        bottom.translate(vecOffset); //extrude
        
        //make side mesh by using outlines
        for (int j=0; j<outline.size(); j++) {
            
            int iMax = outline[j].getVertices().size();
            
            for (int i=0; i<iMax; i++) {
                
                ofVec3f a = outline[j].getVertices()[i];
                ofVec3f b = outline[j].getVertices()[i] + vecOffset;
                ofVec3f c = outline[j].getVertices()[(i+1) % iMax] + vecOffset;
                ofVec3f d = outline[j].getVertices()[(i+1) % iMax];
                
                sides.addFace(a,b,c,d);
            }
        }
        
        ofxInvertIndices(sides);
        
        mesh = ofxMesh(sides.addMesh(bottom.addMesh(top)));
        
        ofxInvertIndices(mesh);
        
        mesh.scale(.2,.2,-.2);
        mesh.rotate(180, ofVec3f(1,0,0));
        mesh.translate(-mesh.getCentroid());
        return mesh;
    }
    
    void saveAsPLY(string filename) {
        ofLogNotice() << "SaveAsPLY: " << filename;
        ofxMesh mesh = getMesh(path);
        mesh.save(filename);
    }
    
    void saveAsSTL(string filename) {
        ofLogNotice() << "SaveAsPLY: " << filename;
        ofxMesh mesh = getMesh(path);
        
        ofxInvertIndices(mesh);

        vector<ofVec3f>& vertices = mesh.getVertices();
        vector<ofIndexType>& indices = mesh.getIndices();
        vector<ofVec3f>& normals = mesh.getNormals();
        
        ofxSTLExporter stl;
        stl.beginModel();
        
        mesh.getIndices();
        for (int i=0; i < indices.size()-2; i+=3) {
            stl.addTriangle(vertices[indices[i+2]], vertices[indices[i+1]], vertices[indices[i]], normals[indices[i]]);
        }
        
        stl.useASCIIFormat(true);
        stl.saveModel(filename);
    }

    
    void loadFromSvg(string filename) {
        vector<string> lines = ofxLoadStrings(filename);
        string path;
        for (int i=0; i<lines.size(); i++) {
            int pos = lines[i].find(" d=\"");
            if (pos!=string::npos) {
                path = lines[i].substr(pos+4);
                pos = path.find("\"");
                path = path.substr(0,pos);
                break;
            }
        }
        if (path!="") {
            ofStringReplace(path, "M ", "M");
            ofStringReplace(path, "L ", "L");
            ofStringReplace(path, "M", "\n");
            ofStringReplace(path, " L", "_");
            ofStringReplace(path, " ", ",");
            ofStringReplace(path, "_", " ");
            loadFromStrings(ofSplitString(path, "\n"));
        }
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