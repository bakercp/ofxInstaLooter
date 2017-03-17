//
// Copyright (c) 2017 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:	MIT
//


#pragma once


#include <string>
#include <chrono>
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
    /// \param hashtag The hashtag that yeilded the image.
    Post(const std::filesystem::path& path,
         uint64_t id,
         uint64_t userId,
         uint64_t timestamp,
         uint64_t width,
         uint64_t height,
         const std::string& hashtag);

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
    std::string hashtag() const;

    bool isDuplicate() const;

    void setIsDuplicate(bool duplicate);

    /// \brief Create an Image by parsing a filename.
    /// \throws Poco::InvalidArgumentException if unable to parse.
    /// \returns the image or throws an Exception.
    static Post fromDownloadPath(const std::filesystem::path& path);

    /// \throws Poco::InvalidArgumentException if invalid syntax.
    static std::tm parseDownloadDateTime(const std::string& dateTime);

    /// \throws Poco::InvalidArgumentException if unable to parse.
    /// \returns a store path for the post, given the baseStorePath.
    static std::filesystem::path relativeStorePathForImage(const Post& post);

    enum
    {
        ID_PATH_DEPTH = 6
    };

private:
    bool _isDuplicate = false;
    std::filesystem::path _path;

    uint64_t _id = 0;
    uint64_t _userId = 0;
    uint64_t _timestamp = 0;
    uint64_t _width = 0;
    uint64_t _height = 0;
    std::string _hashtag;

    friend class HashtagClient;
};

///
/// Images are saved at
///
/// storePath / instagram / hashtag / ...
///
/// Images are downloaded to
///
/// storePath / instagram / hashtag / downloads

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

    virtual ~HashtagClient();

    void setUsername(const std::string& username);
    std::string getUsername() const;

    void setPassword(const std::string& password);
    std::string getPassword() const;

    IO::ThreadChannel<Post> posts;

    enum
    {
        /// \brief The default Instagram polling interval in milliseconds.
        DEFAULT_POLLING_INTERVAL = 15000,

        DEFAULT_NUM_IMAGES_TO_DOWNLOAD = 4000,

        /// \brief Default command timeout in milliseconds.
        DEFAULT_PROCESS_TIMEOUT = 300000,

        /// \brief Thread sleep timeout in milliseconds.
        PROCESS_THREAD_SLEEP = 1000

    };

    /// \brief Default instaLooter script path.
    static const std::string DEFAULT_INSTALOOTER_PATH;

    /// \brief The instaLooter filename template.
    static const std::string FILENAME_TEMPLATE;

private:
    void _loot();

    bool _quiet = false;

    std::string _username;
    std::string _password;

    std::string _hashtag;

    std::filesystem::path _storePath;
    std::filesystem::path _savePath;
    std::filesystem::path _downloadPath;
    std::filesystem::path _instaLooterPath;

    uint64_t _numImagesToDownload = DEFAULT_NUM_IMAGES_TO_DOWNLOAD;

    uint64_t _processTimeout = DEFAULT_PROCESS_TIMEOUT;

    IO::FileExtensionFilter _fileExtensionFilter;

    std::vector<Post> _lastPostsSaved;

};


} } // ofx::InstaLooter
