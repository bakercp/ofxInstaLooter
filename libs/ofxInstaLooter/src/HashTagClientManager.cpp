//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofx/InstaLooter/HashtagClientManager.h"


namespace ofx {
namespace InstaLooter {



HashtagClientManager::HashtagClientManager():
    IO::PollingThread(std::bind(&HashtagClientManager::_process, this))
{
}


HashtagClientManager::~HashtagClientManager()
{

}
    

void HashtagClientManager::setup(const ofJson& paths, const ofJson& settings)
{
    _storePath = ofToDataPath(paths["image_store_path"], true);
    _savePath = _storePath / "instagram";

    setPollingInterval(settings["manager_polling_interval"]);

    auto instaLooterPath = ofToDataPath(settings["instalooter_path"], true);

    std::string username = settings["credentials"]["username"];
    std::string password = settings["credentials"]["password"];

    for (const auto& search: settings["searches"])
    {
        std::string hashtag = search["hashtag"];
        uint64_t interval = search["polling_interval"];
        uint64_t numImagesToDownload = search["num_images_to_download"];

        auto client = std::make_unique<HashtagClient>(hashtag,
                                                      username,
                                                      password,
                                                      _storePath,
                                                      interval,
                                                      numImagesToDownload,
                                                      instaLooterPath);

        _clients.push_back(std::move(client));
    }

    start();
}


void HashtagClientManager::_process()
{
    for (auto& client: _clients)
    {
        Post post;

        while (client->posts.tryReceive(post) && isRunning())
        {
            std::filesystem::path newPath = _savePath / Post::relativeStorePathForImage(post);

            Post newPost(newPath,
                         post.id(),
                         post.userId(),
                         post.timestamp(),
                         post.width(),
                         post.height(),
                         post.hashtag());

            if (!std::filesystem::exists(newPath))
            {
                std::filesystem::create_directories(newPath.parent_path());
                std::filesystem::rename(post.path(), newPath);
                newPost.setIsDuplicate(false);
            }
            else
            {
                std::filesystem::remove(post.path());
                newPost.setIsDuplicate(true);
            }

            posts.send(newPost);
        }
    }
}


} } // ofx::InstaLooter
