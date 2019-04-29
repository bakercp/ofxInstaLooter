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
    const auto& instagram = settings["sources"]["instagram"];

    std::filesystem::path imageStorePath = ofToDataPath(paths["image_store_path"], true);
    std::filesystem::path instaLooterPath = ofToDataPath(instagram["instalooter_path"], true);

    std::string username = "";//instagram["credentials"]["username"];
    std::string password = "";//instagram["credentials"]["password"];

    // Get the 0th search.
    const auto& search = instagram["searches"][0];

    std::string hashtag = search["hashtag"];
    uint64_t interval = search["polling_interval"];
    uint64_t numImagesToDownload = search["num_images_to_download"];

    client = std::make_unique<ofxInstaLooter::HashtagClient>(hashtag,
                                                             username,
                                                             password,
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
        auto hashtags = post.hashtags();

        for (auto hashtag: hashtags)
        {
            std::cout << hashtag << ", ";
        }

        std::cout << post.path() << std::endl;
    }
}
