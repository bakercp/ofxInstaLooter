//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofApp.h"


void ofApp::setup()
{
    ofLog::setChannel(std::make_shared<ofxIO::ThreadsafeConsoleLoggerChannel>());
    ofSetLogLevel(OF_LOG_VERBOSE);

    {
    auto client = std::make_unique<ofxInstaLooter::HashTagClient>("fridaynightselfie",
                                                                  ofToDataPath("temp", true),
                                                                  30000);
        client->start();
        clients.push_back(std::move(client));
    }

    {
        auto client = std::make_unique<ofxInstaLooter::HashTagClient>("selfie",
                                                                      ofToDataPath("temp", true),
                                                                      5000);
        client->start();
        clients.push_back(std::move(client));
    }

}


void ofApp::update()
{
}
