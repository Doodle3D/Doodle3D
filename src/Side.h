#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Side {
public:
    
    ofRectangle bounds;
    ofRectangle border;
    bool visible;
    bool is3D;
    bool isDrawing;
    float vStep;
    int numVertices;
    
    Side() {
        visible=true;
        is3D=true;
        isDrawing=false;
        bounds = ofRectangle(900,210,131,390);
        border = ofRectangle(880,170,2,470);
        vStep = 1;
    }
    
    void draw() {
        if (!visible) return;
        
        //resolution of rendering of vertical shape
        if (numVertices>10000) vStep = ofMap(numVertices,10000,100000,1,10,true);
        else vStep=1;
//        else if (ofGetFrameRate()<20) vStep = 10;
//        else vStep = 1;
        
        ofPushStyle();
        ofPushMatrix();
        
        ofSetColor(0);
        ofRect(border);
        
        ofTranslate(bounds.x,bounds.y);
        ofTranslate(bounds.width/2,bounds.height);

        ofPath vpath = path;
        vector<ofPoint*> points = ofxGetPointsFromPath(vpath);
        
        numVertices = points.size() * objectHeight;
        
        ofRectangle pathBounds = ofxGetBoundingBox(points);
        ofPoint center = pathBounds.getCenter();
        vpath.translate(-center);
        float s = min(bounds.width/pathBounds.height,bounds.width/pathBounds.width);
        vpath.scale(s,s);
        
        if (points.size()>1) {
            if (is3D) {
                ofPushMatrix();
                
                ofSetLineWidth(1*globalScale);
                vpath.setStrokeWidth(1*globalScale);
                vpath.setStrokeColor(ofColor(0,0,0,60));
                
                //draw path
                for (float i=0,n=bounds.height; i<n; i+=vStep) {
                    float ii=float(i)/n;
                    ofPushMatrix();
                    ofTranslate(0,-i);
                    float s = scaleFunction(ii);
                    ofPath spath = vpath;
                    spath.scale(s,s);
                    spath.rotate(ii*twists*360, ofVec3f(0,0,1));
                    ofRotateX(60);
                    //spath.draw(0,0);
                    
                    vector<ofSubPath> &subpaths = spath.getSubPaths();
                    for (int i=0; i<subpaths.size(); i++) {
                        vector<ofSubPath::Command> &commands = subpaths[int(i)].getCommands();
                        ofNoFill();
                        ofSetLineWidth(1*globalScale);
                        if (useSubpathColors) {
                            ofSetColor(ofColor::fromHsb(float(i)/subpaths.size()*255, 255, 255, 60));
                        } else {
                            ofSetColor(255,0,0,60);
                        }
                        ofBeginShape();
                        for (int j=0; j<commands.size(); j++) {
                            ofVertex(commands[j].to.x, commands[j].to.y);
                        }
                        ofEndShape();
                    }   
                    
                    //draw lines between subpaths
                    //vector<ofSubPath> &subpaths = spath.getSubPaths();
                    if (subpaths.size()>1) {
                        ofSetColor(50,60);
                        for (int i=0; i<subpaths.size()-1; i+=2) {
                            vector<ofSubPath::Command> &left = subpaths[i].getCommands();
                            vector<ofSubPath::Command> &right = subpaths[i+1].getCommands();
                            ofLine(left.back().to,right.front().to);
                        }
                    }

                    ofPopMatrix();
                    
                    if (ii>objectHeight/maxObjectHeight) break;
                }
                ofPopMatrix(); //translate z=-...
                
            } else {
                //
            }
            
            ofSetLineWidth(2*globalScale);
            ofSetColor(0);
            
            glBegin(GL_LINE_STRIP);
            for (int i=0; i<vres; i++) {
                float f=float(i)/vres;
                int x=s*pathBounds.width/2*scaleFunction(f);
                int y=ofMap(i,0,vres,0,bounds.height);
                glVertex2f(x,-y);
            }
            glEnd();
            
            glBegin(GL_LINE_STRIP);
            for (int i=0; i<vres; i++) {
                float f=float(i)/vres;
                int x=s*pathBounds.width/2*scaleFunction(f);
                int y=ofMap(i,0,vres,0,bounds.height);
                glVertex2f(-x,-y);
            }
            glEnd();
        }
        
        ofPopMatrix();
        ofPopStyle();
    }
    
    void toggle() {
        visible = !visible;
    }
    
    void setShape(char c) {
        
        for (int i=0; i<vres; i++) {
            float ii=float(i)/vres;
            vfunc[i]=maxScale; //default
            if (c=='|') vfunc[i]=maxScale;
            if (c=='\\') vfunc[i]=ofMap(ii, 0, 1, maxScale, minScale);
            if (c=='/') vfunc[i]=ofMap(ii, 0, 1, minScale, maxScale);
            if (c=='$') vfunc[i]=ofMap(-sin(1*ii*TWO_PI),-1,1,minScale,maxScale);
            if (c=='#') vfunc[i]=ofMap(-sin(2*ii*TWO_PI),-1,1,minScale,maxScale);
            if (c=='%') vfunc[i]=ofMap(-sin(4*ii*TWO_PI),-1,1,minScale,maxScale);
            if (c=='_') vfunc[i]=ofMap(-sin(8*ii*TWO_PI-HALF_PI),-1,1,minScale,maxScale);
            if (c=='&') vfunc[i]=ofMap(-sin(8*ii*TWO_PI),-1,1,minScale,maxScale);
            if (c=='@') vfunc[i]=ofMap(ii*ii,0,1,minScale,maxScale);
            if (c=='^') vfunc[i]=ofMap(asin(sin(3*ii*TWO_PI)),-PI/2,PI/2,minScale,maxScale);
        }
    }
    
    void mousePressed(int x, int y, int button) {
        isDrawing = bounds.inside(x,y);
    }
    
    void mouseDragged(int x, int y, int button) { //mouse values are always scaled to 0..1280 x 0..800 range
        if (!visible) return;
        if (!isDrawing) return;
        
        float mx = ofClamp(ofGetMouseX()/globalScale - bounds.x, 0, bounds.width);
        float my = ofClamp(ofGetMouseY()/globalScale - bounds.y, 0, bounds.height);
        float pmy = ofClamp(ofGetPreviousMouseY()/globalScale - bounds.y, 0, bounds.height);
        int yy = int(ofMap(my,0,bounds.height, 0, vres-1, true));
        int pyy = int(ofMap(pmy,0,bounds.height, 0, vres-1, true));
        float xx = fabs(ofMap(ofGetMouseX()/globalScale, bounds.x, bounds.x+bounds.width, -1, 1, true));
        
        xx = ofMap(xx,-1,1,minScale,maxScale,true); //0...1 of -1...1 ??? er stond 0..1
        
        int minY = min(yy,pyy);
        int maxY = max(yy,pyy);
        
        if (fabs(minY-maxY) < FLT_EPSILON) maxY++; //prevent div 0

        
        vfunc[yy] = xx;
        
        for (int i=minY; i<=maxY; i++) {
            if (maxY<vres) {
                vfunc[i] = ofMap(i,minY,maxY,vfunc[minY],vfunc[maxY],true);
                vfunc[i] = ofClamp(vfunc[i],minScale,maxScale);   
            }
        }
    }
        
    void mouseReleased(int x, int y, int button) {
        //smooth vertical func
        isDrawing=false;
        if (!visible) return;
        for (int it=0; it<20; it++) {
            float px = vfunc[0];
            for (int i=0; i<vres-1; i++) {
                if (abs(px-vfunc[i])>maxScaleDifference) vfunc[i]=(vfunc[i]+px)/2;
                px = vfunc[i];
            }
        }
    }
};
