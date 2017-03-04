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


namespace ofx {
namespace InstaLooter {


/// \brief Create an InstaLooter Image.
class Image
{
public:
    /// \brief Create a image from paramaters.
    /// \param path The path to the image.
    /// \param id The image id.
    /// \param userId The user id.
    /// \param timestamp The timestamp of the image.
    Image(const std::filesystem::path& path,
          uint64_t id,
          uint64_t userId,
          uint64_t timestamp);

    /// \returns the path to the image.
    std::filesystem::path path() const;

    /// \returns the image id.
    uint64_t id() const;

    /// \returns the user id.
    uint64_t userId() const;

    /// \returns the parsed timestamp.
    uint64_t timestamp() const;

    /// \brief Create an Image by parsing a filename.
    /// \throws Poco::InvalidArgumentException if unable to parse.
    /// \returns the image or throws an Exception.
    static Image fromPath(const std::filesystem::path& path);

    /// \throws Poco::InvalidArgumentException if unable to parse.
    /// \returns An image that has been parsed and move to the .
    static Image createAndStoreFromPath(const std::filesystem::path& rawImagePath,
                                        const std::filesystem::path& baseStorePath);

    /// \throws Poco::InvalidArgumentException if unable to parse.
    /// \returns a store path for the image, given the baseStorePath.
    static std::filesystem::path relativeStorePathForImage(const Image& image);

    enum
    {
        ID_PATH_DEPTH = 6
    };

private:
    std::filesystem::path _path;
    uint64_t _id = 0;
    uint64_t _userId = 0;
    uint64_t _timestamp = 0;

};

class HashTagClient: public IO::PollingThread
{
public:
    HashTagClient(const std::string& hashtag,
                  const std::filesystem::path& imageStorePath,
                  uint64_t pollingInterval = DEFAULT_POLLING_INTERVAL,
                  uint64_t numImagesToDownload = DEFAULT_NUM_IMAGES_TO_DOWNLOAD,
                  const std::filesystem::path& instaLooterPath = DEFAULT_INSTALOOTER_PATH);

    virtual ~HashTagClient();

    enum
    {
        /// \brief The default Instagram polling interval in milliseconds.
        DEFAULT_POLLING_INTERVAL = 15000,

        DEFAULT_NUM_IMAGES_TO_DOWNLOAD = 4000,

        /// \brief Default command timeout in milliseconds.
        DEFAULT_PROCESS_TIMEOUT = 20000

    };

    /// \brief Default instaLooter script path.
    static const std::string DEFAULT_INSTALOOTER_PATH;

    /// \brief The instaLooter filename template.
    static const std::string FILENAME_TEMPLATE;

private:
    void _loot();
    void _processLoot();

    std::string _hashtag;

    std::filesystem::path _imageStorePath;
    std::filesystem::path _basePath;
    std::filesystem::path _downloadPath;
    std::filesystem::path _instaLooterPath;

    uint64_t _numImagesToDownload = DEFAULT_NUM_IMAGES_TO_DOWNLOAD;

    uint64_t _processTimeout = DEFAULT_PROCESS_TIMEOUT;

    IO::FileExtensionFilter _fileExtensionFilter;

};


} } // ofx::InstaLooter
