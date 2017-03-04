//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#pragma once


#include "ofMain.h"
#include "ofxIO.h"
#include "ofxInstaLooter.h"


class ofApp: public ofBaseApp
{
public:
    void setup();
    void update();

    std::vector<std::unique_ptr<ofxInstaLooter::HashTagClient>> clients;

};
