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
#include "ofx/IO/ImageUtils.h"


namespace ofx {
namespace InstaLooter {


Post::Post()
{
}


Post::Post(const std::filesystem::path& path,
           uint64_t id,
           uint64_t userId,
           uint64_t timestamp,
           uint64_t width,
           uint64_t height,
           const std::set<std::string>& hashtags):
    _path(path),
    _id(id),
    _userId(userId),
    _timestamp(timestamp),
    _width(width),
    _height(height),
    _hashtags(hashtags)
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


uint64_t Post::width() const
{
    return _width;
}


uint64_t Post::height() const
{
    return _height;
}


std::set<std::string> Post::hashtags() const
{
    return _hashtags;
}


Post Post::fromOldSortedPath(const std::filesystem::path& path)
{
    auto p = path;

    for (auto i = 0; i < ID_PATH_DEPTH; ++i) p = p.parent_path();

    std::string hashtag = p.parent_path().filename().string();

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
    uint64_t time = std::stoull(timestampToken);

    uint64_t width = 0;
    uint64_t height = 0;

    return Post(path,
                id,
                userId,
                time,
                width,
                height,
                { hashtag });
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

    uint64_t width = 0;
    uint64_t height = 0;

    return Post(path,
                id,
                userId,
                static_cast<uint64_t>(_time),
                width,
                height,
                { hashtag });
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
    // Ignore milliseconds token.

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


ofJson Post::toJSON(const Post& post)
{
    ofJson json;
    json["path"] = post._path.string();
    json["id"] = post._id;
    json["user_id"] = post._userId;
    json["timestamp"] = post._timestamp;
    json["width"] = post._width;
    json["height"] = post._height;
    json["hashtags"] = post._hashtags;
    return json;
}


Post Post::fromJSON(const ofJson& json)
{
    Post post;

    try
    {
        post._path = std::filesystem::path(json["path"].get<std::string>());
        post._id = json["id"];
        post._userId = json["user_id"];
        post._timestamp = json["timestamp"];
        post._width = json["width"];
        post._height = json["height"];
        post._hashtags = json["hashtags"].get<std::set<std::string>>();
    }
    catch (const std::exception& exc)
    {
        ofLogError("Post::fromJSON") << "Error parsing JSON:" << exc.what();
    }

    return post;
}


const uint64_t HashtagClient::DEFAULT_POLLING_INTERVAL = 15000;
const uint64_t HashtagClient::DEFAULT_NUM_IMAGES_TO_DOWNLOAD = 4000;
const uint64_t HashtagClient::DEFAULT_PROCESS_TIMEOUT = 300000;
const uint64_t HashtagClient::PROCESS_THREAD_SLEEP = 1000;
const std::string HashtagClient::DEFAULT_INSTALOOTER_PATH = "/usr/local/bin/instaLooter";
const std::string HashtagClient::FILENAME_TEMPLATE = "{id}.{ownerid}.{datetime}";


HashtagClient::HashtagClient(const std::string& hashtag,
                             const std::string& username,
                             const std::string& password,
                             const std::filesystem::path& storePath,
                             uint64_t pollingInterval,
                             uint64_t numImagesToDownload,
                             const std::filesystem::path& instaLooterPath):
    IO::PollingThread(std::bind(&HashtagClient::_loot, this), pollingInterval),
    _hashtag(hashtag),
    _username(username),
    _password(password),
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

    // Start the thread.
    start();
}


HashtagClient::~HashtagClient()
{
}


void HashtagClient::setUsername(const std::string& username)
{
    _username = username;
}


std::string HashtagClient::getUsername() const
{
    return _username;
}

void HashtagClient::setPassword(const std::string& password)
{
    _password = password;
}


std::string HashtagClient::getPassword() const
{
    return _password;
}


void HashtagClient::_loot()
{
    ofLogVerbose("HashtagClient::_loot") << "Looting " << _hashtag << " " << _downloadPath;

    std::vector<std::string> args;

    args.push_back("hashtag");
    args.push_back(_hashtag);
    args.push_back(_downloadPath.string());
    if (_quiet)
    {
        args.push_back("--quiet");
    }
    args.push_back("--new");
    args.push_back("-n " + std::to_string(_numImagesToDownload));
    args.push_back("-T" + FILENAME_TEMPLATE);
    if (!_username.empty() || !_password.empty())
    {
        args.push_back("-c" + _username + ":" + _password);
    }

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

    // compare new posts to old posts.
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

            // Update the path.
            Post newPost = rawPost;
            newPost._path = newPath;

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

                IO::ImageUtils::ImageHeader header;

                if (IO::ImageUtils::loadHeader(header, newPost.path()))
                {
                    newPost._width = header.width;
                    newPost._height = header.height;
                }

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
