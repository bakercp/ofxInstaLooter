//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofApp.h"



void ofApp::setup()
{
    ofSetFrameRate(1);
    ofSetLoggerChannel(std::make_shared<ofxIO::ThreadsafeConsoleLoggerChannel>());


   auto buffer = ofBufferFromFile("folders.txt");
   auto lines = buffer.getLines();
   

    for (auto line: lines) paths.send(line);

    std::size_t numThreads = 1;

    for (std::size_t i = 0; i < numThreads; ++i)
    {
        threads.emplace_back(std::thread([&](){
            std::filesystem::path path;
            while (paths.receive(path))
            {

                try
                {
                    std::unordered_map<uint64_t, std::vector<std::filesystem::path>> pathMap;

                    std::filesystem::directory_iterator iter(path), end;

                    while (iter != end)
                    {
                        std::string extension = std::filesystem::extension(*iter);
                        std::string basename = std::filesystem::basename(*iter);

                        if (basename.size() > 0 && basename[0] != '.' && extension != ".gz" && extension != ".json")
                        {
                            std::string id_str = basename.substr(0, basename.find("."));

                            if (id_str.length() > 0)
                            {
                                uint64_t id = std::stoull(id_str);
                                pathMap[id].push_back(iter->path());
                            }
                        }

                        ++iter;
                    }

                    for (auto& entry: pathMap)
                    {
                        if (entry.second.size() > 1)
                        {
                            std::stringstream ss;
                            //ss << "Merging:" << std::endl;

                            std::set<std::string> hashtags;

                            ofJson reference;

                            uint64_t fs = -1;


                            for (std::size_t i = 0; i < entry.second.size(); ++i)
                            {
                                uint64_t _fs = std::filesystem::file_size(entry.second[i]);

                                if (i == 0)
                                {
                                    fs = _fs;
                                }
                                else if (fs != _fs)
                                {
                                    ss << "    NON MATCHING FS ";
                                }


                                std::filesystem::path jsonPath = entry.second[i];

                                jsonPath = jsonPath.replace_extension(".json.gz");

                                ofJson json;

                                if (ofxIO::JSONUtils::loadJSON(jsonPath, json))
                                {
                                    for (auto& hashtag: json["hashtags"])
                                    {
                                        std::string h = hashtag.get<std::string>();
                                        if (h.length() > 1)
                                            hashtags.insert(h);
                                    }

                                    if (i == entry.second.size() -1)
                                    {
                                        reference = json;
                                        ss << "    Keeping " << entry.second[i].string() << std::endl;
                                    }
                                    else
                                    {
                                        ss << "    Removing " << entry.second[i].string() << std::endl;
                                    }
                                }


                            }

                            ss << std::endl;
                            
                            for (auto& hashtag: hashtags)
                            {
                                ss << " " << hashtag;
                            }

                            reference["hashtags"] = hashtags;

			    // ofxIO::JSONUtils::saveJSON(reference);
	
                                ofLogNotice() << ss.str();
                        }
                    }

                }
                catch (const std::exception& exc)
                {
                    ofLogError() << exc.what();
                }

            }
        }));
    }

}


void ofApp::update()
{
    if (paths.size() == 0)
    {
        std::cout << "done" << std::endl;
        paths.close();
        for (auto& t: threads) t.join();
    }
}

void ofApp::exit()
{
    paths.close();
    for (auto& t: threads) t.join();
}
