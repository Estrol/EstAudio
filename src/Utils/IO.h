#ifndef __IO_H_
#define __IO_H_

#include <filesystem>
#include <fstream>
#include <string>

std::vector<char> ReadFile(const std::filesystem::path &path);
bool              WriteFile(const std::filesystem::path &path, const std::vector<char> &data);

std::string HashFile(const std::filesystem::path &path);
std::string HashBuffer(const void *data, size_t size);

template <typename T>
std::string HashVectorData(const std::vector<T> &data)
{
    MD5Context ctx;
    md5Init(&ctx);

    md5Update(&ctx, (uint8_t *)data.data(), data.size() * sizeof(T));

    md5Finalize(&ctx);

    char result[16];
    memcpy(result, ctx.digest, 16);

    std::string hash;
    for (int i = 0; i < 16; i++) {
        hash += std::to_string(result[i]);
    }

    return hash;
}

#endif