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

    const auto& paths = settings["paths"];
    const auto& instagram = settings["instagram"];

    std::filesystem::path imageStorePath = ofToDataPath(paths["image_store_path"], true);
    std::filesystem::path instaLooterPath = ofToDataPath(instagram["instalooter_path"], true);

    std::string hashtag = instagram["search"]["hashtag"];
    uint64_t interval = instagram["search"]["polling_interval"];
    uint64_t numImagesToDownload = instagram["search"]["num_images_to_download"];

    client = std::make_unique<ofxInstaLooter::HashtagClient>(hashtag,
                                                             imageStorePath,
                                                             interval,
                                                             numImagesToDownload,
                                                             instaLooterPath);

}


void ofApp::update()
{
    ofxInstaLooter::Post post;

    while (client->posts.tryReceive(post))
    {
        std::cout << post.hashtag() << " " << post.path() << std::endl;
    }
}
