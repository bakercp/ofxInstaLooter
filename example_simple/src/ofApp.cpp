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

    ofJson settings = ofLoadJson("settings.json");

    std::filesystem::path instaLooterPath = settings["instalooter_path"].get<std::string>();
    std::filesystem::path imageStorePath = ofToDataPath(settings["image_store_path"], true);

    for (const auto& search: settings["hashtags"])
    {
        std::string hashtag = search["hashtag"];
        uint64_t interval = search["polling_interval"];
        uint64_t numImagesToDownload = search["num_images_to_download"];

        auto client = std::make_unique<ofxInstaLooter::HashTagClient>(hashtag,
                                                                      imageStorePath,
                                                                      interval,
                                                                      numImagesToDownload,
                                                                      instaLooterPath);

        client->start();
        clients.push_back(std::move(client));

    }

}


void ofApp::update()
{
}
