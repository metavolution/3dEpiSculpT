#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <cstring>
#include <algorithm>
#include <span>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: ZERO-DEPENDENCY OSC (OPEN SOUND CONTROL) PARSER
// -----------------------------------------------------------------------------
// OSC is the international standard protocol for inter-process communication
// between music/media software (Ableton, TouchOSC, Max/MSP, Bitwig, Resolume).
// An OSC packet consists of 4-byte aligned, big-endian data:
// 1. Address pattern: e.g. "/epi/preset/select\0\0" (padded to multiple of 4)
// 2. Type tag string: e.g. ",i\0\0" (padded to multiple of 4)
// 3. Binary Arguments: 32-bit big-endian ints/floats, or padded strings.
// -----------------------------------------------------------------------------

using OscArg = std::variant<int32_t, float, std::string>;

struct OscMessage {
    std::string address;
    std::vector<OscArg> args;

    [[nodiscard]] bool empty() const noexcept { return address.empty(); }

    [[nodiscard]] int32_t getInt(std::size_t index, int32_t fallback = 0) const noexcept {
        if (index < args.size() && std::holds_alternative<int32_t>(args[index])) {
            return std::get<int32_t>(args[index]);
        }
        if (index < args.size() && std::holds_alternative<float>(args[index])) {
            return static_cast<int32_t>(std::get<float>(args[index]));
        }
        return fallback;
    }

    [[nodiscard]] float getFloat(std::size_t index, float fallback = 0.0f) const noexcept {
        if (index < args.size() && std::holds_alternative<float>(args[index])) {
            return std::get<float>(args[index]);
        }
        if (index < args.size() && std::holds_alternative<int32_t>(args[index])) {
            return static_cast<float>(std::get<int32_t>(args[index]));
        }
        return fallback;
    }

    [[nodiscard]] std::string_view getString(std::size_t index, std::string_view fallback = {}) const noexcept {
        if (index < args.size() && std::holds_alternative<std::string>(args[index])) {
            return std::get<std::string>(args[index]);
        }
        return fallback;
    }
};

class SculptureOscDecoder {
public:
    static bool parseMessage(std::span<const uint8_t> buffer, OscMessage& outMessage) noexcept {
        if (buffer.size() < 4) return false;

        std::size_t offset = 0;

        // 1. Read Address Pattern
        std::string address;
        if (!readPaddedString(buffer, offset, address)) return false;
        if (address.empty() || address[0] != '/') return false;

        outMessage.address = std::move(address);
        outMessage.args.clear();

        if (offset >= buffer.size()) {
            return true; // No arguments, just address
        }

        // 2. Read Typetag string
        std::string typeTag;
        if (!readPaddedString(buffer, offset, typeTag)) return true;
        if (typeTag.empty() || typeTag[0] != ',') return true;

        // 3. Read Arguments according to Typetags
        for (std::size_t i = 1; i < typeTag.size(); ++i) {
            char tag = typeTag[i];
            switch (tag) {
            case 'i': { // 32-bit int
                if (offset + 4 > buffer.size()) return false;
                uint32_t be = (static_cast<uint32_t>(buffer[offset]) << 24) |
                              (static_cast<uint32_t>(buffer[offset + 1]) << 16) |
                              (static_cast<uint32_t>(buffer[offset + 2]) << 8) |
                              static_cast<uint32_t>(buffer[offset + 3]);
                int32_t val = 0;
                std::memcpy(&val, &be, 4);
                outMessage.args.emplace_back(val);
                offset += 4;
                break;
            }
            case 'f': { // 32-bit IEEE float
                if (offset + 4 > buffer.size()) return false;
                uint32_t be = (static_cast<uint32_t>(buffer[offset]) << 24) |
                              (static_cast<uint32_t>(buffer[offset + 1]) << 16) |
                              (static_cast<uint32_t>(buffer[offset + 2]) << 8) |
                              static_cast<uint32_t>(buffer[offset + 3]);
                float val = 0.0f;
                std::memcpy(&val, &be, 4);
                outMessage.args.emplace_back(val);
                offset += 4;
                break;
            }
            case 's': { // Padded string
                std::string str;
                if (!readPaddedString(buffer, offset, str)) return false;
                outMessage.args.emplace_back(std::move(str));
                break;
            }
            case 'T': // True boolean
                outMessage.args.emplace_back(1);
                break;
            case 'F': // False boolean
                outMessage.args.emplace_back(0);
                break;
            case 'N': // Nil
                break;
            default:
                // Skip unknown 4-byte word
                if (offset + 4 <= buffer.size()) offset += 4;
                break;
            }
        }
        return true;
    }

private:
    static bool readPaddedString(std::span<const uint8_t> buffer, std::size_t& offset, std::string& outStr) noexcept {
        if (offset >= buffer.size()) return false;

        std::size_t start = offset;
        std::size_t nullPos = buffer.size();

        for (std::size_t i = start; i < buffer.size(); ++i) {
            if (buffer[i] == '\0') {
                nullPos = i;
                break;
            }
        }

        if (nullPos == buffer.size()) return false;

        outStr.assign(reinterpret_cast<const char*>(&buffer[start]), nullPos - start);

        // Advance past the null byte and pad to 4-byte boundary
        offset = (nullPos + 4) & ~0x03;
        return offset <= buffer.size() + 3;
    }
};

class SculptureOscEncoder {
public:
    static std::vector<uint8_t> encodeMessage(
        std::string_view address,
        const std::vector<OscArg>& args
    ) {
        std::vector<uint8_t> packet;
        writePaddedString(packet, address);

        // Build type tag string: e.g. ",ifs"
        std::string typeTags = ",";
        for (const auto& arg : args) {
            if (std::holds_alternative<int32_t>(arg)) {
                typeTags += 'i';
            } else if (std::holds_alternative<float>(arg)) {
                typeTags += 'f';
            } else if (std::holds_alternative<std::string>(arg)) {
                typeTags += 's';
            }
        }
        writePaddedString(packet, typeTags);

        // Write binary payload
        for (const auto& arg : args) {
            if (std::holds_alternative<int32_t>(arg)) {
                const auto val = std::get<int32_t>(arg);
                writeBigEndian32(packet, static_cast<uint32_t>(val));
            } else if (std::holds_alternative<float>(arg)) {
                const auto val = std::get<float>(arg);
                uint32_t be = 0;
                std::memcpy(&be, &val, 4);
                writeBigEndian32(packet, be);
            } else if (std::holds_alternative<std::string>(arg)) {
                writePaddedString(packet, std::get<std::string>(arg));
            }
        }
        return packet;
    }

    static std::vector<uint8_t> encodeFloat(std::string_view address, float val) {
        std::vector<uint8_t> packet;
        writePaddedString(packet, address);
        writePaddedString(packet, ",f");
        uint32_t be = 0;
        std::memcpy(&be, &val, 4);
        writeBigEndian32(packet, be);
        return packet;
    }

    static std::vector<uint8_t> encodeInt(std::string_view address, int32_t val) {
        std::vector<uint8_t> packet;
        writePaddedString(packet, address);
        writePaddedString(packet, ",i");
        writeBigEndian32(packet, static_cast<uint32_t>(val));
        return packet;
    }

    static std::vector<uint8_t> encodeString(std::string_view address, std::string_view str) {
        std::vector<uint8_t> packet;
        writePaddedString(packet, address);
        writePaddedString(packet, ",s");
        writePaddedString(packet, str);
        return packet;
    }

private:
    static void writePaddedString(std::vector<uint8_t>& packet, std::string_view str) {
        packet.insert(packet.end(), str.begin(), str.end());
        packet.push_back('\0'); // Null terminator
        while (packet.size() % 4 != 0) {
            packet.push_back('\0'); // 4-byte boundary padding
        }
    }

    static void writeBigEndian32(std::vector<uint8_t>& packet, uint32_t val) {
        packet.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
        packet.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        packet.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        packet.push_back(static_cast<uint8_t>(val & 0xFF));
    }
};

} // namespace epi_sculpt
