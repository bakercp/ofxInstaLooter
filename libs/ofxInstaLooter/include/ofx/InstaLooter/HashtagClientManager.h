//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#pragma once


#include "ofJson.h"
#include "ofx/InstaLooter/HashtagClient.h"
#include "ofx/IO/Thread.h"


namespace ofx {
namespace InstaLooter {


/// \brief Manage multiple HashTagClients and merge their results.
class HashtagClientManager: public IO::PollingThread
{
public:
    HashtagClientManager();
    virtual ~HashtagClientManager();

    void setup(const ofJson& paths, const ofJson& settings);

    /// \brief New posts.
    IO::ThreadChannel<Post> posts;

    /// \brief Posts that have been downloaded already but have additional or updated info (e.g. hashtags).
    IO::ThreadChannel<Post> updatedPosts;

private:
    void _process();

    std::filesystem::path _storePath;
    std::filesystem::path _savePath;

    std::vector<std::unique_ptr<HashtagClient>> _clients;

};


} } // ofx::InstaLooter
