//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#pragma once


#include "ofMain.h"
#include "ofxIO.h"


class ofApp: public ofBaseApp
{
public:
    void setup() override;
    void update() override;
    void exit() override;

    std::vector<std::thread> threads;
    ofxIO::ThreadChannel<std::filesystem::path> paths;

};
