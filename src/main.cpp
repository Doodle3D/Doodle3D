//openFrameworks & addons
#include "ofMain.h"
#include "ofxExtras.h"
#include "ofxIniSettings.h"
#include "ofxUltimaker.h"
#include "ofxHTTPServer.h"

//globals
ofPath path;
ofImage mask;
ofxIniSettings ini;
ofxUltimaker ultimaker;
ofxGCode gcode;
const int vres=1000;
float vfunc[vres];
float twists=0;
float objectHeight=10;
float layerHeight=.2;
float minScale=.8f;
float maxScale=1.2f;
float maxScaleDifference=.1f;
float maxObjectHeight=200.0f;
int targetTemperature=220;
bool debug=false;
bool useSubpathColors=false;
int serverPort=8888;
bool deviceAutoDetect=true;
int deviceSpeed=115200;
string deviceName;
bool connectionFailed=false;
float globalScale=1;
int simplifyIterations=10;
int simplifyMinNumPoints=15;
float simplifyMinDistance=5;
string resourceFolder;
string documentFolder;
string doodlesFolder;
string gcodeFolder;
map<string,string> params;
string statusMessage;

float scaleFunction(float f) {
    return vfunc[int(ofMap(f, 0, 1, vres-1, 0, true))];
}

//classes
#include "Btn.h"
#include "Canvas.h"
#include "Files.h"
Files files;

#include "Side.h"
#include "Thermometer.h"
#include "Printer.h"

class ofApp : public ofBaseApp, public ofxHTTPServerListener {
public:

    ofImage bg,bg_busy,vb,shapes;
    Btn btnNew,btnSave,btnOops,btnLoadPrevious,btnLoadNext,btnPrint,btnStop;
    Btn btnTwistLeft,btnTwistRight,btnZoomIn,btnZoomOut,btnHigher,btnLower;
    Canvas canvas;
    Side side;
    Thermometer thermometer;
    Printer printer;
    ofxHTTPServer *server;
    ofSerial tmp;
    string ipaddress,networkName;

    void setup() {        
        ofSetDataPathRoot("../Resources/");
        resourceFolder = ofFile("../Resources").getAbsolutePath() + "/";
        documentFolder = ofFilePath::getPathForDirectory("~/Documents/Doodle3D/");
        doodlesFolder = documentFolder + "doodles/";
        gcodeFolder = documentFolder + "gcode/";
        ofSetDataPathRoot(documentFolder);
        if (!ofDirectory(documentFolder).exists()) ofDirectory::createDirectory(documentFolder);
        if (!ofDirectory(doodlesFolder).exists()) ofDirectory::createDirectory(doodlesFolder);
        if (!ofDirectory(gcodeFolder).exists()) ofDirectory::createDirectory(gcodeFolder);

        ini.load(resourceFolder+"Doodle3D.ini");
        ini.load(documentFolder+"Doodle3D.ini");
        
        if (params["loadSettings"]!="") {
            string folder = ofFilePath::getPathForDirectory("~/Documents/Doodle3D/");
            ini.load(folder+params["loadSettings"]);
        }
        
        ofSetDataPathRoot(resourceFolder);
        
        deviceSpeed = ini.get("device.speed",deviceSpeed);
        deviceAutoDetect = ini.get("device.autoDetect",true);
        targetTemperature = ini.get("targetTemperature",targetTemperature);
        objectHeight = ini.get("objectHeight",objectHeight);
        maxObjectHeight = ini.get("maxObjectHeight",maxObjectHeight);
        layerHeight = ini.get("layerHeight",layerHeight);
        minScale = ini.get("minScale",minScale);
        maxScale = ini.get("maxScale",maxScale);
        side.setShape(ini.get("shape","|").at(0));
        maxScaleDifference = ini.get("maxScaleDifference",maxScaleDifference); //problems with resolution>layers. should be half of wallThickness or so.
        side.visible = ini.get("side.visible",side.visible);
        side.is3D = ini.get("side.is3D",side.is3D);
        side.bounds = ini.get("side.bounds",side.bounds);
        side.border = ini.get("side.border",side.border);
        side.vStep = ini.get("side.vStep",side.vStep);
        useSubpathColors = ini.get("useSubpathColors",useSubpathColors);
        printer.screenToMillimeterScale=ini.get("screenToMillimeterScale",printer.screenToMillimeterScale);
        printer.speed = ini.get("speed",printer.speed);
        printer.travelSpeed = ini.get("travelSpeed",printer.travelSpeed);
        printer.wallThickness = ini.get("wallThickness",printer.wallThickness);
        printer.zOffset = ini.get("zOffset",printer.zOffset);
        printer.useSubLayers = ini.get("useSubLayers",printer.useSubLayers);
        twists = ini.get("twists",twists);
        printer.filamentThickness = ini.get("filamentThickness",printer.filamentThickness);
        printer.loopAlways = ini.get("loopAlways",printer.loopAlways);
        thermometer.showWarmUp = ini.get("showWarmUp",thermometer.showWarmUp);
        serverPort = ini.get("server.port",serverPort);
        deviceName = ini.get("device.name",deviceName);
        simplifyIterations = ini.get("simplify.iterations",simplifyIterations);
        simplifyMinNumPoints = ini.get("simplify.minNumPoints",simplifyMinNumPoints);
        simplifyMinDistance = ini.get("simplify.minDistance",simplifyMinDistance);
        
        btnNew.setup(0xffff00);
        btnSave.setup(0x00ff00);
        btnOops.setup(0x006464);
        btnLoadPrevious.setup(0x804652);
        btnLoadNext.setup(0x7fb9ad);
        btnPrint.setup(4044666);
        btnStop.setup(0xe6098d);
        btnZoomIn.setup(0x500000);
        btnZoomOut.setup(0x640000);
        btnHigher.setup(0x780000);
        btnLower.setup(0x8c0000);
        btnTwistLeft.setup(0xa00000);
        btnTwistRight.setup(0xb40000);
        
        thermometer.setup();
        canvas.setup();
        bg.loadImage("bg.png");
        bg_busy.loadImage("bgbusy.png");
        mask.loadImage("mask.png");
        shapes.loadImage("shapes.png");
        ofEnableAlphaBlending();
        ofRectangle window = ini.get("window.bounds",ofRectangle(0,0,1280,800));
        ofxSetWindowRect(window);
        ofSetFullscreen(ini.get("window.fullscreen",true));
        if (window.width!=ofGetScreenWidth()) ofSetFullscreen(false);
        ofSetFrameRate(ini.get("frameRate", 30));
        ofSetEscapeQuitsApp(ini.get("quitOnEscape",true));
        ofEnableSmoothing();
        ofBackground(255);

        if (deviceAutoDetect) {
            deviceName = detectDeviceName();
        } else {
            deviceName = ini.get("device.name","");
        }
        
        if (deviceName!="") {
            string str = "connecting to " + deviceName + " @ " + ofToString(deviceSpeed) + "bps";
            cout << str << endl;
            if (!ultimaker.connect(deviceName,deviceSpeed)) {
                deviceName += ": not connected";
            }
        } else {
            deviceName = deviceAutoDetect ? "could not autoDetect device" : "device name is empty";
        }
        
        server = ofxHTTPServer::getServer(); // get the instance of the server
        server->setServerRoot(".");          // folder with files to be served
        server->setUploadDir("upload");		 // folder to save uploaded files
        server->setCallbackExtension("of");	 // extension of urls that aren't files but will generate a post or get event
        server->setListener(*this);
        server->start(serverPort);
        
        files.setup();
        
        if (ini.get("centerWindow",true)) {
            int x = ofGetScreenWidth()/2 - window.width/2;
            int y = ofGetScreenHeight()/2 - window.height/2;
            ofSetWindowPosition(x,y);
        }
        
        refreshDebugInfo();
    }
    
    string detectDeviceName() {
        ofSerial serial;
        vector<ofSerialDeviceInfo> devices = serial.getDeviceList();

        for (int i=0; i<devices.size(); i++){
            if (devices[i].getDeviceName().find("usbmodem")!=string::npos ||
                devices[i].getDeviceName().find("usbserial")!=string::npos) { //osx
                return devices[i].getDeviceName();
            }
        }
        return "";
    }
    
    void getRequest(ofxHTTPServerResponse & response) {
    }
    
    void postRequest(ofxHTTPServerResponse & response) {
        files.cur = -1;
        canvas.clear();
        
        string data = response.requestFields["data"];
        
        vector<string> items = ofSplitString(data,"\nBEGIN\n");
        items = ofSplitString(items[items.size()-1],"\n\nEND\n");

        if (items.size()>0) {
            vector<string> lines = ofSplitString(items[0],"\n");
            files.loadFromStrings(lines);
            files.save("doodle-"+ofxUrlToSafeLocalPath(ofxGetIsoDateTime())+".txt");
        }
    }

    void update() {
        if (connectionFailed) thermometer.visible = false;
            
        canvas.update();
        
        if (ini.get("autoWarmUp",true) && ofGetFrameNum()==100) {
            if (deviceSpeed==57600) {
                ultimaker.send("M104 S" + ofToString(targetTemperature));
            } else {
                ultimaker.send("M109 S" + ofToString(targetTemperature));
            }
        }

        if (deviceSpeed==57600 && !ultimaker.isBusy && ofGetFrameNum()>100 && ofGetFrameNum()%120==0) {
            ultimaker.readTemperature();
        }
        
        if (btnZoomIn.selected) canvas.zoom(1);
        if (btnZoomOut.selected) canvas.zoom(-1);

        if (btnHigher.selected) objectHeight = ofClamp(objectHeight+2, 3, maxObjectHeight);
        if (btnLower.selected) objectHeight = ofClamp(objectHeight-2, 3, maxObjectHeight);

        if (btnTwistLeft.selected) twists-=.01;
        if (btnTwistRight.selected) twists+=.01;
        
        if (btnOops.selected) {
            canvas.undo();
        }
    }

    void draw() {
        ofSetupScreenOrtho(0,0,OF_ORIENTATION_UNKNOWN,true,-200,200);
        ofScale(globalScale,globalScale);
        ofSetColor(255);
        
        if (connectionFailed) bg.draw(0,0);
        else if (ultimaker.temperature<targetTemperature ||
                 ultimaker.isBusy ||
                 !ultimaker.isStartTagFound ||
                 (ini.get("autoWarmUp",true) && ofGetFrameNum()<100)) {
             bg_busy.draw(0,0);
        } else {
             bg.draw(0,0);
        }
        
        canvas.draw();
        if (debug) canvas.drawDebug();
        side.draw();
        shapes.draw(side.bounds.x-15,side.bounds.y-40);
        thermometer.draw();
        ofSetColor(0);
        
        if (ofGetFrameNum()<10*30 && !ultimaker.isStartTagFound) {
            statusMessage = "Connecting to Ultimaker...";
        } else if (ofGetFrameNum()>10*30 && ofGetFrameNum()<20*30 && !ultimaker.isStartTagFound) {
            statusMessage = "Failed to connect. Please run Marlin firmware at 115200 bps";
            connectionFailed = true;
        }
            	
        if (ultimaker.isStartTagFound) statusMessage = "Connected";
        
        if (debug) {
            ofSetupScreen();
            float x=220*globalScale,y=180*globalScale;
            ofDrawBitmapString("status: " + statusMessage,x,y+=15);
            ofDrawBitmapString("deviceName: " + deviceName,x,y+=15);
            ofDrawBitmapString("deviceSpeed: " + ofToString(deviceSpeed),x,y+=15);
            ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate(),0),x,y+=15);
            ofDrawBitmapString("file: " + files.getFilename(),x,y+=15);
            ofDrawBitmapString("numVertices (3d view): " + ofToString(side.numVertices),x,y+=15);
            ofDrawBitmapString("WiFi network: " + networkName,x,y+=15);
            ofDrawBitmapString("ip-address: " + ipaddress,x,y+=15);
            ofDrawBitmapString("port: " + ofToString(serverPort),x,y+=15);
        }
    }

    void stop() {
        ultimaker.stopPrint();
        ultimaker.load(resourceFolder+"end.gcode");
        ultimaker.startPrint();
    }
    
    void print(bool exportOnly=false) {
        ofxSimplifyPath(path,simplifyIterations,simplifyMinNumPoints,simplifyMinDistance);
        if (ofGetFrameRate()<20) side.is3D=false;
        printer.print(resourceFolder+"start.gcode",resourceFolder+"end.gcode");
    }

    void mousePressed(int x, int y, int button) {
        x /= globalScale;
        y /= globalScale;
        
        canvas.mousePressed(x, y, button);
        side.mousePressed(x, y, button);
        if (btnNew.hitTest(x,y)) { files.cur=-1; canvas.clear(); files.unloadFile(); }
        if (btnSave.hitTest(x,y)) files.save();
        if (btnLoadPrevious.hitTest(x,y)) files.loadPrevious();
        if (btnLoadNext.hitTest(x,y)) files.loadNext();
        if (btnPrint.hitTest(x,y)) print();
        if (btnStop.hitTest(x,y)) stop();
        if (btnOops.hitTest(x,y)) { btnOops.selected=true; }
        if (btnZoomIn.hitTest(x,y)) btnZoomIn.selected=true;
        if (btnZoomOut.hitTest(x,y)) btnZoomOut.selected=true;
        if (btnHigher.hitTest(x,y)) btnHigher.selected=true;
        if (btnLower.hitTest(x,y)) btnLower.selected=true;
        if (btnTwistLeft.hitTest(x,y)) btnTwistLeft.selected=true;
        if (btnTwistRight.hitTest(x,y)) btnTwistRight.selected=true;
    }

    void mouseDragged(int x, int y, int button) {
        x /= globalScale;
        y /= globalScale;
        
        canvas.mouseDragged(x, y, button);
        side.mouseDragged(x, y, button);
    }

    void mouseReleased(int x, int y, int button) {
        x /= globalScale;
        y /= globalScale;
        
        ofxSimplifyPath(path,simplifyIterations,simplifyMinNumPoints,simplifyMinDistance);
        
        side.mouseReleased(x, y, button);
        canvas.mouseReleased(x, y, button);
        btnOops.selected=false;
        btnZoomIn.selected=false;
        btnZoomOut.selected=false;
        btnHigher.selected=false;
        btnLower.selected=false;
        btnTwistLeft.selected=false;
        btnTwistRight.selected=false;
    }

    void showHelp() {
        ofSystemAlertDialog("On your tablet connect to the '" + getWirelessNetwork() + "' network.\nIn the webbrowser open: http://" + getIP() + ":" + ofToString(serverPort));
    }
    
    string ofxExecute(string cmd) {
        string result;
        char line[130];
        FILE *fp = popen(cmd.c_str(), "r");
        while (fgets( line, sizeof line, fp)) result += line;
        pclose(fp);
        return ofxTrimString(result);
    }
    
    string getIP() {
        return ofxExecute("ifconfig en1 | grep 'inet ' | cut -d ' ' -f2");
    }

    string getWirelessNetwork() {
        return ofxExecute("networksetup -getairportnetwork en1 | cut -c 24-");
    }
    
    void refreshDebugInfo() {
        ipaddress = getIP();
        networkName = getWirelessNetwork();
    }
    
    void keyPressed(int key) {
        switch (key) {
            case '/': case '\\': case '$': case '#': case '|': case '%': case '@': case '^': case '&': case '_': side.setShape(key); break;
            case '3': side.is3D=!side.is3D; break;
            case '<': twists-=.5; break;
            case '>': twists+=.5; break;
            case '\'': twists=0; break;
            case '?': showHelp(); break;
            case 'a': side.toggle(); break;
            case 'b': useSubpathColors=!useSubpathColors; break;
            case 'C': canvas.createCircle(); break;
            case 'c': case 'n': canvas.clear(); files.unloadFile(); break;
            case 'd': debug=!debug; refreshDebugInfo(); break;
            case 'e': print(true); break;
            case 'f': ofToggleFullscreen(); break;
            case 'h': objectHeight+=5; if (objectHeight>maxObjectHeight) objectHeight=maxObjectHeight; break;
            case 'H': objectHeight-=5; if (objectHeight<3) objectHeight=3; break;
            case 'l': files.loadNext(); break;
            case 'L': files.loadPrevious(); break;
            case 'o': files.load(); break;
            case 'p': case 'm': case OF_KEY_RETURN: print(); break;
            case 'q': stop(); break;
            case 'r': ultimaker.setRelative(); break;
            case 'S': files.save(); break;
            case 's': files.saveAs(); break;
            case 't': ultimaker.readTemperature(); break;
            case 'u': case 'z': canvas.undo(); break;
            case '~': files.deleteCurrentFile(); break;
            case ' ': files.listDir(); break;
            case 'x': files.saveSvg(resourceFolder+"template.svg",documentFolder+"output.svg"); break;
        }
    }
    
    void windowResized(int w, int h) {
        globalScale = MIN(w/1280.0,h/800.0);
    }

};

int main(int argc, const char** argv){
    for (int i=0; i<argc; i++) {
        vector<string> keyvalue = ofSplitString(argv[i], "=");
        if (keyvalue.size()==2) params[keyvalue[0]] = keyvalue[1];
    }
    
    ofAppGlutWindow window;
    window.setGlutDisplayString("rgba double samples>=4");
    ofSetupOpenGL(&window, 1280, 800, OF_WINDOW);
    ofRunApp(new ofApp());
}
  