#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>

namespace fs = std::filesystem;

static const std::regex pattern("([a-zA-Z]+)(\\d+)");

namespace zal_utils {
/// @brief replace the number in the `pathStr` with the given `disknumber`
/// assuming that ther is only one number in the path
std::string replaceDiskNumber(const std::string& pathStr, unsigned diskNumber) {
    std::ostringstream newPath;
    for (const auto& part : fs::path(pathStr)) {
        newPath << fs::path::preferred_separator;

        std::smatch match;
        std::string partStr = part.string();
        if (std::regex_match(partStr, match, pattern)) {
            newPath << match[1].str() << diskNumber;
        } else {
            newPath << part;
        }
    }
    return newPath.str();
}

/// @brief replace the number in the `pathStr` with the given `diskNumber` but only at the `rindex`th part
/// @param pathStr 
/// @param diskNumber 
/// @param rindex usually, it is negative, which means the index from the end. e.g. -1 means the last part
/// @return replaced newPath
std::string replaceDiskNumber(const std::string& pathStr, unsigned diskNumber, int rindex) {
    fs::path path(pathStr);
    std::ostringstream newPath;
    std::vector<std::string> parts;

    for (const auto& part : path) {
        parts.push_back(part.string());
    }
    std::cout << parts.size() << std::endl;

    for (int i=0; i<parts.size(); i++) {
        if (i > 1) {
            // newPath << fs::path::preferred_separator;  // HACK not a good idea on windows, because windows is using "\\" as separator
            newPath << "/";
        }
        if (i == parts.size() + rindex) {
            std::smatch match;
            if (std::regex_match(parts[i], match, pattern)) {
                newPath << match[1].str() << diskNumber;
            } else {
                newPath << parts[i];
            }
        } else {
            newPath << parts[i];
        }
    }
   
    return newPath.str();
}
}  // namespace zal_utils