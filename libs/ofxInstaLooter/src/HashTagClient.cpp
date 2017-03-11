//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofx/InstaLooter/HashtagClient.h"
#include <iomanip>
#include "Poco/PipeStream.h"
#include "Poco/Process.h"
#include "Poco/StreamCopier.h"
#include "ofLog.h"
#include "ofx/IO/DirectoryUtils.h"


namespace ofx {
namespace InstaLooter {


Post::Post()
{
}


Post::Post(const std::filesystem::path& path,
           uint64_t id,
           uint64_t userId,
           uint64_t timestamp,
           const std::string& hashtag):
    _path(path),
    _id(id),
    _userId(userId),
    _timestamp(timestamp),
    _hashtag(hashtag)
{
}


std::filesystem::path Post::path() const
{
    return _path;
}


uint64_t Post::id() const
{
    return _id;
}


uint64_t Post::userId() const
{
    return _userId;
}


uint64_t Post::timestamp() const
{
    return _timestamp;
}


std::string Post::hashtag() const
{
    return _hashtag;
}


bool Post::isDuplicate() const
{
    return _isDuplicate;
}


void Post::setIsDuplicate(bool isDuplicate)
{
    _isDuplicate = isDuplicate;
}


Post Post::fromDownloadPath(const std::filesystem::path& path)
{
    std::string hashtag = path.parent_path().parent_path().filename().string();

    auto filename = path.filename();
    auto tokens = ofSplitString(filename.string(), ".");

    if (tokens.size() != 4)
    {
        throw Poco::InvalidArgumentException("Invalid path: " + filename.string());
    }

    auto idToken = tokens[0];
    auto userIdToken = tokens[1];
    auto timestampToken = tokens[2];

    uint64_t id = std::stoull(idToken);
    uint64_t userId = std::stoull(userIdToken);

    auto _tm = parseDownloadDateTime(timestampToken);

    std::time_t _time = std::mktime(&_tm);

    return Post(path, id, userId, static_cast<uint64_t>(_time), hashtag);
}


std::tm Post::parseDownloadDateTime(const std::string& dateTime)
{
    // Doesn't work across platform for an unknown reason.
    //    std::stringstream ss(timestampToken);
    //    ss >> std::get_time(&_tm, "%Y-%m-%d %Hh%Mm%Ss");

    std::tm _tm = {};

    auto tok0 = ofSplitString(dateTime, " ");

    if (tok0.size() != 2)
    {
        throw Poco::InvalidArgumentException("Invalid dateTime: " + dateTime);
    }

    auto dateToken = tok0[0];
    auto timeToken = tok0[1];

    auto tok1 = ofSplitString(tok0[0], "-");

    if (tok1.size() != 3)
    {
        throw Poco::InvalidArgumentException("Invalid dateTime: " + dateTime);
    }

    _tm.tm_year = ofToInt(tok1[0]) - 1900;
    _tm.tm_mon = ofToInt(tok1[1]) - 1;
    _tm.tm_mday = ofToInt(tok1[2]);

    // 1451944358173325122.221088125.2017-2-16 22h21m17s0.jpg

    auto hIndex = timeToken.find("h");
    auto mIndex = timeToken.find("m");
    auto sIndex = timeToken.find("s");

    if (hIndex == std::string::npos ||
        mIndex == std::string::npos ||
        sIndex == std::string::npos)
    {
        throw Poco::InvalidArgumentException("Invalid timestamp: " + dateTime);
    }

    _tm.tm_hour = ofToInt(timeToken.substr(0, hIndex));
    _tm.tm_min = ofToInt(timeToken.substr(hIndex + 1, mIndex - hIndex - 1));
    _tm.tm_sec = ofToInt(timeToken.substr(mIndex + 1, sIndex - mIndex - 1));
    // ignore milliseconds

    return _tm;
}


std::filesystem::path Post::relativeStorePathForImage(const Post& post)
{
    std::filesystem::path path = "";

    std::string id = std::to_string(post.id());

    if (id.length() <= ID_PATH_DEPTH)
    {
        throw Poco::InvalidArgumentException("Invalid ID: " + id);
    }

    for (auto i = 0; i < ID_PATH_DEPTH; ++i)
    {
        path = path / id.substr(i, 1);
    }

    path = path / id;
    path += ".";
    path += std::to_string(post.userId());
    path += ".";
    path += std::to_string(post.timestamp());
    path += post.path().extension();

    return path;
}


const std::string HashtagClient::DEFAULT_INSTALOOTER_PATH = "/usr/local/bin/instaLooter";
const std::string HashtagClient::FILENAME_TEMPLATE = "{id}.{ownerid}.{datetime}";


HashtagClient::HashtagClient(const std::string& hashtag,
                             const std::filesystem::path& storePath,
                             uint64_t pollingInterval,
                             uint64_t numImagesToDownload,
                             const std::filesystem::path& instaLooterPath):
    IO::PollingThread(std::bind(&HashtagClient::_loot, this), pollingInterval),
    _hashtag(hashtag),
    _storePath(storePath),
    _savePath(_storePath / "instagram" / "downloads" / _hashtag),
    _downloadPath(_savePath / "unsorted"),
    _numImagesToDownload(numImagesToDownload),
    _instaLooterPath(instaLooterPath)
{
    // Ensure that the paths exist.
    std::filesystem::create_directories(_downloadPath);

    // Add file folder extensions.
    _fileExtensionFilter.addExtensions({ "jpg", "jpeg", "gif", "png" });

    start();
}


HashtagClient::~HashtagClient()
{
}


void HashtagClient::_loot()
{
    ofLogVerbose("HashtagClient::_loot") << "Looting " << _hashtag << " " << _downloadPath;

    std::vector<std::string> args;

    args.push_back("hashtag");
    args.push_back(_hashtag);
    args.push_back(_downloadPath.string());
    args.push_back("--quiet");
    args.push_back("--new");
    args.push_back("-n " + std::to_string(_numImagesToDownload));
    args.push_back("-T" + FILENAME_TEMPLATE);

    Poco::Pipe outPipe;

    Poco::ProcessHandle handle = Poco::Process::launch(_instaLooterPath.string(),
                                                       args,
                                                       nullptr,
                                                       &outPipe,
                                                       &outPipe);
    Poco::PipeInputStream istr(outPipe);

    std::atomic<int> exitCode(0);

    std::thread processThread([&](){
        std::string outString;
        Poco::StreamCopier::copyToString(istr, outString);
        ofLogVerbose("HashtagClient::_loot") << "Process Output: " << outString;

        try
        {
            exitCode = handle.wait();
        }
        catch (const std::exception& exc)
        {
            ofLogWarning("HashtagClient::_loot") << "Process Thread: " << exc.what();
        }
    });

    bool didKill = false;

    uint64_t startTime = ofGetElapsedTimeMillis();

    while (isRunning())
    {
        if (Poco::Process::isRunning(handle))
        {
            uint64_t now = ofGetElapsedTimeMillis();

            if (now < (startTime + DEFAULT_PROCESS_TIMEOUT))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_THREAD_SLEEP));
            }
            else break;
        }
        else break;
    }

    if (Poco::Process::isRunning(handle))
    {
        ofLogWarning("HashtagClient::_loot") << "Process timed out, killing.";
        Poco::Process::kill(handle);
        didKill = true;
    }

    try
    {
        exitCode = handle.wait();
    }
    catch (const std::exception& exc)
    {
    }

    try
    {
        ofLogVerbose("HashtagClient::_loot") << "Joining Process ...";
        processThread.join();
    }
    catch (const std::exception& exc)
    {
        ofLogVerbose("HashtagClient::_loot") << "Exit: " << exc.what();
    }

    ofLogVerbose("HashtagClient::_loot") << "... joined and process exited with code: " << exitCode;

    std::vector<std::filesystem::path> paths;

    IO::DirectoryUtils::list(_downloadPath, paths, false, &_fileExtensionFilter);

    std::vector<Post> rawPosts;

/// compare new posts to old posts.
    // sort / copy new posts
    // remove old posts
    // new posts remain as reference for downloader


    for (const auto& path: paths)
    {
        rawPosts.push_back(Post::fromDownloadPath(path));
    }


    std::vector<Post> newPosts;
    std::vector<Post> rawPostsToDelete;

    if (!rawPosts.empty())
    {
        for (const auto& rawPost: rawPosts)
        {
            std::filesystem::path newPath = _savePath / Post::relativeStorePathForImage(rawPost);
            std::filesystem::create_directories(newPath.parent_path());

            Post newPost(newPath,
                         rawPost.id(),
                         rawPost.userId(),
                         rawPost.timestamp(),
                         rawPost.hashtag());

            bool alreadySaved = false;

            for (const auto& post: _lastPostsSaved)
            {
                if (post.path() == newPath)
                {
                    alreadySaved = true;
                    break;
                }
            }


            if (alreadySaved || std::filesystem::exists(newPost.path()))
            {
                if (std::filesystem::exists(rawPost.path()))
                {
                    rawPostsToDelete.push_back(rawPost);
                }
            }
            else
            {
                std::filesystem::copy(rawPost.path(), newPost.path());
                std::filesystem::last_write_time(newPost.path(), static_cast<std::time_t>(newPost.timestamp()));
                newPosts.push_back(newPost);
            }

            if (!isRunning()) break;
        }
    }

    _lastPostsSaved = newPosts;

    std::size_t cleanedUp = 0;

    if (!newPosts.empty() && !didKill)
    {
        for (const auto& rawImage: rawPostsToDelete)
        {
            if (std::filesystem::remove(rawImage.path()))
            {
                ++cleanedUp;
            }
        }
    }

    ofLogNotice("HashtagClient::_loot") << "#" << _hashtag << " New: " << newPosts.size() << (didKill ? " [killed process]" : "") << " Old: " << rawPostsToDelete.size() << " Cleaned up: " << cleanedUp;

    for (const auto& post: newPosts) posts.send(post);
}


} } // ofx::InstaLooter
