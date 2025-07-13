#include "Images.h"
#include "noimage_old.h"
#include "noimage_new.h"

#include <fstream>
#include "Util/Logger.hpp"

namespace {
void WriteNewFile(const std::filesystem::path& file) {
    std::ofstream(file.string(), std::ios::binary).write(sNoImageNewData, sNoImageNewSize);
}
}

void Images::Update(const std::filesystem::path& file) {
    if (!std::filesystem::exists(file)) {
        LOG(Info, "File {} was missing, creating...", file.stem().string());
        WriteNewFile(file);
        return;
    }
    uintmax_t fileSize = std::filesystem::file_size(file);
    if (fileSize == sNoImageOldSize) {
        LOG(Info, "Detected original {}: Replacing for Enhanced compatibility...", file.stem().string());
        std::filesystem::remove(file);
        WriteNewFile(file);
        return;
    }
    if (fileSize == sNoImageNewSize) {
        LOG(Debug, "Detected updated {}, do nothing", file.stem().string());
        return;
    }
    LOG(Debug, "Detected custom {}, do nothing", file.stem().string());
}
