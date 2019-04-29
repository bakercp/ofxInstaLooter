//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofx/InstaLooter/HashtagClientManager.h"
#include "ofx/IO/JSONUtils.h"


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
    _storePath = ofToDataPath(paths.value("image_store_path", ""), true);
    _savePath = _storePath / "instagram";

    setPollingInterval(settings.value("manager_polling_interval",
                                      getPollingInterval()));

    auto instaLooterPath = ofToDataPath(settings.value("instalooter_path",
                                                       HashtagClient::DEFAULT_INSTALOOTER_PATH),
                                        true);

    auto credentials = settings.find("credentials");

    std::string username = "";
    std::string password = "";

    if (credentials != settings.end())
    {
        username = credentials->value("username", "");
        password = credentials->value("password", "");
    }

    auto searchesIter = settings.find("searches");

    if (searchesIter != settings.end())
    {
        for (const auto& search: *searchesIter)
        {
            auto hashtagIter = search.find("hashtag");

            if (hashtagIter != search.end())
            {
                std::string hashtag = hashtagIter->get<std::string>();

                uint64_t interval = search.value("polling_interval",
                                                 HashtagClient::DEFAULT_POLLING_INTERVAL);

                uint64_t numImagesToDownload = search.value("num_images_to_download",
                                                            HashtagClient::DEFAULT_NUM_IMAGES_TO_DOWNLOAD);

                auto client = std::make_unique<HashtagClient>(hashtag,
                                                              username,
                                                              password,
                                                              _storePath,
                                                              interval,
                                                              numImagesToDownload,
                                                              instaLooterPath);

                _clients.push_back(std::move(client));
            }
            else
            {
                ofLogError("HashtagClientManager::setup") << "No hashtag was listed in the search.";
            }
        }

        start();
    }
    else
    {
        ofLogError("HashtagClientManager::setup") << "No searches were in the settings file.";
    }
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
                         post.hashtags());

            std::filesystem::path jsonPath = newPath;
            jsonPath.replace_extension(".json.gz");

            if (!std::filesystem::exists(newPath))
            {
                ofJson newPostJson;

                if (std::filesystem::exists(jsonPath))
                {
                    ofLogError("HashtagClientManager::_process") << "No image, but was json - overwriting.";
                }

                newPostJson = Post::toJSON(newPost);

                std::filesystem::create_directories(newPath.parent_path());
                std::filesystem::rename(post.path(), newPath);

                ofx::IO::JSONUtils::saveJSON(jsonPath, Post::toJSON(newPost));

                posts.send(newPost);
            }
            else
            {
                ofJson existingPostJson;

                if (ofx::IO::JSONUtils::loadJSON(jsonPath, existingPostJson))
                {
                    ofLogError("HashtagClientManager::_process") << "Image, but invalid json, saving afterall.";
                    ofx::IO::JSONUtils::saveJSON(jsonPath, Post::toJSON(newPost));
                    updatedPosts.send(newPost);
                }
                else
                {
                    try
                    {
                        Post existingPost = Post::fromJSON(existingPostJson);

                        auto oldHashtags = existingPost.hashtags();
                        auto oldNumHashtags = oldHashtags.size();

                        auto newHashtags = newPost.hashtags();

                        oldHashtags.insert(newHashtags.begin(),
                                           newHashtags.end());

                        if (oldNumHashtags != oldHashtags.size())
                        {
                            existingPost._hashtags = oldHashtags;
                            ofx::IO::JSONUtils::saveJSON(jsonPath, Post::toJSON(existingPost));
                            updatedPosts.send(existingPost);
                        }
                        else
                        {
                            // skipping
                        }
                    }
                    catch (const std::exception& e)
                    {
                        ofLogError("HashtagClientManager::_process") << "Unable to load exisitng post json.";
                        ofx::IO::JSONUtils::saveJSON(jsonPath, Post::toJSON(newPost));
                        updatedPosts.send(newPost);
                    }
                }
            }
        }
    }
}


} } // ofx::InstaLooter
