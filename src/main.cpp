//openFrameworks & addons
#include "ofMain.h"
#include "ofxExtras.h"
#include "ofxIniSettings.h"
#include "ofxUltimaker.h"
#include "ofxGCode.h"
#ifdef TARGET_OSX
#include "ofxHTTPServer.h"
#endif

//globals
ofPath path;
ofImage mask;
ofxIniSettings ini;
ofxGCode gcode;
const int vres=1000;
float vfunc[vres];
char previewShape=0;
string shapeString = "#_&|/%^$\\";
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
//bool deviceAutoDetect=true;
//int deviceSpeed=115200;
//string deviceName;
bool connectionFailed=false;
float globalScale=1;
int simplifyIterations=10;
int simplifyMinNumPoints=15;
float simplifyMinDistance=5;
string resourceFolder;
string documentFolder;
string doodlesFolder;
string gcodeFolder;
string imagesFolder;
map<string,string> params;
string statusMessage;
ofRectangle shapeButtons(888,118,147,45);
int fps=30;
bool autoWarmUp=true;
bool autoWarmUpRequested=false;
string autoWarmUpCommand="M104 S230";
int autoWarmUpDelay=3;
int checkTemperatureInterval=1;
ofPoint btnHelpPos(1250,780);

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

#ifdef TARGET_OSX
class ofApp : public ofBaseApp, public ofxHTTPServerListener {
#else
class ofApp : public ofBaseApp {
#endif

public:

    ofxUltimaker ultimaker;
    ofImage bg,bg_busy,vb,shapes;
    Btn btnNew,btnSave,btnOops,btnLoadPrevious,btnLoadNext,btnPrint,btnStop;
    Btn btnTwistLeft,btnTwistRight,btnZoomIn,btnZoomOut,btnHigher,btnLower;
    ofImage btnHelp;
    Canvas canvas;
    Side side;
    Thermometer thermometer;
    Printer printer;
    #ifdef TARGET_OSX
    ofxHTTPServer *server;
    #endif
    ofSerial tmp;
    string ipaddress,networkName;
    ofTrueTypeFont font;
    string version;

    void setup() {

        #ifdef TARGET_OSX
        ofSetDataPathRoot("../Resources/");
        resourceFolder = ofFile("../Resources").getAbsolutePath() + "/";
        #else
        resourceFolder = ""; //ofFilePath::getCurrentWorkingDirectory()+"data\\";
        #endif

        documentFolder = ofFilePath::getUserHomeDir() + "/Documents/Doodle3D/";
        //ofFilePath::getPathForDirectory((string)getenv("HOME")+"/Documents/Doodle3D/"); //or ofFilePath::getUserHomeDir()

        cout << "resourceFolder: " << resourceFolder << endl;
        cout << "documentFolder: " << documentFolder << endl;

        doodlesFolder = documentFolder + "doodles/";
        gcodeFolder = documentFolder + "gcode/";
        imagesFolder = resourceFolder+"images/";

        if (!ofDirectory(documentFolder).exists()) ofDirectory::createDirectory(documentFolder);
        if (!ofDirectory(doodlesFolder).exists()) ofDirectory::createDirectory(doodlesFolder);
        if (!ofDirectory(gcodeFolder).exists()) ofDirectory::createDirectory(gcodeFolder);

        ini.load(resourceFolder+"Doodle3D.ini");
        ini.load(documentFolder+"Doodle3D.ini");

        if (params["loadSettings"]!="") {
            cout << "loadSettings: " << (documentFolder+params["loadSettings"]) << endl;
            ini.load(documentFolder+params["loadSettings"]);
        }

        fps = ini.get("frameRate",fps);
//        deviceSpeed = ini.get("device.speed",deviceSpeed);
//        deviceAutoDetect = ini.get("device.autoDetect",true);
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
        printer.hop = ini.get("hop",printer.hop);
        printer.screenToMillimeterScale=ini.get("screenToMillimeterScale",printer.screenToMillimeterScale);
        printer.speed = ini.get("speed",printer.speed);
        printer.travelSpeed = ini.get("travelSpeed",printer.travelSpeed);
        printer.wallThickness = ini.get("wallThickness",printer.wallThickness);
        printer.zOffset = ini.get("zOffset",printer.zOffset);
        printer.useSubLayers = ini.get("useSubLayers",printer.useSubLayers);
        printer.minimalDistanceForRetraction = ini.get("retraction.minDistance",1);
        printer.retraction = ini.get("retraction.amount",0);
        printer.speed = ini.get("retraction.speed",0);
        twists = ini.get("twists",twists);
        printer.filamentThickness = ini.get("filamentThickness",printer.filamentThickness);
        printer.loopAlways = ini.get("loopAlways",printer.loopAlways);
        autoWarmUp = ini.get("autoWarmUp",true);
        autoWarmUpCommand = ini.get("autoWarmUpCommand",autoWarmUpCommand);
        autoWarmUpDelay = ini.get("autoWarmUpDelay",autoWarmUpDelay)*fps; //multiplied by fps
        serverPort = ini.get("server.port",serverPort);
//        deviceName = ini.get("device.name",deviceName);
        simplifyIterations = ini.get("simplify.iterations",simplifyIterations);
        simplifyMinNumPoints = ini.get("simplify.minNumPoints",simplifyMinNumPoints);
        simplifyMinDistance = ini.get("simplify.minDistance",simplifyMinDistance);
        checkTemperatureInterval = ini.get("checkTemperatureInterval",checkTemperatureInterval);

        btnHelp.loadImage("images/btnInfo.png");
        btnHelp.setAnchorPercent(.5,.5);
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
        bg.loadImage(imagesFolder+"bg.png");
        bg_busy.loadImage(imagesFolder+"bgbusy.png");
        mask.loadImage(imagesFolder+"mask.png");
        shapes.loadImage(imagesFolder+"shapes.png");
        ofEnableAlphaBlending();
        ofRectangle window = ini.get("window.bounds",ofRectangle(0,0,1280,800));
        ofxSetWindowRect(window);
        ofSetLogLevel((ofLogLevel)ini.get("loglevel",2));
        ofSetFullscreen(ini.get("window.fullscreen",true));
        font.loadFont("Abel-Regular.ttf", 14, true, false, true);

        ofSetFrameRate(fps);
        ofSetEscapeQuitsApp(ini.get("quitOnEscape",true));
        ofEnableSmoothing();
        ofBackground(255);
                
        
        #ifdef TARGET_OSX
        server = ofxHTTPServer::getServer(); // get the instance of the server
        server->setServerRoot("www");          // folder with files to be served
        server->setUploadDir("upload");		 // folder to save uploaded files
        server->setCallbackExtension("of");	 // extension of urls that aren't files but will generate a post or get event
        server->setListener(*this);
        server->start(serverPort);
        #endif

        files.setup();

        if (ini.get("window.center",true)) {
            int x = ofGetScreenWidth()/2 - window.width/2;
            int y = ofGetScreenHeight()/2 - window.height/2;
            ofSetWindowPosition(x,y);
        }

        refreshDebugInfo();

//        cout << "deviceName: " << deviceName << endl;

        //        vector<string> lines = ofxLoadStrings(resourceFolder+"VERSION");
        //        if (lines.size()>0) version = lines[0];
        
        
        version = getVersion();

        
        ultimaker.setup();
    }

    #ifdef TARGET_OSX
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
            files.saveSvg(resourceFolder+"template.svg", doodlesFolder+"/doodle-"+ofxUrlToSafeLocalPath(ofxGetIsoDateTime())+".svg");
        }
    }
    #endif

    void update() {
        thermometer.temperature = ultimaker.temperature;

        //send autoWarmUp command
        if (!autoWarmUpRequested && autoWarmUp && ultimaker.isStartTagFound && autoWarmUpDelay--<0) {
            ultimaker.sendCommand(autoWarmUpCommand);
            autoWarmUpRequested = true;
        }

        //check temperature
        if (ultimaker.isStartTagFound && checkTemperatureInterval!=0 && ofGetFrameNum() % (checkTemperatureInterval*fps)==0) {
            ultimaker.sendCommand("M105",1);
        }

        canvas.update();

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

        bg.draw(0,0);
        
        //ofSetColor(0);
        //ofDrawBitmapString("?",ofGetWidth()-16,ofGetHeight()-16);

        ofSetColor(255);
        canvas.draw();
        if (debug) canvas.drawDebug();
        side.draw();
        shapes.draw(shapeButtons.x,shapeButtons.y);
        if (ultimaker.isStartTagFound) thermometer.draw();
        ofSetColor(0);

        ofSetColor(255);
        btnHelp.draw(btnHelpPos);
        
        if (debug) drawConsole();
    }

    void drawConsole() {
        //font.drawStringAsShapes
        #define console(s) ofDrawBitmapString(string(s).substr(0,MIN(string(s).size(),75)),x,y+=h);
        float x=220, y=175, h=17;
        console("Doodle3D " + version + " Copyright (c) 2012-2013 by Rick Companje");
        console("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
        string connectionStatus = ultimaker.isConnectedToPort ? "Connected to port" : "NOT connected to port";
        string startTagStatus = ultimaker.isStartTagFound ? "The Marlin 'start' tag was found (this is good)" : "Start tag not found. Please check firmware or connection speed";
        console(connectionStatus + " " + ultimaker.deviceName + " @ " + ofToString(ultimaker.deviceSpeed) + "bps");
        console(startTagStatus);
        if (ultimaker.temperature!=0) console("Temperature: " + ofToString(ultimaker.temperature));
        console("Framerate: " + ofToString(ofGetFrameRate(),0));
        console("iPad version: http://" + ipaddress + ":" + ofToString(serverPort) + " on WiFi network: " + networkName);
        console("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
//        deque<string> msg = ultimaker.messages;
//        for (int i=MAX(0,msg.size()-19); i<msg.size(); i++) {
//            console(msg.at(i));
//        }
        #undef console
    }

    void stop() {
        cout << "test " << resourceFolder+"gcode/end.gcode" << endl;
        ultimaker.sendCommandsFromFile(resourceFolder+"gcode/end.gcode",true);
    }

    void print(bool exportOnly=false) {
        ofxSimplifyPath(path,simplifyIterations,simplifyMinNumPoints,simplifyMinDistance);

        string hourMinutes = ofxFormatDateTime(ofxGetDateTime(),"%H.%M");
        string outputFilename = gcodeFolder+files.getFilename()+"_"+hourMinutes+".gcode";
        printer.print(outputFilename, resourceFolder+"gcode/start.gcode",resourceFolder+"gcode/end.gcode");

        if (exportOnly) return;

        ultimaker.sendCommandsFromFile(outputFilename);
    }

    void mousePressed(int x, int y, int button) {
        x /= globalScale;
        y /= globalScale;

        canvas.mousePressed(x, y, button);
        side.mousePressed(x, y, button);
//        cout << btnHelpPos.distance(ofPoint(x,y)) << endl;
        if (btnHelpPos.distance(ofPoint(x,y))<btnHelp.width/2) showHelp();
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

        if (shapeButtons.inside(x,y)) {
            int index = ofNormalize(x,shapeButtons.x,shapeButtons.x+shapeButtons.width) * shapeString.size();
            side.setShape(shapeString.at(index));
        }
    }

    void mouseDragged(int x, int y, int button) {
        x /= globalScale;
        y /= globalScale;

        canvas.mouseDragged(x, y, button);
        side.mouseDragged(x, y, button);
    }


    void mouseMoved(int x, int y) {
        x /= globalScale;
        y /= globalScale;

        if (shapeButtons.inside(x,y)) {
            previewShape = shapeString.at(ofNormalize(x,shapeButtons.x,shapeButtons.x+shapeButtons.width) * shapeString.size());
        } else {
            previewShape = 0;
        }
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
        ofSystemAlertDialog("Doodle3D version " + getVersion() + " - www.doodle3d.com\n\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n"
                            "Did you know you can use Doodle3D on your iPad?\n\n"
                            "1. Connect with your tablet to the '" + getWirelessNetwork() + "' network.\n\n"
                            "2. On your tablet open this webpage:\n\n"
                            "http://" + getIP() + ":" + ofToString(serverPort));
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
        #ifdef TARGET_OSX
        string ip = ofxExecute("ifconfig en1 | grep 'inet ' | cut -d ' ' -f2");
        if (ip=="") return "[unknown]";
        else return ip;
        #else
        return "[unknown]";
        #endif
    }
    
    string getVersion() {
        #ifdef TARGET_OSX
        return ofxExecute("defaults read " + resourceFolder + "../Info CFBundleShortVersionString");
        #else
        return "[unknown]"
        #endif
    }

    string getWirelessNetwork() {
        #ifdef TARGET_OSX
        string network = ofxExecute("networksetup -getairportnetwork en1"); // | cut -c 24-");
        if (network=="You are not associated with an AirPort network.") return "[unknown]";
        else return network.substr(23);
        #else
        return "[unknown]";
        #endif
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
            case 'd': case 'i': debug=!debug; refreshDebugInfo(); break;
            case 'e': print(true); break;
            case 'f': ofToggleFullscreen(); break;
            case 'h': objectHeight+=5; if (objectHeight>maxObjectHeight) objectHeight=maxObjectHeight; break;
            case 'H': objectHeight-=5; if (objectHeight<3) objectHeight=3; break;
            case 'G': ultimaker.sendCommand("G28 X0 Y0 Z0\nM84",2); break;
            case 'A': ultimaker.sendCommand("M84"); break;
            case 'T': ultimaker.sendCommand("M109 S230"); break;
            case 'l': files.loadNext(); break;
            case 'L': files.loadPrevious(); break;
            case 'o': files.load(); break;
            case 'p': case 'm': case OF_KEY_RETURN: print(); break;
            case 'q': stop(); break;
//            case 'r': ultimaker.setRelative(); break;
            case 'S': files.save(); break;
            case 's': files.saveAs(); break;
            case 't': ultimaker.sendCommand("M105",1); break;
            case 'u': case 'z': canvas.undo(); break;
            case '~': files.deleteCurrentFile(); break;
            case ' ': files.listDir(); break;
            case 'x': files.saveSvg(resourceFolder+"template.svg",documentFolder+"output.svg"); break;
            case 27: if (ultimaker.isThreadRunning()) ultimaker.stopThread(); break;
        }
    }

    void windowResized(int w, int h) {
        globalScale = MIN(w/1280.0,h/800.0);
    }

};

int main(int argc, const char** argv){
    for (int i=0; i<argc; i++) {
        vector<string> keyvalue = ofSplitString(argv[i], "=");
        if (keyvalue.size()==2) {
            params[keyvalue[0]] = keyvalue[1];
            cout << "param: " << keyvalue[0] << "=" << keyvalue[1] << endl;
        }
    }

    ofAppGlutWindow window;
    #ifdef TARGET_OSX
    window.setGlutDisplayString("rgba double samples>=4");
    #endif
    ofSetupOpenGL(&window, 1280, 800, OF_WINDOW);
    ofRunApp(new ofApp());
}
