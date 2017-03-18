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
    std::filesystem::path destination = "/home/bakercp/SelfieStore/database/instagram/";


    std::filesystem::recursive_directory_iterator dir(destination), end;

    while (dir != end)
    {
        // make sure we don't recurse into certain directories
        // note: maybe check for is_directory() here as well...
        if (std::filesystem::is_directory(dir->path()) && dir->path().filename() == "downloads")
        {
            std::cout << "skipping " << dir->path().string() << std::endl;
            dir.no_push(); // don't recurse into this directory.
        }

        // do other stuff here.








//        if (
//
//
//
//
//
//
//
//        std::filesystem::path newPath = _savePath / Post::relativeStorePathForImage(post);
//
//        Post newPost(newPath,
//                     post.id(),
//                     post.userId(),
//                     post.timestamp(),
//                     post.width(),
//                     post.height(),
//                     post.hashtags());
//
//        std::filesystem::path jsonPath = newPath;
//        jsonPath.replace_extension(".json");
//
//        if (!std::filesystem::exists(newPath))
//        {
//            ofJson newPostJson;
//
//            if (std::filesystem::exists(jsonPath))
//            {
//                ofLogError("HashtagClientManager::_process") << "No image, but was json - overwriting.";
//            }
//
//            newPostJson = Post::toJSON(newPost);
//
//            std::filesystem::create_directories(newPath.parent_path());
//            std::filesystem::rename(post.path(), newPath);
//            ofSavePrettyJson(jsonPath, Post::toJSON(newPost));
//            posts.send(newPost);
//        }
//        else
//        {
//            ofJson existingPostJson = ofLoadJson(jsonPath);
//
//            if (existingPostJson.empty())
//            {
//                ofLogError("HashtagClientManager::_process") << "Image, but invalid json, saving afterall.";
//                ofSavePrettyJson(jsonPath, Post::toJSON(newPost));
//                updatedPosts.send(newPost);
//            }
//            else
//            {
//                try
//                {
//                    Post existingPost = Post::fromJSON(existingPostJson);
//
//                    auto oldHashtags = existingPost.hashtags();
//                    auto oldNumHashtags = oldHashtags.size();
//
//                    auto newHashtags = newPost.hashtags();
//
//                    oldHashtags.insert(newHashtags.begin(),
//                                       newHashtags.end());
//
//                    if (oldNumHashtags != oldHashtags.size())
//                    {
//                        existingPost._hashtags = oldHashtags;
//                        ofSavePrettyJson(jsonPath, Post::toJSON(existingPost));
//                        updatedPosts.send(existingPost);
//                    }
//                    else
//                    {
//                        // skipping
//                    }
//                }
//                catch (const std::exception& e)
//                {
//                    ofLogError("HashtagClientManager::_process") << "Unable to load exisitng post json.";
//                    ofSavePrettyJson(jsonPath, Post::toJSON(newPost));
//                    updatedPosts.send(newPost);
//                }
//            }
//        }

        ++dir;
    }

}

