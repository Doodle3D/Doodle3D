#include "ofxExtras.h"
#include "ofxGCode.h"
#include "ofxUltimaker.h"
#include "ofxIniSettings.h"
//#include "/Users/rick/Documents/openFrameworks/of007-osx/libs/glut/lib/osx/GLUT.framework/Headers/glut.h"

#define useSerial true
#define btnField 63231
//#define btnMake 4044666
#define btnLoad 15010445
#define btnSave 0x19f672
#define btnUndo 0xa6b2d0
#define btnNieuw 0xfccf58
#define btnPrev 8537941
#define btnNext 8242102
#define btnPrint 4044666
#define btnStop 15075981
#define btnLeftVertical 0xff0900
#define btnRightVertical 0x003cff

class App : public ofBaseApp {
public:

    ofImage bg,bg_bezig,mask,thermomask,krul,kruis,opwarmen;
    ofxGCode gcode;
    ofxUltimaker ultimaker;
    ofPath path,vPath;
    ofDirectory dir;
    ofxIniSettings ini;
//    ofRectangle bounds,platform;
//    ofPoint shift;
    string gateway;
    map<int,int> func;

    //float temperature;
    bool debug;
    float screenToMillimeterScale;
    float scaleStep;
    float rotationStep;
    float translateStep;
    float feedrate;
    float travelrate;
    float flowrate;
    int layers;
    float layerHeight;
    float zOffset;
    float retract;
    int cur;
    string autoLoadImage;
    ofPoint offset;
    int desiredTemperature;
    bool isDrawing;
    ofPoint clickPoint;
    int clickAlpha,clickAlphaDecay;
    float minScale,maxScale;
    ofRectangle scaleBounds;
    bool doSpiral;
    bool isAdvanced;
    float spiralRadius;
    float useSubLayers;
    float circleDetail;
    bool useUltimaker;

    void setup() {
        cur=-1;
        isDrawing=false;
        clickAlpha=0;
        clickAlphaDecay=20;

        loadSettings();
        ofSetWindowPosition(0,0);
        //ofSetFullscreen(true);
        ofSetFrameRate(ini.get("frameRate", 30));
        bg.loadImage("images/bg.png");
        bg_bezig.loadImage("images/bg-bezig.png");
        mask.loadImage("images/mask.png"); //hitareas
        thermomask.loadImage("images/thermo-mask.png");
        krul.loadImage("images/krul.png");
        kruis.loadImage("images/kruis.png");
        opwarmen.loadImage("images/opwarmen.png");
        path.setFilled(false);
        path.setCurveResolution(100);
        path.setStrokeWidth(2);
        path.setStrokeColor(0);

        ofEnableSmoothing();
        ofEnableAlphaBlending();

        if (useUltimaker) {
            ultimaker.listDevices();
            if (ini.has("portnumber")) ultimaker.connect(ini.get("portnumber",0));
            if (ini.has("portname")) ultimaker.connect(ini.get("portname","/dev/ttyUSB0"),0);
        }

        listDir();

        if (autoLoadImage!="") {
            cout << "autoload: " << autoLoadImage << endl;
            for (cur=0; cur<dir.numFiles(); cur++) {
                //cout << "file: " << dir.getName(cur) << endl;
                if (dir.getName(cur)==autoLoadImage || dir.getPath(cur)==autoLoadImage) {
                    //cout << "found" << endl;
                    load(dir.getPath(cur));
                    break;
                }
            }
        }

        setVerticalFunc(ini.get("verticalFunc", "$").at(0));
    }

    void loadSettings() {
        ini.load("Doodle3D.ini");
        debug = ini.get("debug",false);
        isAdvanced = ini.get("isAdvanced",true);
        screenToMillimeterScale = ini.get("screenToMillimeterScale",0.3f);
        scaleStep = ini.get("scaleStep",0.0f);
        rotationStep = ini.get("rotationStep",0.0f);
        translateStep = ini.get("translateStep",1.0f);
        feedrate = ini.get("feedrate",4000);
        travelrate = ini.get("travelrate",25000);
        flowrate = ini.get("flowrate",.8f);
        layers = ini.get("layers",15);
        layerHeight = ini.get("layerHeight",.3f);
        zOffset = ini.get("zOffset",0.0f);
        doSpiral = ini.get("doSpiral",false);
        spiralRadius = ini.get("spiralRadius",20);
        minScale = ini.get("minScale",0.5f);
        maxScale = ini.get("maxScale",1.5f);
        retract = ini.get("retract",50); //not used
        autoLoadImage = ini.get("autoLoadImage","");
        scaleBounds = ini.get("scaleBounds",ofRectangle(890,200,150,418));
        offset = ini.get("offset",ofPoint(220,180));
        desiredTemperature = ini.get("desiredTemperature",220);
        useSubLayers = ini.get("useSubLayers",false);
        circleDetail = ini.get("circleDetail",60);
        useUltimaker = ini.get("useUltimaker",true);
    }

    void setVerticalFunc(char c) {
        int minX=scaleBounds.x;
        int maxX=scaleBounds.x+scaleBounds.width/2-20;
        int minY=scaleBounds.y;
        int maxY=scaleBounds.y+scaleBounds.height;

        for (int i=minY; i<maxY; i++) {
            float ii=ofNormalize(i, minY, maxY);
            if (c=='|') func[i]=ofxLerp(minX,maxX,.5);
            if (c=='\\') func[i]=ofMap(i,minY,maxY,minX,maxX);
            if (c=='/') func[i]=ofMap(i,minY,maxY,maxX,minX);
            if (c=='$') func[i]=ofMap(sin(ii*TWO_PI),-1,1,minX,maxX);
            if (c=='#') func[i]=ofMap(sin(ii*2*TWO_PI),-1,1,minX,maxX);
        }
    }

    void update() {
        ofPoint center = ofxGetCenterOfMass(ofxGetPointsFromPath(path));
        if (ofGetKeyPressed('-')) { path.translate(-center); path.scale(.97,.97); path.translate(center); }
        if (ofGetKeyPressed('=')) { path.translate(-center); path.scale(1/.97,1/.97); path.translate(center); }
        if (ofGetKeyPressed('[')) { path.translate(-center); path.rotate(-1,ofVec3f(0,0,1)); path.translate(center); }
        if (ofGetKeyPressed(']')) { path.translate(-center); path.rotate(1,ofVec3f(0,0,1)); path.translate(center); }
        if (ofGetKeyPressed(OF_KEY_LEFT)) path.translate(ofPoint(-translateStep,0));
        if (ofGetKeyPressed(OF_KEY_RIGHT)) path.translate(ofPoint(translateStep,0));
        if (ofGetKeyPressed(OF_KEY_UP)) path.translate(ofPoint(0,-translateStep));
        if (ofGetKeyPressed(OF_KEY_DOWN)) path.translate(ofPoint(0,translateStep));

        if (useUltimaker && ofGetFrameNum()==200) {
            cout << "get temperature at frame 200" << endl;
            ultimaker.readTemperature();
        }
    }

    void listDir() {
        dir.reset();
        dir.listDir("doodles/");
    }

    void draw() {
        ofSetColor(255);
        //if (ultimaker.isPrinting) bg_bezig.draw(0,0); else bg.draw(0,0);
        ofFill();
        ofSetColor(255,0,0);
        ofRect(1110,531+ofMap(ultimaker.temperature,0,240,230,0,true),127,240);
        ofSetColor(255);
        //thermomask.draw(0,0);

//        if (ultimaker.temperature<desiredTemperature-3) {
//            kruis.draw(0,0);
//            if (ultimaker.isPrinting) opwarmen.draw(0,0);
//        } else {
//            krul.draw(0,0);
//        }

        vector<ofSubPath> &subpaths = path.getSubPaths();
        if (subpaths.size()>0) path.draw(0,0);
        if (subpaths.size()>1) {
            ofSetColor(200);
            for (int i=0; i<subpaths.size()-1; i++) {
                vector<ofSubPath::Command> &left = subpaths[i].getCommands();
                vector<ofSubPath::Command> &right = subpaths[i+1].getCommands();
                ofLine(left.back().to,right.front().to);
            }
        }

        if (debug) {
            vector<ofPoint*> points = ofxGetPointsFromPath(path);
            ofSetColor(255,0,0);
            ofNoFill();
            ofRect(ofxGetBoundingBox(points));
            ofFill();
            ofCircle(ofxGetCenterOfMass(points),3);
            ofSetColor(0,255,0);
            ofCircle(ofxGetBoundingBox(points).getCenter(),3);

            ofSetColor(0);
            ofDrawBitmapString("file:" + ofToString(cur) + "/" + ofToString(dir.numFiles()),15,15);
            ofDrawBitmapString(ofToString(ultimaker.temperature),1110+50,531+200);

            ofSetColor(255,50);
            mask.draw(0,0);
        }

        //vertical
        if (isAdvanced) {
            int d=876;
            int dd=80;
            ofSetColor(0);
            ofLine(d,168,d,640);
            ofLine(d+1,168,d+1,640);
            ofSetColor(0,0,0);
            ofNoFill();
            ofSetLineWidth(2);

            glBegin(GL_LINE_STRIP);
            for (map<int,int>::iterator it=func.begin(); it!=func.end(); it++) {
                int y = it->first;
                int x = func[y];
                glVertex2f(x,y);
            }
            glEnd();

            glBegin(GL_LINE_STRIP);
            for (map<int,int>::iterator it=func.begin(); it!=func.end(); it++) {
                int y = it->first;
                int x = func[y];
                glVertex2f(960-(x-960),y);
            }
            glEnd();
        }

        //click feedback
        ofSetLineWidth(1);
        ofSetColor(255,clickAlpha);
        ofFill();
        ofCircle(clickPoint,clickAlpha/6);
        if (clickAlpha>0) clickAlpha-=clickAlphaDecay;
    }

    float scaleFunction(float f) {
        if (func.size()==0) return 0;

        int y = ofxLerp(scaleBounds.y,scaleBounds.y+scaleBounds.height,f);
        int x = func.find(y)->second;

        return ofMap(x, scaleBounds.x, scaleBounds.x+scaleBounds.width,minScale,maxScale,true);
    }

    void make() {
        cout << "make: frame=" << ofGetFrameNum() << endl;

        ofPath tmpPath = path;

        float extruder = 0;

        gcode.lines.clear();
        gcode.insert("gcode/start.gcode");

        vector<ofSubPath> &subpaths = path.getSubPaths();

        vector<ofPoint*> points = ofxGetPointsFromPath(path);
        if (points.size()<2) return;

        bool isLoop = points.front()->distance(*points.back())<10;

        for (int layer=0; layer<layers; layer++) {

            //reset
            path = tmpPath;

            points = ofxGetPointsFromPath(path);

            //scale to machine dimensions
            path.translate(-offset); //left corner of drawing field in pixels
            path.translate(-ofxGetCenterOfMass(points));
            path.scale(screenToMillimeterScale,-screenToMillimeterScale);
//            path.translate(ofxGetCenterOfMass(points));

            float layerScale = scaleFunction(float(layer)/layers);

//            if (layer%25==0) {
//                gcode.addCommandWithParams("M104 S%d",desiredTemperature);
//            }

            float angle=float(layer)/layers*TWO_PI;

//            path.translate(-ofxGetCenterOfMass(points));
            path.scale(layerScale,layerScale); //1+layer*scaleStep, 1+layer*scaleStep);
            if (doSpiral) path.translate(ofPoint(spiralRadius*sin(angle),spiralRadius*cos(angle)));
            path.rotate(layer*rotationStep,ofVec3f(0,0,1));
//            path.translate(ofxGetCenterOfMass(points));

            bool even = (layer%2==0);

            for (int j=0; j<subpaths.size(); j++) {

                vector<ofSubPath::Command> &commands = subpaths[even ? j : subpaths.size()-1-j].getCommands();

                for (int i=0; i<commands.size(); i++) {
                    int last = commands.size()-1;

                    ofPoint from,to;

                    if (isLoop) even = true; //overrule, don't go backwards

                    if (isLoop && i==last) continue; //prevent double action

                    if (even) {
                        to = commands[i].to; //deze
                        from = commands[i>0?i-1:0].to; //vorige
                    } else {
                        to = commands[last-i].to; //deze achterwaarts
                        from = commands[i>0?last-i+1:last].to; //volgende
                    }

                    extruder += flowrate * from.distance(to);

                    float sublayer = layer + (useSubLayers ? float(i)/commands.size() : 0);

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
        print();

        path = tmpPath;
    }

    void print() {
        if (ofGetFrameNum()<210 || ultimaker.isBusy) return;
        ultimaker.load("gcode/output.gcode");
        ultimaker.startPrint();
    }

    void loadPrevious() {
        cur = (cur-1+dir.numFiles()) % dir.numFiles();
        load(dir.getPath(cur));
    }

    void loadNext() {
        cur = (cur+1+dir.numFiles()) % dir.numFiles();
        load(dir.getPath(cur));
    }

    void deleteCurrentFile() {
        ofFile f(dir.getPath(cur));
        f.remove();
        listDir();
        load(dir.getPath(cur)); //reload the thing on place cur
    }

    void load() {
        ofFileDialogResult result = ofSystemLoadDialog();
        if (result.bSuccess) load(result.getPath());
    }

    void load(string filename) {
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
                if (j==0) path.moveTo(p.x,p.y);
                else path.lineTo(p.x,p.y);
            }
        }

        //temp
        vector<ofPoint*> points = ofxGetPointsFromPath(path);
        if (points.size()<2) return;
        bool isLoop = points.front()->distance(*points.back())<10;
        cout << filename << ", loop=" << isLoop << endl;

        ofxSimplifyPath(path);
    }

    void save() {
        if (cur>-1 && cur<dir.numFiles()) {
            save(dir.getName(cur));
        } else {
            saveAs();
        }
    }

    void saveAs() {
        ofFileDialogResult result = ofSystemSaveDialog("doodle.txt","Je tekening wordt altijd opgeslagen in de doodles map.");
        if (result.bSuccess) save(result.getName());
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

    void undo() {
        vector<ofSubPath> &subpaths = path.getSubPaths();
        if (subpaths.size()<1) return;
        vector<ofSubPath::Command> &commands = subpaths.back().getCommands();
        if (commands.size()<=1) {
            subpaths.erase(subpaths.end());
            return;
        }
        commands.erase(commands.end());
        if (commands.size()<=1) {
            subpaths.erase(subpaths.end());
        }
        path.flagShapeChanged();
    }

    void clear() {
        path.clear();
        path.flagShapeChanged();
    }

    void stop() {
        ultimaker.stopPrint();
        //ultimaker.load("end.gcode");
        //ultimaker.startPrint(); //start only end gcode
    }

    void mousePressed(int x, int y, int button) {
        clickPoint.set(x,y);
        clickAlpha=150;

        bool hitLeft = mask.getColor(x,y).getHex()==btnLeftVertical;
        bool hitRight = mask.getColor(x,y).getHex()==btnRightVertical;
        if (isAdvanced && (hitLeft || hitRight)) func.clear();

        switch (mask.getColor(x,y).getHex()) {
            case btnField: path.moveTo(x,y); isDrawing=true; break;
            case btnPrint: make(); break;
            case btnLoad: loadNext(); break;
            case btnSave: saveAs(); break;
            case btnUndo: undo(); break;
            case btnNieuw: clear(); break;
            case btnPrev: loadPrevious(); break;
            case btnNext: loadNext(); break;
            case btnStop: stop(); break;
            default: break;
        }

    }

    void mouseMoved(int x, int y) {

    }

    void mouseDragged(int x, int y, int button) {
        bool hitField = mask.getColor(x,y).getHex()==btnField;
        bool hitLeft = mask.getColor(x,y).getHex()==btnLeftVertical;
        bool hitRight = mask.getColor(x,y).getHex()==btnRightVertical;
        bool hasMouseMoved = ofGetMouseX()!=ofGetPreviousMouseX() || ofGetMouseY()!=ofGetPreviousMouseY();
        bool hitLargeField = !isAdvanced && (hitLeft || hitRight);

        if (hasMouseMoved && (hitField || hitLargeField)) {
            if (isDrawing) path.lineTo(x,y);
        }

        if (isAdvanced && (hitLeft || hitRight)) {

            int x = ofGetMouseX();
            int y = ofGetMouseY();
            int py = ofGetPreviousMouseY();

            if (hitRight) x=960-(x-960);
            if (x>950) x=950;

            if (!scaleBounds.inside(x,y)) return;
            if (!scaleBounds.inside(x,py)) return;

            float minY = min(y,py);
            float maxY = max(y,py);

            func[ofGetMouseY()] = x;

            if (fabs(minY-maxY) < FLT_EPSILON) maxY=minY+1; //prevent /0

            for (int i=minY; i<=maxY; i++) {
                func[i] = ofMap(i, minY, maxY, func[minY], func[maxY]);
                func[i] = ofClamp(func[i],scaleBounds.x, scaleBounds.x+scaleBounds.width);
            }
        }
    }

    void mouseReleased(int x, int y, int button) {
        ofxSimplifyPath(path);

        //smooth vertical func
        if (isAdvanced) {
            for (int i=0; i<20; i++) {
                int px=func.begin()->second;
                for (map<int,int>::iterator it=func.begin(); it!=func.end(); it++) {
                    int y = it->first;
                    int x = it->second;
                    if (abs(px-x)>1) x=(x+px)/2;
                    func[y] = x;
                    px=x;
                }
            }
        }
        isDrawing=false;
    }

    void createCircle() {
        for (float i=0,n=circleDetail; i<=n; i++) {
            float ii=float(i)/n;
            float x=100*sin(ii*TWO_PI)+500;
            float y=100*cos(ii*TWO_PI)+400;
            if (i==0) path.moveTo(x,y); else path.lineTo(x,y);
        }
//        path.setArcResolution(64);
//        path.arc(400, 400, 100, 100, 0, 360);
//        path.flagShapeChanged();
    }

    void keyPressed(int key) {
        switch (key) {
            case 'f': ofToggleFullscreen(); break;
            case 'm': make(); break;
            case OF_KEY_RETURN: case 'p': make(); break;
            case 'M': ultimaker.stopPrint(); break;
            case 'l': loadNext(); break;
            case 'L': loadPrevious(); break;
            case '!': deleteCurrentFile(); break;
            case 'd': debug=!debug; break;
            case 's': saveAs(); break;
            case 'S': save(); break;
            case 'u': undo(); break;
            case 'c': case 'n': clear(); break;
            case 'h': ultimaker.physicalHomeXYZ(); break;
            case 'T': ultimaker.setTemperature(220); break;
            case 't': ultimaker.readTemperature(); break;
            case 'r': ultimaker.setRelative(); break;
            case 'e': ultimaker.extrude(260,1000); break;
            case 'q': stop(); break;
            case 'R': rotationStep=!rotationStep; break;
            case 'K': if (scaleStep==0) scaleStep=-0.005; else scaleStep=0; break;
            case 'b': ultimaker.isPrinting = !ultimaker.isPrinting; break;
            case 'a': isAdvanced=!isAdvanced; break;
            case 'C': createCircle(); break;
            case '*': loadSettings(); break;
            case '/':
            case '\\':
            case '$':
            case '#':
            case '|': setVerticalFunc(key); break;
        }
    }



}; //App

int main() {
    ofAppGlutWindow window;
    //window.setGlutDisplayString("rgba double samples>=4");
    ofSetupOpenGL(&window, 1280, 800, OF_WINDOW);
    ofRunApp(new App());
}


