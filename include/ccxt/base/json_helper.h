#pragma once

#include <nlohmann/json.hpp>
#include "types.h"

namespace ccxt {

using json = nlohmann::json;

// Helper functions for JSON operations
inline String get_string(const json& j, const char* key, const String& default_value = "") {
    return j.contains(key) ? j[key].get<String>() : default_value;
}

inline double get_double(const json& j, const char* key, double default_value = 0.0) {
    return j.contains(key) ? j[key].get<double>() : default_value;
}

inline int get_int(const json& j, const char* key, int default_value = 0) {
    return j.contains(key) ? j[key].get<int>() : default_value;
}

inline bool get_bool(const json& j, const char* key, bool default_value = false) {
    return j.contains(key) ? j[key].get<bool>() : default_value;
}

} // namespace ccxt
