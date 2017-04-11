//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofApp.h"


void ofApp::setup()
{
    // ofSetLogLevel(OF_LOG_VERBOSE);
    ofLog::setChannel(std::make_shared<ofxIO::ThreadsafeConsoleLoggerChannel>());

    ofJson settings = ofLoadJson("settings.json");

    manager.setup(settings["paths"], settings["sources"]["instagram"]);

}


void ofApp::update()
{
    ofxInstaLooter::Post post;

    while (manager.posts.tryReceive(post))
    {
        std::stringstream ss;

        for (auto hashtag: post.hashtags())
        {
            ss << hashtag << ",";
        }

        ofLogNotice("ofApp::update") << "New post with hashtags " << ss.str() << " @ " << post.path().filename();
    }

    while (manager.updatedPosts.tryReceive(post))
    {
        std::stringstream ss;
        
        for (auto hashtag: post.hashtags())
        {
            ss << hashtag << ",";
        }

        ofLogNotice("ofApp::update") << "Updated post with hashtags " << ss.str() << " @ " << post.path().filename();
    }
}
