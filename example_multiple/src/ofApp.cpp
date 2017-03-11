//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofApp.h"


void ofApp::setup()
{
    ofSetFrameRate(1);
    
    ofLog::setChannel(std::make_shared<ofxIO::ThreadsafeConsoleLoggerChannel>());
    
    ofSetLogLevel(OF_LOG_NOTICE);

    ofJson settings = ofLoadJson("settings.json");

    manager.setup(settings["paths"],
                  settings["instagram"]);

}


void ofApp::update()
{
    ofxInstaLooter::Post post;

    while (manager.posts.tryReceive(post))
    {
        std::cout << post.isDuplicate() << " " << post.hashtag() << " " << post.path() << std::endl;
    }
}
