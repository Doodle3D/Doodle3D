#pragma once
#include "ofMain.h"
#include "ofxExtras.h"

class Canvas {
public:
    
    int circleDetail;
    ofRectangle bounds;
    bool isDrawing;

    void setup() {
        isDrawing = false;
        bounds.set(220,180,640,440);
        circleDetail = 16;
        path.setFilled(false);
        path.setCurveResolution(100);
        path.setStrokeWidth(2);
        path.setStrokeColor(0);
    }
    
    void draw() {
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(bounds.x,bounds.y);
        
        vector<ofSubPath> &subpaths = path.getSubPaths();

        if (useSubpathColors) {
            for (int i=0; i<subpaths.size(); i++) {
                vector<ofSubPath::Command> &commands = subpaths[i].getCommands();
                ofNoFill();
                ofSetLineWidth(2);
                ofSetColor(ofColor::fromHsb(float(i)/subpaths.size()*255, 255, 255));
                ofBeginShape();
                for (int j=0; j<commands.size(); j++) {
                    ofVertex(commands[j].to.x, commands[j].to.y);
                }
                ofEndShape();
            }
        } else {
            if (subpaths.size()>0) path.draw(0,0);
        }
        
        if (subpaths.size()>1) {
            ofSetColor(200);
            for (int i=0; i<subpaths.size()-1; i+=2) {
                vector<ofSubPath::Command> &left = subpaths[i].getCommands();
                vector<ofSubPath::Command> &right = subpaths[i+1].getCommands();
                ofLine(left.back().to,right.front().to);
            }
        }

        ofPopMatrix();
        ofPopStyle();
    }
    
    void update() {
        float translateStep=3;
        ofPoint center = ofxGetCenterOfMass(ofxGetPointsFromPath(path));
        if (ofGetKeyPressed('-')) { path.translate(-center); path.scale(.97,.97); path.translate(center); }
        if (ofGetKeyPressed('=')) { path.translate(-center); path.scale(1/.97,1/.97); path.translate(center); }
        if (ofGetKeyPressed('[')) { path.translate(-center); path.rotate(-1,ofVec3f(0,0,1)); path.translate(center); }
        if (ofGetKeyPressed(']')) { path.translate(-center); path.rotate(1,ofVec3f(0,0,1)); path.translate(center); }
        if (ofGetKeyPressed(OF_KEY_LEFT)) path.translate(ofPoint(-translateStep,0));
        if (ofGetKeyPressed(OF_KEY_RIGHT)) path.translate(ofPoint(translateStep,0));
        if (ofGetKeyPressed(OF_KEY_UP)) path.translate(ofPoint(0,-translateStep));
        if (ofGetKeyPressed(OF_KEY_DOWN)) path.translate(ofPoint(0,translateStep));
    }
    
    void drawDebug() {
        ofPushStyle();
        ofPushMatrix();
        ofSetColor(255,50);
        ofRect(bounds);
        ofTranslate(bounds.x,bounds.y);
        vector<ofPoint*> points = ofxGetPointsFromPath(path);
        ofSetColor(255,0,0);
        ofNoFill();
        ofRect(ofxGetBoundingBox(points));
        ofFill();
        ofCircle(ofxGetCenterOfMass(points),3);
        ofSetColor(0,255,0);
        ofCircle(ofxGetBoundingBox(points).getCenter(),3);
        ofPopMatrix();
        ofPopStyle();
    }
    
    void createCircle() {
        for (float i=0,n=circleDetail; i<=n; i++) {
            float ii=float(i)/n;
            float x=100*sin(ii*TWO_PI)+300;
            float y=100*cos(ii*TWO_PI)+200;
            if (i==0) path.moveTo(x,y); else path.lineTo(x,y);
        }
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
    
    void mousePressed(int x, int y, int button) {
        if (bounds.inside(x,y)) {
            isDrawing = true;
            path.moveTo(x-bounds.x,y-bounds.y);
        }
    }

    void mouseDragged(int x, int y, int button) {
        if (isDrawing && bounds.inside(x,y)) {
            path.lineTo(x-bounds.x,y-bounds.y);
        }        
    }
    
    void mouseReleased(int x, int y, int button) {
        isDrawing = false;
    }
};
