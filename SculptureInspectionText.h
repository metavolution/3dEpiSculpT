#pragma once

#include <ostream>
#include <string_view>

namespace epi_sculpt {

// Keep one physical report row even for metadata containing control bytes.
// This is the inspector's byte-oriented text convention, independent of JSON.
inline void quoteInspectionText(std::ostream& out, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (const unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << static_cast<char>(c);
        else if (c < 0x20U || c == 0x7fU) out << "\\x" << hex[c >> 4U] << hex[c & 15U];
        else out << static_cast<char>(c);
    }
    out << '"';
}

} // namespace epi_sculpt
