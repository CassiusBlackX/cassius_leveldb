#include <iostream>
#include <string>
#include <sstream>
#include <random>

namespace zal_utils {
    std::string gen_key(size_t index) {
    std::ostringstream s;
    s << index;
    return s.str();
}

std::string gen_value(std::mt19937& rng, size_t len) {
    static const std::string alphabet("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    static std::uniform_int_distribution<int> dist(0, alphabet.size() - 1);

    std::string value;
    value.resize(len);
    for (auto i = 0; i < value.size(); ++i) {
        value[i] = alphabet[dist(rng) % alphabet.size()];
    }

    return value;
}
}
