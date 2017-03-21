//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofApp.h"



void ofApp::setup()
{
    ofSetLoggerChannel(std::make_shared<ofxIO::ThreadsafeConsoleLoggerChannel>());


   auto buffer = ofBufferFromFile("folders.txt");
   auto lines = buffer.getLines();
   

    for (auto line: lines) paths.send(line);

    std::size_t numThreads = 16;

    for (std::size_t i = 0; i < numThreads; ++i)
    {
        threads.emplace_back(std::thread([&](){
            std::filesystem::path path;
            while (paths.receive(path))
            {
                try
                {
                    std::map<uint64_t, std::vector<std::filesystem::path>> paths;

                    std::filesystem::directory_iterator iter(path), end;

                    while (iter != end)
                    {
                        std::string extension = std::filesystem::extension(*iter);
                        std::string basename = std::filesystem::basename(*iter);

                        if (basename.size() > 0 && basename[0] != '.' && extension != ".gz" && extension != ".json")
                        {
                            ofLogNotice() << ">" << iter->path().filename().string();
                        }

                        ++iter;
                    }

                }
                catch (const std::exception& exc)
                {
                    ofLogError() << exc.what();
                }

            }
        }));
    }

cout << "hhh" << endl;

    // ofx::IO::JSONUtils::saveJSON(jsonPath, Post::toJSON(existingPost));
}


void ofApp::exit()
{
    paths.close();
    for (auto& t: threads) t.join();
}
