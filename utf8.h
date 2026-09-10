#pragma once
// Minimal UTF-8 + Latin case utilities for Scalpel.
// Zero-dependency; covers ASCII + Latin-1 Supplement + Latin Extended-A,
// which is sufficient for Irish (á é í ó ú, Á É Í Ó Ú) and general
// European text. Codepoints outside that range are classified OTHER and
// case-invariant. Extend here if a corpus needs more.
#include <cstdint>
#include <string>
#include <vector>

namespace utf8 {

// Decode the codepoint starting at byte i; writes its byte length to len.
// Invalid sequences decode as the single byte (len=1), keeping the
// tokenizer single-pass and total.
inline uint32_t decode(const std::string& s, size_t i, size_t& len) {
    unsigned char b0 = s[i];
    if (b0 < 0x80) { len = 1; return b0; }
    auto cont = [&](size_t k) {
        return i + k < s.size() && (static_cast<unsigned char>(s[i + k]) & 0xC0) == 0x80;
    };
    if ((b0 & 0xE0) == 0xC0 && cont(1)) {
        len = 2;
        return ((b0 & 0x1Fu) << 6) | (s[i + 1] & 0x3Fu);
    }
    if ((b0 & 0xF0) == 0xE0 && cont(1) && cont(2)) {
        len = 3;
        return ((b0 & 0x0Fu) << 12) | ((s[i + 1] & 0x3Fu) << 6) | (s[i + 2] & 0x3Fu);
    }
    if ((b0 & 0xF8) == 0xF0 && cont(1) && cont(2) && cont(3)) {
        len = 4;
        return ((b0 & 0x07u) << 18) | ((s[i + 1] & 0x3Fu) << 12)
             | ((s[i + 2] & 0x3Fu) << 6) | (s[i + 3] & 0x3Fu);
    }
    len = 1;
    return b0;
}

inline void append(std::string& s, uint32_t cp) {
    if (cp < 0x80) { s += static_cast<char>(cp); }
    else if (cp < 0x800) {
        s += static_cast<char>(0xC0 | (cp >> 6));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

inline std::vector<uint32_t> to_codepoints(const std::string& s) {
    std::vector<uint32_t> out;
    size_t i = 0, len;
    while (i < s.size()) { out.push_back(decode(s, i, len)); i += len; }
    return out;
}

inline std::string from_codepoints(const std::vector<uint32_t>& cps) {
    std::string out;
    for (uint32_t cp : cps) append(out, cp);
    return out;
}

// ---- case (ASCII + Latin-1; Latin Ext-A pairs are even/odd) ----------------

inline bool is_upper(uint32_t cp) {
    if (cp >= 'A' && cp <= 'Z') return true;
    if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return true;          // À-Þ \ ×
    if (cp >= 0x100 && cp <= 0x177) return (cp % 2) == 0;             // Ext-A pairs
    return false;
}

inline bool is_lower(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') return true;
    if (cp >= 0xDF && cp <= 0xFF && cp != 0xF7) return true;          // ß-ÿ \ ÷
    if (cp >= 0x100 && cp <= 0x177) return (cp % 2) == 1;
    return false;
}

inline uint32_t to_lower(uint32_t cp) {
    if (cp >= 'A' && cp <= 'Z') return cp + 32;
    if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return cp + 32;
    if (cp >= 0x100 && cp <= 0x177 && (cp % 2) == 0) return cp + 1;
    return cp;
}

inline uint32_t to_upper(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') return cp - 32;
    if (cp >= 0xE0 && cp <= 0xFE && cp != 0xF7) return cp - 32;
    if (cp >= 0x100 && cp <= 0x177 && (cp % 2) == 1) return cp - 1;
    return cp;
}

inline bool is_letter(uint32_t cp) {
    return is_upper(cp) || is_lower(cp) || cp == 0xDF /*ß*/;
}

}  // namespace utf8
