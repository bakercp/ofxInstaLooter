//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofApp.h"
#include "ofxIO.h"
#include "ofxInstaLooter.h"


void ofApp::setup()
{
    std::filesystem::path _basePath = ofToDataPath("/Volumes/SuperFastHD/Saatchi/instagram/selfie");;

    std::filesystem::path path = _basePath / "images";

    std::vector<std::filesystem::path> extensions = { ".jpg", ".jpeg", ".png", ".gif" };

    std::size_t existingImages = 0;
    std::size_t newImages = 0;

    std::size_t count = 0;
    for (auto& path: std::filesystem::recursive_directory_iterator(path))
    {
        ++count;

        if (count % 500 == 0)
        {
            std::cout << "# " << count << std::endl;
        }

        if (!std::filesystem::is_directory(path))
        {
            auto extension = path.path().extension();

            if (extension == ".jpg" ||
                extension == ".jpeg" ||
                extension == ".png" ||
                extension == ".gif")
            {
                try
                {
                    auto rawImage = ofxInstaLooter::Image::fromPath(path);

                    std::filesystem::path newPath = _basePath / ofxInstaLooter::Image::relativeStorePathForImage(rawImage);
                    std::filesystem::create_directories(newPath.parent_path());

                    ofxInstaLooter::Image newImage(newPath, rawImage.id(), rawImage.userId(), rawImage.timestamp());

                    if (std::filesystem::exists(newPath))
                    {
                        ++existingImages;
                    }
                    else
                    {
                        ++newImages;
                        std::filesystem::copy(rawImage.path(), newImage.path());
                    }

                    std::filesystem::last_write_time(newImage.path(), static_cast<std::time_t>(newImage.timestamp()));

                }
                catch (const Poco::InvalidArgumentException& exc)
                {
                    ofLogError() << exc.displayText();
                }
            }
            else
            {
                ofLogWarning() << "Unknown extension: " << extension;
            }
        }
    }

    ofExit();
}