#include "U32Str.h"
#include <stdexcept>

namespace LangMgr
{
    std::string u32strToUtf8str(const char32_t &ch32) {
        std::string utf8str;

        if (ch32 <= 0x7F) {
            // 1-byte UTF-8
            utf8str.push_back(static_cast<char>(ch32));
        } else if (ch32 <= 0x7FF) {
            // 2-byte UTF-8
            utf8str.push_back(static_cast<char>(0xC0 | ch32 >> 6));
            utf8str.push_back(static_cast<char>(0x80 | ch32 & 0x3F));
        } else if (ch32 <= 0xFFFF) {
            // 3-byte UTF-8
            utf8str.push_back(static_cast<char>(0xE0 | ch32 >> 12));
            utf8str.push_back(static_cast<char>(0x80 | ch32 >> 6 & 0x3F));
            utf8str.push_back(static_cast<char>(0x80 | ch32 & 0x3F));
        } else if (ch32 <= 0x10FFFF) {
            // 4-byte UTF-8
            utf8str.push_back(static_cast<char>(0xF0 | ch32 >> 18));
            utf8str.push_back(static_cast<char>(0x80 | ch32 >> 12 & 0x3F));
            utf8str.push_back(static_cast<char>(0x80 | ch32 >> 6 & 0x3F));
            utf8str.push_back(static_cast<char>(0x80 | ch32 & 0x3F));
        } else {
            throw std::invalid_argument("Invalid UTF-32 codepoint");
        }

        return utf8str;
    }

    std::string u32strToUtf8str(const std::u32string &u32str) {
        std::string utf8str;
        utf8str.reserve(u32str.size() * 3);

        for (const char32_t ch32 : u32str) {
            if (ch32 <= 0x7F) {
                // 1-byte sequence
                utf8str.push_back(static_cast<char>(ch32));
            } else if (ch32 <= 0x7FF) {
                // 2-byte sequence
                utf8str.push_back(static_cast<char>(0xC0 | ch32 >> 6));
                utf8str.push_back(static_cast<char>(0x80 | ch32 & 0x3F));
            } else if (ch32 <= 0xFFFF) {
                // 3-byte sequence
                utf8str.push_back(static_cast<char>(0xE0 | ch32 >> 12));
                utf8str.push_back(static_cast<char>(0x80 | ch32 >> 6 & 0x3F));
                utf8str.push_back(static_cast<char>(0x80 | ch32 & 0x3F));
            } else if (ch32 <= 0x10FFFF) {
                // 4-byte sequence
                utf8str.push_back(static_cast<char>(0xF0 | ch32 >> 18));
                utf8str.push_back(static_cast<char>(0x80 | ch32 >> 12 & 0x3F));
                utf8str.push_back(static_cast<char>(0x80 | ch32 >> 6 & 0x3F));
                utf8str.push_back(static_cast<char>(0x80 | ch32 & 0x3F));
            } else {
                throw std::invalid_argument("Invalid UTF-32 codepoint");
            }
        }

        return utf8str;
    }

    std::u32string utf8strToU32str(const std::string &utf8str) {
        std::u32string u32str;
        u32str.reserve(utf8str.size());

        size_t i = 0;
        while (i < utf8str.size()) {
            const unsigned char c = utf8str[i];
            char32_t codepoint;

            if (c < 0x80) {
                // 1-byte sequence
                codepoint = c;
                i += 1;
            } else if (c < 0xE0) {
                // 2-byte sequence
                if (i + 1 >= utf8str.size())
                    throw std::invalid_argument("Invalid UTF-8 sequence");
                codepoint = (c & 0x1F) << 6 | utf8str[i + 1] & 0x3F;
                i += 2;
            } else if (c < 0xF0) {
                // 3-byte sequence
                if (i + 2 >= utf8str.size())
                    throw std::invalid_argument("Invalid UTF-8 sequence");
                codepoint = (c & 0x0F) << 12 | (utf8str[i + 1] & 0x3F) << 6 | utf8str[i + 2] & 0x3F;
                i += 3;
            } else if (c < 0xF8) {
                // 4-byte sequence
                if (i + 3 >= utf8str.size())
                    throw std::invalid_argument("Invalid UTF-8 sequence");
                codepoint = (c & 0x07) << 18 | (utf8str[i + 1] & 0x3F) << 12 | (utf8str[i + 2] & 0x3F) << 6 |
                    utf8str[i + 3] & 0x3F;
                i += 4;
            } else {
                throw std::invalid_argument("Invalid UTF-8 sequence");
            }

            if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) { // 代理对区域
                throw std::invalid_argument("Invalid Unicode codepoint");
            }

            u32str.push_back(codepoint);
        }

        return u32str;
    }
} // namespace LangMgr
