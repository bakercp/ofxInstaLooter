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
    auto tokens = ofSplitString(filename.string(), ".", true, true);

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

Image Image::createAndStoreFromPath(const std::filesystem::path& rawImagePath,
                                    const std::filesystem::path& baseStorePath)
{
    Image image = fromPath(rawImagePath);
    std::filesystem::path newPath = baseStorePath / relativeStorePathForImage(image);
    std::filesystem::create_directories(newPath.parent_path());
    std::filesystem::rename(image.path(), newPath);
    image._path = newPath;
    std::filesystem::last_write_time(image.path(),
                                     static_cast<std::time_t>(image.timestamp()));
    return image;
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

    uint64_t startTime = ofGetElapsedTimeMillis();

    Poco::Pipe outPipe;
    Poco::ProcessHandle handle = Poco::Process::launch(_instaLooterPath.string(), args, 0, &outPipe, &outPipe);
    Poco::PipeInputStream istr(outPipe);

    std::string str;
    Poco::StreamCopier::copyToString(istr, str);

    ofLogVerbose("HashTagClient::_loot") << str;

    int code = handle.wait();

    ofLogVerbose("HashTagClient::_loot") << "Process exited with code: " << code;

    _processLoot();
}



void HashTagClient::_processLoot()
{
    std::vector<Image> images;

    std::vector<std::filesystem::path> paths;

    IO::DirectoryUtils::list(_downloadPath,
                             paths,
                             false,
                             &_fileExtensionFilter);

    if (!paths.empty())
    {
        // We leave the newest so instaLooter will have a reference point for newer images.
        for (auto i = 0; i < paths.size() - 1; ++i)
        {
            auto filename = paths[i].filename();

            images.push_back(Image::createAndStoreFromPath(paths[i], _basePath));

            ofLogVerbose("HashTagClient::_processLoot") << filename;

            if (!isRunning()) break;
        }
    }


}


} } // ofx::InstaLooter
