//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#pragma once


#include <string>
#include <chrono>
#include "ofJson.h"
#include "ofFileUtils.h"
#include "ofx/IO/PollingThread.h"
#include "ofx/IO/FileExtensionFilter.h"
#include "ofx/IO/ThreadChannel.h"


namespace ofx {
namespace InstaLooter {


/// \brief Create an InstaLooter Post.
class Post
{
public:
    Post();

    /// \brief Create a image from paramaters.
    /// \param path The path to the image.
    /// \param id The image id.
    /// \param userId The user id.
    /// \param timestamp The timestamp of the image.
    /// \param width Image width.
    /// \param height Image height.
    /// \param hashtags The hashtags associated with the image.
    Post(const std::filesystem::path& path,
         uint64_t id,
         uint64_t userId,
         uint64_t timestamp,
         uint64_t width,
         uint64_t height,
         const std::set<std::string>& hashtags);

    /// \returns the path to the image.
    std::filesystem::path path() const;

    /// \returns the image id.
    uint64_t id() const;

    /// \returns the user id.
    uint64_t userId() const;

    /// \returns the parsed timestamp.
    uint64_t timestamp() const;

    /// \returns the detected image width.
    uint64_t width() const;

    /// \returns the detected image height.
    uint64_t height() const;

    /// \returns the hashtag that that yielded the image.
    std::set<std::string> hashtags() const;

    static Post fromOldSortedPath(const std::filesystem::path& path);

    /// \brief Create an Image by parsing a filename.
    /// \throws Poco::InvalidArgumentException if unable to parse.
    /// \returns the image or throws an Exception.
    static Post fromDownloadPath(const std::filesystem::path& path);

    /// \throws Poco::InvalidArgumentException if invalid syntax.
    static std::tm parseDownloadDateTime(const std::string& dateTime);

    /// \throws Poco::InvalidArgumentException if unable to parse.
    /// \returns a store path for the post, given the baseStorePath.
    static std::filesystem::path relativeStorePathForImage(const Post& post);

    static ofJson toJSON(const Post& post);
    static Post fromJSON(const ofJson& json);

    enum
    {
        ID_PATH_DEPTH = 6
    };

private:
    std::filesystem::path _path;

    uint64_t _id = 0;
    uint64_t _userId = 0;
    uint64_t _timestamp = 0;
    uint64_t _width = 0;
    uint64_t _height = 0;
    std::set<std::string> _hashtags;

    friend class HashtagClient;
    friend class HashtagClientManager;
    
};


/// \brief A wrapper for executing instaLooter and organizing its loot.
class HashtagClient: public IO::PollingThread
{
public:
    HashtagClient(const std::string& hashtag,
                  const std::string& username,
                  const std::string& password,
                  const std::filesystem::path& storePath,
                  uint64_t pollingInterval = DEFAULT_POLLING_INTERVAL,
                  uint64_t numImagesToDownload = DEFAULT_NUM_IMAGES_TO_DOWNLOAD,
                  const std::filesystem::path& instaLooterPath = DEFAULT_INSTALOOTER_PATH);

    /// \brief Destroy the HashtagClient.
    virtual ~HashtagClient();

    void setUsername(const std::string& username);
    std::string getUsername() const;

    void setPassword(const std::string& password);
    std::string getPassword() const;

    /// \brief A thread channel for new posts fo und by this client.
    IO::ThreadChannel<Post> posts;

    /// \brief The default Instagram polling interval in milliseconds.
    static const uint64_t DEFAULT_POLLING_INTERVAL;

    /// \brief The default number of images to download per query.
    static const uint64_t DEFAULT_NUM_IMAGES_TO_DOWNLOAD;

    /// \brief Default command timeout in milliseconds.
    static const uint64_t DEFAULT_PROCESS_TIMEOUT;

    /// \brief Thread sleep timeout in milliseconds.
    static const uint64_t PROCESS_THREAD_SLEEP;

    /// \brief Default instaLooter script path.
    static const std::string DEFAULT_INSTALOOTER_PATH;

    /// \brief The instaLooter filename template.
    static const std::string FILENAME_TEMPLATE;

private:
    /// \brief An internal function for executing instaLooter.
    void _loot();

    /// \brief If true, there is no output from instaLooter.
    bool _quiet = false;

    /// \brief The optional username for authenticated searches.
    std::string _username;

    /// \brief The optional password for authenticated searches.
    std::string _password;

    /// \brief The search hashtag.
    std::string _hashtag;

    std::filesystem::path _storePath;

    /// \brief The image save path.
    std::filesystem::path _savePath;

    /// \brief The raw download path.
    std::filesystem::path _downloadPath;

    /// \brief The location of the instaLooter app.
    std::filesystem::path _instaLooterPath;

    uint64_t _numImagesToDownload = DEFAULT_NUM_IMAGES_TO_DOWNLOAD;

    uint64_t _processTimeout = DEFAULT_PROCESS_TIMEOUT;

    IO::FileExtensionFilter _fileExtensionFilter;

    std::vector<Post> _lastPostsSaved;

};


} } // ofx::InstaLooter
