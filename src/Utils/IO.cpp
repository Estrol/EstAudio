#include "IO.h"
#include "MD5.h"

std::vector<char> ReadFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> data(size);
    file.read(data.data(), size);

    return data;
}

bool WriteFile(const std::filesystem::path &path, const std::vector<char> &data)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(data.data(), data.size());
    return true;
}

std::string HashFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    MD5Context ctx;
    md5Init(&ctx);

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        md5Update(&ctx, (uint8_t *)buffer, file.gcount());
    }

    md5Finalize(&ctx);

    char result[16];
    memcpy(result, ctx.digest, 16);

    std::string hash;
    for (int i = 0; i < 16; i++) {
        hash += std::to_string(result[i]);
    }

    return hash;
}

std::string HashBuffer(const void *data, size_t size)
{
    MD5Context ctx;
    md5Init(&ctx);
    md5Update(&ctx, (uint8_t *)data, size);
    md5Finalize(&ctx);

    char result[16];
    memcpy(result, ctx.digest, 16);

    std::string hash;
    for (int i = 0; i < 16; i++) {
        hash += std::to_string(result[i]);
    }

    return hash;
}