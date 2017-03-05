//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#include "ofx/InstaLooter/HashTagClient.h"
#include "Poco/PipeStream.h"
#include "Poco/Process.h"
#include "Poco/StreamCopier.h"
#include "ofLog.h"
#include "ofx/IO/DirectoryUtils.h"


namespace ofx {
namespace InstaLooter {


Image::Image(const std::filesystem::path& path,
             uint64_t id,
             uint64_t userId,
             uint64_t timestamp):
    _path(path),
    _id(id),
    _userId(userId),
    _timestamp(timestamp)
{
}


std::filesystem::path Image::path() const
{
    return _path;
}


uint64_t Image::id() const
{
    return _id;
}


uint64_t Image::userId() const
{
    return _userId;
}


uint64_t Image::timestamp() const
{
    return _timestamp;
}


Image Image::fromPath(const std::filesystem::path& path)
{
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

    std::tm _tm = {};
    std::stringstream ss(timestampToken);
    ss >> std::get_time(&_tm, "%Y-%m-%d %Hh%Mm%Ss");

    std::time_t _time = std::mktime(&_tm);

    std::filesystem::last_write_time(path, static_cast<std::time_t>(_time));

    return Image(path, id, userId, static_cast<uint64_t>(_time));
}


std::filesystem::path Image::relativeStorePathForImage(const Image& image)
{
    std::filesystem::path path = "";

    std::string id = std::to_string(image.id());

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
    path += std::to_string(image.userId());
    path += ".";
    path += std::to_string(image.timestamp());
    path += image.path().extension();

    return path;
}


const std::string HashTagClient::DEFAULT_INSTALOOTER_PATH = "/usr/local/bin/instaLooter";
const std::string HashTagClient::FILENAME_TEMPLATE = "{id}.{ownerid}.{datetime}";


HashTagClient::HashTagClient(const std::string& hashtag,
                             const std::filesystem::path& imageStorePath,
                             uint64_t pollingInterval,
                             uint64_t numImagesToDownload,
                             const std::filesystem::path& instaLooterPath):
    IO::PollingThread(std::bind(&HashTagClient::_loot, this), pollingInterval),
    _hashtag(hashtag),
    _imageStorePath(imageStorePath),
    _numImagesToDownload(numImagesToDownload),
    _instaLooterPath(instaLooterPath)
{
    // Construct paths.
    _basePath = _imageStorePath / _hashtag;
    _downloadPath = _basePath / "downloads";

    // Ensure that the paths exist.
    std::filesystem::create_directories(_downloadPath);

    // Add file folder extensions.
    _fileExtensionFilter.addExtensions({ "jpg", "jpeg", "gif", "png" });
}


HashTagClient::~HashTagClient()
{
}


void HashTagClient::_loot()
{
    ofLogVerbose("HashTagClient::_loot") << "Looting " << _hashtag << " " << _downloadPath;

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
        ofLogVerbose("HashTagClient::_loot") << "Process Output: " << outString;

        try
        {
            exitCode = handle.wait();
        }
        catch (const std::exception& exc)
        {
            ofLogWarning("HashTagClient::_loot") << "Process Thread: " << exc.what();
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
        ofLogWarning("HashTagClient::_loot") << "Process timed out, killing.";
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
        ofLogVerbose("HashTagClient::_loot") << "Joining Process ...";
        processThread.join();
    }
    catch (const std::exception& exc)
    {
        ofLogVerbose("HashTagClient::_loot") << "Exit: " << exc.what();
    }

    ofLogVerbose("HashTagClient::_loot") << "... joined and process exited with code: " << exitCode;

    std::vector<std::filesystem::path> paths;

    IO::DirectoryUtils::list(_downloadPath, paths, false, &_fileExtensionFilter);

    std::vector<Image> rawImages;

    for (const auto& path: paths) rawImages.push_back(Image::fromPath(path));

    std::vector<Image> newImages;
    std::vector<Image> rawImagesToDelete;

    if (!rawImages.empty())
    {
        // We leave the newest so instaLooter will have a reference point for newer images.
        for (const auto& rawImage: rawImages)
        {
            std::filesystem::path newPath = _basePath / Image::relativeStorePathForImage(rawImage);
            std::filesystem::create_directories(newPath.parent_path());

            Image newImage(newPath, rawImage.id(), rawImage.userId(), rawImage.timestamp());

            if (std::filesystem::exists(newPath))
            {
                if (std::filesystem::exists(rawImage.path()))
                {
                    rawImagesToDelete.push_back(rawImage);
                }
            }
            else
            {
                std::filesystem::copy(rawImage.path(), newImage.path());
                newImages.push_back(newImage);
            }

            if (!isRunning()) break;
        }
    }

    std::size_t cleanedUp = 0;

    if (!newImages.empty() && !didKill)
    {
        for (const auto& rawImage: rawImagesToDelete)
        {
            if (std::filesystem::remove(rawImage.path()))
            {
                ++cleanedUp;
            }
        }
    }

    ofLogNotice("HashTagClient::_loot") << "#" << _hashtag << " New: " << newImages.size() << (didKill ? " [killed process]" : "") << " Old: " << rawImagesToDelete.size() << " Cleaned up: " << cleanedUp;
}


} } // ofx::InstaLooter
