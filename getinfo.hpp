#pragma once
#include <string>

struct hwInfo {
    std::string vendor;
    std::string renderer;
    std::string version;
    std::string glsl;
};

hwInfo getInfo();
std::string getProfile();