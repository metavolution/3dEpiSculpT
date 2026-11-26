#pragma once

#include <ostream>
#include <string_view>

namespace epi_sculpt::inspection_json_detail {

// Linux identities/revisions may contain arbitrary bytes, not valid UTF-8.
// Hex is an explicit byte encoding; JSON string escaping is for display labels.
inline void bytes(std::ostream& out, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (const unsigned char c : value) out << hex[c >> 4U] << hex[c & 15U];
    out << '"';
}

inline void label(std::ostream& out, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (const unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << static_cast<char>(c);
        else if (c < 0x20U || c >= 0x7fU) out << "\\u00" << hex[c >> 4U] << hex[c & 15U];
        else out << static_cast<char>(c);
    }
    out << '"';
}

} // namespace epi_sculpt::inspection_json_detail
