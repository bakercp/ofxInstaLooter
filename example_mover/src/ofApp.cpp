//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofApp.h"




void ofApp::setup()
{
    using ofx::InstaLooter::Post;

    std::filesystem::path unsorted = "/SelfieStore/data/database/instagram_unsorted/";
    std::filesystem::path _savePath = "/home/bakercp/SelfieStore/database/instagram/";


    std::filesystem::recursive_directory_iterator dir(unsorted), end;

    while (dir != end)
    {
        if (std::filesystem::is_directory(dir->path()) && dir->path().filename() == "downloads")
        {
            std::cout << "skipping " << dir->path().string() << std::endl;
            dir.no_push(); // don't recurse into this directory.
        }
        else if (!std::filesystem::is_directory(dir->path()))
        {
            Post post = Post::fromOldSortedPath(dir->path());

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
                std::filesystem::copy(post.path(), newPath);

                ofx::IO::JSONUtils::saveJSON(jsonPath, Post::toJSON(newPost));

            }
            else
            {
                ofJson existingPostJson;

                if (ofx::IO::JSONUtils::loadJSON(jsonPath, existingPostJson))
                {
                    ofLogError("HashtagClientManager::_process") << "Image, but invalid json, saving afterall.";
                    ofx::IO::JSONUtils::saveJSON(jsonPath, Post::toJSON(newPost));
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
                    }
                }
            }
        }

        ++dir;
    }

}

