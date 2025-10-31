#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "thirdparty/miniz.h"

namespace miniz_cpp {

class zip_exception : public std::runtime_error {
public:
    explicit zip_exception(const std::string& message)
        : std::runtime_error(message) {}
};

class zip_file {
public:
    zip_file() {
        std::memset(&archive_, 0, sizeof(archive_));
    }

    ~zip_file() {
        close();
    }

    zip_file(const zip_file&) = delete;
    zip_file& operator=(const zip_file&) = delete;

    zip_file(zip_file&& other) noexcept {
        *this = std::move(other);
    }

    zip_file& operator=(zip_file&& other) noexcept {
        if (this != &other) {
            close();
            archive_ = other.archive_;
            buffer_ = std::move(other.buffer_);
            other.archive_.m_pState = nullptr;
        }
        return *this;
    }

    bool load(const std::filesystem::path& path) {
        close();

        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) {
            return false;
        }

        std::streamoff fileSize = input.tellg();
        if (fileSize <= 0) {
            return false;
        }

        buffer_.resize(static_cast<size_t>(fileSize));
        input.seekg(0, std::ios::beg);
        if (!input.read(reinterpret_cast<char*>(buffer_.data()), buffer_.size())) {
            buffer_.clear();
            return false;
        }

        std::memset(&archive_, 0, sizeof(archive_));
        if (!mz_zip_reader_init_mem(&archive_, buffer_.data(), buffer_.size(), 0)) {
            buffer_.clear();
            archive_.m_pState = nullptr;
            return false;
        }

        return true;
    }

    void close() {
        if (archive_.m_pState) {
            mz_zip_reader_end(&archive_);
            archive_.m_pState = nullptr;
        }
        buffer_.clear();
    }

    mz_uint get_num_files() const {
        return archive_.m_pState ? mz_zip_reader_get_num_files(const_cast<mz_zip_archive*>(&archive_)) : 0;
    }

    bool file_stat(mz_uint index, mz_zip_archive_file_stat& stat) const {
        if (!archive_.m_pState) {
            return false;
        }
        return mz_zip_reader_file_stat(const_cast<mz_zip_archive*>(&archive_), index, &stat) != 0;
    }

    std::vector<char> read(mz_uint index) const {
        if (!archive_.m_pState) {
            throw zip_exception("zip archive is not open");
        }
        size_t uncompressedSize = 0;
        void* extracted = mz_zip_reader_extract_to_heap(const_cast<mz_zip_archive*>(&archive_), index, &uncompressedSize, 0);
        if (!extracted) {
            throw zip_exception("failed to extract file from archive");
        }

        std::vector<char> result(static_cast<char*>(extracted), static_cast<char*>(extracted) + uncompressedSize);
        mz_free(extracted);
        return result;
    }

private:
    mutable mz_zip_archive archive_{};
    std::vector<uint8_t> buffer_{};
};

} // namespace miniz_cpp
