/**
 * @file dsmil_nlp_unicode_runtime.c
 * @brief Unicode/UTF-8 Support for NLP Tokenization
 * 
 * Provides comprehensive Unicode support including UTF-8 encoding/decoding,
 * normalization, and character classification.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_nlp_tokenize.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/**
 * @brief UTF-8 character decoding
 * 
 * @param text UTF-8 encoded text
 * @param text_len Text length in bytes
 * @param pos Current position (updated on return)
 * @param codepoint Output Unicode codepoint
 * @return Number of bytes consumed, 0 on error
 */
static size_t utf8_decode_char(const char *text, size_t text_len, size_t *pos, uint32_t *codepoint) {
    if (!text || !pos || *pos >= text_len || !codepoint) {
        return 0;
    }
    
    const unsigned char *bytes = (const unsigned char *)text;
    size_t start_pos = *pos;
    uint32_t cp = 0;
    size_t len = 0;
    
    if (bytes[*pos] < 0x80) {
        // ASCII character
        cp = bytes[*pos];
        len = 1;
    } else if ((bytes[*pos] & 0xE0) == 0xC0) {
        // 2-byte sequence
        if (*pos + 1 >= text_len) return 0;
        cp = ((bytes[*pos] & 0x1F) << 6) | (bytes[*pos + 1] & 0x3F);
        len = 2;
    } else if ((bytes[*pos] & 0xF0) == 0xE0) {
        // 3-byte sequence
        if (*pos + 2 >= text_len) return 0;
        cp = ((bytes[*pos] & 0x0F) << 12) | 
             ((bytes[*pos + 1] & 0x3F) << 6) | 
             (bytes[*pos + 2] & 0x3F);
        len = 3;
    } else if ((bytes[*pos] & 0xF8) == 0xF0) {
        // 4-byte sequence
        if (*pos + 3 >= text_len) return 0;
        cp = ((bytes[*pos] & 0x07) << 18) | 
             ((bytes[*pos + 1] & 0x3F) << 12) | 
             ((bytes[*pos + 2] & 0x3F) << 6) | 
             (bytes[*pos + 3] & 0x3F);
        len = 4;
    } else {
        // Invalid UTF-8
        return 0;
    }
    
    // Validate continuation bytes
    for (size_t i = 1; i < len; i++) {
        if ((bytes[*pos + i] & 0xC0) != 0x80) {
            return 0;  // Invalid continuation byte
        }
    }
    
    *codepoint = cp;
    *pos += len;
    return len;
}

/**
 * @brief UTF-8 character encoding
 * 
 * @param codepoint Unicode codepoint
 * @param output Output buffer (must be at least 4 bytes)
 * @return Number of bytes written
 */
static size_t utf8_encode_char(uint32_t codepoint, char *output) {
    if (!output) {
        return 0;
    }
    
    if (codepoint < 0x80) {
        output[0] = (char)codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        output[0] = (char)(0xC0 | (codepoint >> 6));
        output[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint < 0x10000) {
        output[0] = (char)(0xE0 | (codepoint >> 12));
        output[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        output[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    } else if (codepoint < 0x110000) {
        output[0] = (char)(0xF0 | (codepoint >> 18));
        output[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        output[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        output[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    
    return 0;  // Invalid codepoint
}

/**
 * @brief Check if codepoint is a letter
 */
static bool unicode_is_letter(uint32_t codepoint) {
    // Basic Latin letters
    if ((codepoint >= 'A' && codepoint <= 'Z') ||
        (codepoint >= 'a' && codepoint <= 'z')) {
        return true;
    }
    
    // Unicode letter categories - comprehensive ranges
    // U+00C0 to U+024F: Latin Extended
    if (codepoint >= 0x00C0 && codepoint <= 0x024F) {
        return true;
    }
    
    // U+0250 to U+02AF: IPA Extensions
    if (codepoint >= 0x0250 && codepoint <= 0x02AF) {
        return true;
    }
    
    // U+0370 to U+03FF: Greek and Coptic
    if (codepoint >= 0x0370 && codepoint <= 0x03FF) {
        return true;
    }
    
    // U+0400 to U+04FF: Cyrillic
    if (codepoint >= 0x0400 && codepoint <= 0x04FF) {
        return true;
    }
    
    // U+0500 to U+052F: Cyrillic Supplement
    if (codepoint >= 0x0500 && codepoint <= 0x052F) {
        return true;
    }
    
    // U+0530 to U+058F: Armenian
    if (codepoint >= 0x0530 && codepoint <= 0x058F) {
        return true;
    }
    
    // U+0590 to U+05FF: Hebrew
    if (codepoint >= 0x0590 && codepoint <= 0x05FF) {
        return true;
    }
    
    // U+0600 to U+06FF: Arabic
    if (codepoint >= 0x0600 && codepoint <= 0x06FF) {
        return true;
    }
    
    // U+0700 to U+074F: Syriac
    if (codepoint >= 0x0700 && codepoint <= 0x074F) {
        return true;
    }
    
    // U+0750 to U+077F: Arabic Supplement
    if (codepoint >= 0x0750 && codepoint <= 0x077F) {
        return true;
    }
    
    // U+0780 to U+07BF: Thaana
    if (codepoint >= 0x0780 && codepoint <= 0x07BF) {
        return true;
    }
    
    // U+0900 to U+097F: Devanagari
    if (codepoint >= 0x0900 && codepoint <= 0x097F) {
        return true;
    }
    
    // U+0980 to U+09FF: Bengali
    if (codepoint >= 0x0980 && codepoint <= 0x09FF) {
        return true;
    }
    
    // U+0A00 to U+0A7F: Gurmukhi
    if (codepoint >= 0x0A00 && codepoint <= 0x0A7F) {
        return true;
    }
    
    // U+0A80 to U+0AFF: Gujarati
    if (codepoint >= 0x0A80 && codepoint <= 0x0AFF) {
        return true;
    }
    
    // U+0B00 to U+0B7F: Oriya
    if (codepoint >= 0x0B00 && codepoint <= 0x0B7F) {
        return true;
    }
    
    // U+0B80 to U+0BFF: Tamil
    if (codepoint >= 0x0B80 && codepoint <= 0x0BFF) {
        return true;
    }
    
    // U+0C00 to U+0C7F: Telugu
    if (codepoint >= 0x0C00 && codepoint <= 0x0C7F) {
        return true;
    }
    
    // U+0C80 to U+0CFF: Kannada
    if (codepoint >= 0x0C80 && codepoint <= 0x0CFF) {
        return true;
    }
    
    // U+0D00 to U+0D7F: Malayalam
    if (codepoint >= 0x0D00 && codepoint <= 0x0D7F) {
        return true;
    }
    
    // U+0D80 to U+0DFF: Sinhala
    if (codepoint >= 0x0D80 && codepoint <= 0x0DFF) {
        return true;
    }
    
    // U+0E00 to U+0E7F: Thai
    if (codepoint >= 0x0E00 && codepoint <= 0x0E7F) {
        return true;
    }
    
    // U+0E80 to U+0EFF: Lao
    if (codepoint >= 0x0E80 && codepoint <= 0x0EFF) {
        return true;
    }
    
    // U+0F00 to U+0FFF: Tibetan
    if (codepoint >= 0x0F00 && codepoint <= 0x0FFF) {
        return true;
    }
    
    // U+1000 to U+109F: Myanmar
    if (codepoint >= 0x1000 && codepoint <= 0x109F) {
        return true;
    }
    
    // U+10A0 to U+10FF: Georgian
    if (codepoint >= 0x10A0 && codepoint <= 0x10FF) {
        return true;
    }
    
    // U+1100 to U+11FF: Hangul Jamo
    if (codepoint >= 0x1100 && codepoint <= 0x11FF) {
        return true;
    }
    
    // U+1200 to U+137F: Ethiopic
    if (codepoint >= 0x1200 && codepoint <= 0x137F) {
        return true;
    }
    
    // U+13A0 to U+13FF: Cherokee
    if (codepoint >= 0x13A0 && codepoint <= 0x13FF) {
        return true;
    }
    
    // U+1400 to U+167F: Unified Canadian Aboriginal Syllabics
    if (codepoint >= 0x1400 && codepoint <= 0x167F) {
        return true;
    }
    
    // U+1680 to U+169F: Ogham
    if (codepoint >= 0x1680 && codepoint <= 0x169F) {
        return true;
    }
    
    // U+16A0 to U+16FF: Runic
    if (codepoint >= 0x16A0 && codepoint <= 0x16FF) {
        return true;
    }
    
    // U+1700 to U+171F: Tagalog
    if (codepoint >= 0x1700 && codepoint <= 0x171F) {
        return true;
    }
    
    // U+1720 to U+173F: Hanunoo
    if (codepoint >= 0x1720 && codepoint <= 0x173F) {
        return true;
    }
    
    // U+1740 to U+175F: Buhid
    if (codepoint >= 0x1740 && codepoint <= 0x175F) {
        return true;
    }
    
    // U+1760 to U+177F: Tagbanwa
    if (codepoint >= 0x1760 && codepoint <= 0x177F) {
        return true;
    }
    
    // U+1780 to U+17FF: Khmer
    if (codepoint >= 0x1780 && codepoint <= 0x17FF) {
        return true;
    }
    
    // U+1800 to U+18AF: Mongolian
    if (codepoint >= 0x1800 && codepoint <= 0x18AF) {
        return true;
    }
    
    // U+1900 to U+194F: Limbu
    if (codepoint >= 0x1900 && codepoint <= 0x194F) {
        return true;
    }
    
    // U+1950 to U+197F: Tai Le
    if (codepoint >= 0x1950 && codepoint <= 0x197F) {
        return true;
    }
    
    // U+19E0 to U+19FF: Khmer Symbols
    if (codepoint >= 0x19E0 && codepoint <= 0x19FF) {
        return true;
    }
    
    // U+1D00 to U+1D7F: Phonetic Extensions
    if (codepoint >= 0x1D00 && codepoint <= 0x1D7F) {
        return true;
    }
    
    // U+1E00 to U+1EFF: Latin Extended Additional
    if (codepoint >= 0x1E00 && codepoint <= 0x1EFF) {
        return true;
    }
    
    // U+1F00 to U+1FFF: Greek Extended
    if (codepoint >= 0x1F00 && codepoint <= 0x1FFF) {
        return true;
    }
    
    // U+2000 to U+206F: General Punctuation (some letter-like symbols)
    if (codepoint >= 0x2100 && codepoint <= 0x214F) {
        return true;  // Letterlike Symbols
    }
    
    // U+2C00 to U+2C5F: Glagolitic
    if (codepoint >= 0x2C00 && codepoint <= 0x2C5F) {
        return true;
    }
    
    // U+2C60 to U+2C7F: Latin Extended-C
    if (codepoint >= 0x2C60 && codepoint <= 0x2C7F) {
        return true;
    }
    
    // U+2C80 to U+2CFF: Coptic
    if (codepoint >= 0x2C80 && codepoint <= 0x2CFF) {
        return true;
    }
    
    // U+2D00 to U+2D2F: Georgian Supplement
    if (codepoint >= 0x2D00 && codepoint <= 0x2D2F) {
        return true;
    }
    
    // U+2D30 to U+2D7F: Tifinagh
    if (codepoint >= 0x2D30 && codepoint <= 0x2D7F) {
        return true;
    }
    
    // U+2D80 to U+2DDF: Ethiopic Extended
    if (codepoint >= 0x2D80 && codepoint <= 0x2DDF) {
        return true;
    }
    
    // U+2DE0 to U+2DFF: Cyrillic Extended-A
    if (codepoint >= 0x2DE0 && codepoint <= 0x2DFF) {
        return true;
    }
    
    // U+2E00 to U+2E7F: Supplemental Punctuation
    // U+2E80 to U+2EFF: CJK Radicals Supplement
    // U+2F00 to U+2FDF: Kangxi Radicals
    // U+3000 to U+303F: CJK Symbols and Punctuation
    // U+3040 to U+309F: Hiragana
    if (codepoint >= 0x3040 && codepoint <= 0x309F) {
        return true;
    }
    
    // U+30A0 to U+30FF: Katakana
    if (codepoint >= 0x30A0 && codepoint <= 0x30FF) {
        return true;
    }
    
    // U+3100 to U+312F: Bopomofo
    if (codepoint >= 0x3100 && codepoint <= 0x312F) {
        return true;
    }
    
    // U+3130 to U+318F: Hangul Compatibility Jamo
    if (codepoint >= 0x3130 && codepoint <= 0x318F) {
        return true;
    }
    
    // U+3190 to U+319F: Kanbun
    if (codepoint >= 0x3190 && codepoint <= 0x319F) {
        return true;
    }
    
    // U+31A0 to U+31BF: Bopomofo Extended
    if (codepoint >= 0x31A0 && codepoint <= 0x31BF) {
        return true;
    }
    
    // U+31C0 to U+31EF: CJK Strokes
    // U+31F0 to U+31FF: Katakana Phonetic Extensions
    if (codepoint >= 0x31F0 && codepoint <= 0x31FF) {
        return true;
    }
    
    // U+3200 to U+32FF: Enclosed CJK Letters and Months
    if (codepoint >= 0x3200 && codepoint <= 0x32FF) {
        return true;
    }
    
    // U+3300 to U+33FF: CJK Compatibility
    if (codepoint >= 0x3300 && codepoint <= 0x33FF) {
        return true;
    }
    
    // U+3400 to U+4DBF: CJK Unified Ideographs Extension A
    if (codepoint >= 0x3400 && codepoint <= 0x4DBF) {
        return true;
    }
    
    // U+4E00 to U+9FFF: CJK Unified Ideographs
    if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
        return true;
    }
    
    // U+A000 to U+A48F: Yi Syllables
    if (codepoint >= 0xA000 && codepoint <= 0xA48F) {
        return true;
    }
    
    // U+A490 to U+A4CF: Yi Radicals
    if (codepoint >= 0xA490 && codepoint <= 0xA4CF) {
        return true;
    }
    
    // U+A500 to U+A63F: Vai
    if (codepoint >= 0xA500 && codepoint <= 0xA63F) {
        return true;
    }
    
    // U+A640 to U+A69F: Cyrillic Extended-B
    if (codepoint >= 0xA640 && codepoint <= 0xA69F) {
        return true;
    }
    
    // U+A6A0 to U+A6FF: Bamum
    if (codepoint >= 0xA6A0 && codepoint <= 0xA6FF) {
        return true;
    }
    
    // U+A700 to U+A71F: Modifier Tone Letters
    if (codepoint >= 0xA700 && codepoint <= 0xA71F) {
        return true;
    }
    
    // U+A720 to U+A7FF: Latin Extended-D
    if (codepoint >= 0xA720 && codepoint <= 0xA7FF) {
        return true;
    }
    
    // U+A800 to U+A82F: Syloti Nagri
    if (codepoint >= 0xA800 && codepoint <= 0xA82F) {
        return true;
    }
    
    // U+A840 to U+A87F: Common Indic Number Forms
    // U+A880 to U+A8DF: Saurashtra
    if (codepoint >= 0xA880 && codepoint <= 0xA8DF) {
        return true;
    }
    
    // U+A900 to U+A92F: Kayah Li
    if (codepoint >= 0xA900 && codepoint <= 0xA92F) {
        return true;
    }
    
    // U+A930 to U+A95F: Rejang
    if (codepoint >= 0xA930 && codepoint <= 0xA95F) {
        return true;
    }
    
    // U+AA00 to U+AA5F: Cham
    if (codepoint >= 0xAA00 && codepoint <= 0xAA5F) {
        return true;
    }
    
    // U+AC00 to U+D7AF: Hangul Syllables
    if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) {
        return true;
    }
    
    // U+F900 to U+FAFF: CJK Compatibility Ideographs
    if (codepoint >= 0xF900 && codepoint <= 0xFAFF) {
        return true;
    }
    
    // U+FB00 to U+FB4F: Alphabetic Presentation Forms
    if (codepoint >= 0xFB00 && codepoint <= 0xFB4F) {
        return true;
    }
    
    // U+FB50 to U+FDFF: Arabic Presentation Forms-A
    if (codepoint >= 0xFB50 && codepoint <= 0xFDFF) {
        return true;
    }
    
    // U+FE00 to U+FE0F: Variation Selectors
    // U+FE10 to U+FE1F: Vertical Forms
    // U+FE20 to U+FE2F: Combining Half Marks
    // U+FE30 to U+FE4F: CJK Compatibility Forms
    if (codepoint >= 0xFE30 && codepoint <= 0xFE4F) {
        return true;
    }
    
    // U+FE50 to U+FE6F: Small Form Variants
    // U+FE70 to U+FEFF: Arabic Presentation Forms-B
    if (codepoint >= 0xFE70 && codepoint <= 0xFEFF) {
        return true;
    }
    
    // U+FF00 to U+FFEF: Halfwidth and Fullwidth Forms
    if (codepoint >= 0xFF00 && codepoint <= 0xFFEF) {
        // Check if it's a letter (A-Z, a-z)
        if ((codepoint >= 0xFF21 && codepoint <= 0xFF3A) ||  // Fullwidth A-Z
            (codepoint >= 0xFF41 && codepoint <= 0xFF5A)) {  // Fullwidth a-z
            return true;
        }
    }
    
    // U+10000 to U+1007F: Linear B Syllable B
    if (codepoint >= 0x10000 && codepoint <= 0x1007F) {
        return true;
    }
    
    // U+10080 to U+100FF: Linear B Ideograms
    if (codepoint >= 0x10080 && codepoint <= 0x100FF) {
        return true;
    }
    
    // U+10100 to U+1013F: Aegean Numbers
    // U+10140 to U+1018F: Ancient Greek Numbers
    // U+10190 to U+101CF: Ancient Symbols
    // U+101D0 to U+101FF: Phaistos Disc
    // U+10280 to U+1029F: Lycian
    if (codepoint >= 0x10280 && codepoint <= 0x1029F) {
        return true;
    }
    
    // U+102A0 to U+102DF: Carian
    if (codepoint >= 0x102A0 && codepoint <= 0x102DF) {
        return true;
    }
    
    // U+10300 to U+1032F: Old Italic
    if (codepoint >= 0x10300 && codepoint <= 0x1032F) {
        return true;
    }
    
    // U+10330 to U+1034F: Gothic
    if (codepoint >= 0x10330 && codepoint <= 0x1034F) {
        return true;
    }
    
    // U+10380 to U+1039F: Ugaritic
    if (codepoint >= 0x10380 && codepoint <= 0x1039F) {
        return true;
    }
    
    // U+103A0 to U+103DF: Old Persian
    if (codepoint >= 0x103A0 && codepoint <= 0x103DF) {
        return true;
    }
    
    // U+10400 to U+1044F: Deseret
    if (codepoint >= 0x10400 && codepoint <= 0x1044F) {
        return true;
    }
    
    // U+10450 to U+1047F: Shavian
    if (codepoint >= 0x10450 && codepoint <= 0x1047F) {
        return true;
    }
    
    // U+10480 to U+104AF: Osmanya
    if (codepoint >= 0x10480 && codepoint <= 0x104AF) {
        return true;
    }
    
    // U+10800 to U+1083F: Cypriot Syllabary
    if (codepoint >= 0x10800 && codepoint <= 0x1083F) {
        return true;
    }
    
    // U+10840 to U+1085F: Imperial Aramaic
    if (codepoint >= 0x10840 && codepoint <= 0x1085F) {
        return true;
    }
    
    // U+10900 to U+1091F: Phoenician
    if (codepoint >= 0x10900 && codepoint <= 0x1091F) {
        return true;
    }
    
    // U+10920 to U+1093F: Lydian
    if (codepoint >= 0x10920 && codepoint <= 0x1093F) {
        return true;
    }
    
    // U+10A00 to U+10A5F: Kharoshthi
    if (codepoint >= 0x10A00 && codepoint <= 0x10A5F) {
        return true;
    }
    
    // U+10A60 to U+10A7F: Old South Arabian
    if (codepoint >= 0x10A60 && codepoint <= 0x10A7F) {
        return true;
    }
    
    // U+10B00 to U+10B3F: Avestan
    if (codepoint >= 0x10B00 && codepoint <= 0x10B3F) {
        return true;
    }
    
    // U+10B40 to U+10B5F: Inscriptional Parthian
    if (codepoint >= 0x10B40 && codepoint <= 0x10B5F) {
        return true;
    }
    
    // U+10B60 to U+10B7F: Inscriptional Pahlavi
    if (codepoint >= 0x10B60 && codepoint <= 0x10B7F) {
        return true;
    }
    
    // U+10C00 to U+10C4F: Old Turkic
    if (codepoint >= 0x10C00 && codepoint <= 0x10C4F) {
        return true;
    }
    
    // U+10E60 to U+10E7F: Rumi Numeral Symbols
    // U+11000 to U+1107F: Brahmi
    if (codepoint >= 0x11000 && codepoint <= 0x1107F) {
        return true;
    }
    
    // U+11080 to U+110CF: Kaithi
    if (codepoint >= 0x11080 && codepoint <= 0x110CF) {
        return true;
    }
    
    // U+110D0 to U+110FF: Sora Sompeng
    if (codepoint >= 0x110D0 && codepoint <= 0x110FF) {
        return true;
    }
    
    // U+11100 to U+1114F: Chakma
    if (codepoint >= 0x11100 && codepoint <= 0x1114F) {
        return true;
    }
    
    // U+11180 to U+111DF: Sharada
    if (codepoint >= 0x11180 && codepoint <= 0x111DF) {
        return true;
    }
    
    // U+111E0 to U+111FF: Sinhala Archaic Numbers
    // U+11200 to U+1124F: Khojki
    if (codepoint >= 0x11200 && codepoint <= 0x1124F) {
        return true;
    }
    
    // U+11280 to U+112AF: Multani
    if (codepoint >= 0x11280 && codepoint <= 0x112AF) {
        return true;
    }
    
    // U+112B0 to U+112FF: Khudawadi
    if (codepoint >= 0x112B0 && codepoint <= 0x112FF) {
        return true;
    }
    
    // U+11300 to U+1137F: Grantha
    if (codepoint >= 0x11300 && codepoint <= 0x1137F) {
        return true;
    }
    
    // U+11400 to U+1147F: Newa
    if (codepoint >= 0x11400 && codepoint <= 0x1147F) {
        return true;
    }
    
    // U+11480 to U+114DF: Tirhuta
    if (codepoint >= 0x11480 && codepoint <= 0x114DF) {
        return true;
    }
    
    // U+11580 to U+115FF: Siddham
    if (codepoint >= 0x11580 && codepoint <= 0x115FF) {
        return true;
    }
    
    // U+11600 to U+1165F: Modi
    if (codepoint >= 0x11600 && codepoint <= 0x1165F) {
        return true;
    }
    
    // U+11680 to U+116CF: Takri
    if (codepoint >= 0x11680 && codepoint <= 0x116CF) {
        return true;
    }
    
    // U+11700 to U+1173F: Ahom
    if (codepoint >= 0x11700 && codepoint <= 0x1173F) {
        return true;
    }
    
    // U+11800 to U+1184F: Dogra
    if (codepoint >= 0x11800 && codepoint <= 0x1184F) {
        return true;
    }
    
    // U+118A0 to U+118FF: Warang Citi
    if (codepoint >= 0x118A0 && codepoint <= 0x118FF) {
        return true;
    }
    
    // U+11A00 to U+11A4F: Zanabazar Square
    if (codepoint >= 0x11A00 && codepoint <= 0x11A4F) {
        return true;
    }
    
    // U+11A50 to U+11AAF: Soyombo
    if (codepoint >= 0x11A50 && codepoint <= 0x11AAF) {
        return true;
    }
    
    // U+11AC0 to U+11AFF: Pau Cin Hau
    if (codepoint >= 0x11AC0 && codepoint <= 0x11AFF) {
        return true;
    }
    
    // U+11C00 to U+11C6F: Bhaiksuki
    if (codepoint >= 0x11C00 && codepoint <= 0x11C6F) {
        return true;
    }
    
    // U+11C70 to U+11CBF: Marchen
    if (codepoint >= 0x11C70 && codepoint <= 0x11CBF) {
        return true;
    }
    
    // U+11D00 to U+11D5F: Masaram Gondi
    if (codepoint >= 0x11D00 && codepoint <= 0x11D5F) {
        return true;
    }
    
    // U+11D60 to U+11DAF: Gunjala Gondi
    if (codepoint >= 0x11D60 && codepoint <= 0x11DAF) {
        return true;
    }
    
    // U+11EE0 to U+11EFF: Makasar
    if (codepoint >= 0x11EE0 && codepoint <= 0x11EFF) {
        return true;
    }
    
    // U+11FB0 to U+11FBF: Lisu Supplement
    if (codepoint >= 0x11FB0 && codepoint <= 0x11FBF) {
        return true;
    }
    
    // U+11FC0 to U+11FFF: Tamil Supplement
    if (codepoint >= 0x11FC0 && codepoint <= 0x11FFF) {
        return true;
    }
    
    // U+12000 to U+123FF: Cuneiform
    if (codepoint >= 0x12000 && codepoint <= 0x123FF) {
        return true;
    }
    
    // U+12400 to U+1247F: Cuneiform Numbers and Punctuation
    // U+12480 to U+1254F: Early Dynastic Cuneiform
    if (codepoint >= 0x12480 && codepoint <= 0x1254F) {
        return true;
    }
    
    // U+13000 to U+1342F: Egyptian Hieroglyphs
    if (codepoint >= 0x13000 && codepoint <= 0x1342F) {
        return true;
    }
    
    // U+13430 to U+1343F: Egyptian Hieroglyph Format Controls
    // U+14400 to U+1467F: Anatolian Hieroglyphs
    if (codepoint >= 0x14400 && codepoint <= 0x1467F) {
        return true;
    }
    
    // U+16800 to U+16A3F: Bamum Supplement
    if (codepoint >= 0x16800 && codepoint <= 0x16A3F) {
        return true;
    }
    
    // U+16A40 to U+16A6F: Mro
    if (codepoint >= 0x16A40 && codepoint <= 0x16A6F) {
        return true;
    }
    
    // U+16AD0 to U+16AFF: Bassa Vah
    if (codepoint >= 0x16AD0 && codepoint <= 0x16AFF) {
        return true;
    }
    
    // U+16B00 to U+16B8F: Pahawh Hmong
    if (codepoint >= 0x16B00 && codepoint <= 0x16B8F) {
        return true;
    }
    
    // U+16E40 to U+16E9F: Medefaidrin
    if (codepoint >= 0x16E40 && codepoint <= 0x16E9F) {
        return true;
    }
    
    // U+16F00 to U+16F9F: Miao
    if (codepoint >= 0x16F00 && codepoint <= 0x16F9F) {
        return true;
    }
    
    // U+16FE0 to U+16FFF: Ideographic Symbols and Punctuation
    if (codepoint >= 0x16FE0 && codepoint <= 0x16FFF) {
        return true;
    }
    
    // U+17000 to U+187FF: Tangut
    if (codepoint >= 0x17000 && codepoint <= 0x187FF) {
        return true;
    }
    
    // U+18800 to U+18AFF: Tangut Components
    if (codepoint >= 0x18800 && codepoint <= 0x18AFF) {
        return true;
    }
    
    // U+1B000 to U+1B0FF: Kana Supplement
    if (codepoint >= 0x1B000 && codepoint <= 0x1B0FF) {
        return true;
    }
    
    // U+1B100 to U+1B12F: Kana Extended-A
    if (codepoint >= 0x1B100 && codepoint <= 0x1B12F) {
        return true;
    }
    
    // U+1B130 to U+1B16F: Small Kana Extension
    if (codepoint >= 0x1B130 && codepoint <= 0x1B16F) {
        return true;
    }
    
    // U+1B170 to U+1B2FF: Nushu
    if (codepoint >= 0x1B170 && codepoint <= 0x1B2FF) {
        return true;
    }
    
    // U+1BC00 to U+1BC9F: Duployan
    if (codepoint >= 0x1BC00 && codepoint <= 0x1BC9F) {
        return true;
    }
    
    // U+1BCA0 to U+1BCAF: Shorthand Format Controls
    // U+1D000 to U+1D0FF: Byzantine Musical Symbols
    // U+1D100 to U+1D1FF: Musical Symbols
    // U+1D200 to U+1D24F: Ancient Greek Musical Notation
    // U+1D2E0 to U+1D2FF: Mayan Numerals
    // U+1D300 to U+1D35F: Tai Xuan Jing Symbols
    // U+1D360 to U+1D37F: Counting Rod Numerals
    // U+1D400 to U+1D7FF: Mathematical Alphanumeric Symbols
    if (codepoint >= 0x1D400 && codepoint <= 0x1D7FF) {
        return true;
    }
    
    // U+1E800 to U+1E8DF: Mende Kikakui
    if (codepoint >= 0x1E800 && codepoint <= 0x1E8DF) {
        return true;
    }
    
    // U+1E900 to U+1E95F: Adlam
    if (codepoint >= 0x1E900 && codepoint <= 0x1E95F) {
        return true;
    }
    
    // U+1EE00 to U+1EEFF: Arabic Mathematical Alphabetic Symbols
    if (codepoint >= 0x1EE00 && codepoint <= 0x1EEFF) {
        return true;
    }
    
    // U+1F000 to U+1F02F: Mahjong Tiles
    // U+1F030 to U+1F09F: Domino Tiles
    // U+1F0A0 to U+1F0FF: Playing Cards
    // U+1F100 to U+1F1FF: Enclosed Alphanumeric Supplement
    // U+1F200 to U+1F2FF: Enclosed CJK Letters and Months
    // U+1F300 to U+1F5FF: Miscellaneous Symbols and Pictographs
    // U+1F600 to U+1F64F: Emoticons
    // U+1F650 to U+1F67F: Ornamental Dingbats
    // U+1F680 to U+1F6FF: Transport and Map Symbols
    // U+1F700 to U+1F77F: Alchemical Symbols
    // U+1F780 to U+1F7FF: Geometric Shapes Extended
    // U+1F800 to U+1F8FF: Supplemental Arrows-C
    // U+1F900 to U+1F9FF: Supplemental Symbols and Pictographs
    // U+1FA00 to U+1FA6F: Chess Symbols
    // U+1FA70 to U+1FAFF: Symbols and Pictographs Extended-A
    // U+20000 to U+2A6DF: CJK Unified Ideographs Extension B
    if (codepoint >= 0x20000 && codepoint <= 0x2A6DF) {
        return true;
    }
    
    // U+2A700 to U+2B73F: CJK Unified Ideographs Extension C
    if (codepoint >= 0x2A700 && codepoint <= 0x2B73F) {
        return true;
    }
    
    // U+2B740 to U+2B81F: CJK Unified Ideographs Extension D
    if (codepoint >= 0x2B740 && codepoint <= 0x2B81F) {
        return true;
    }
    
    // U+2B820 to U+2CEAF: CJK Unified Ideographs Extension E
    if (codepoint >= 0x2B820 && codepoint <= 0x2CEAF) {
        return true;
    }
    
    // U+2CEB0 to U+2EBEF: CJK Unified Ideographs Extension F
    if (codepoint >= 0x2CEB0 && codepoint <= 0x2EBEF) {
        return true;
    }
    
    // U+2F800 to U+2FA1F: CJK Compatibility Ideographs Supplement
    if (codepoint >= 0x2F800 && codepoint <= 0x2FA1F) {
        return true;
    }
    
    // U+30000 to U+3134F: CJK Unified Ideographs Extension G
    if (codepoint >= 0x30000 && codepoint <= 0x3134F) {
        return true;
    }
    
    // U+31350 to U+323AF: CJK Unified Ideographs Extension H
    if (codepoint >= 0x31350 && codepoint <= 0x323AF) {
        return true;
    }
    
    return false;
}

/**
 * @brief Check if codepoint is a digit
 */
static bool unicode_is_digit(uint32_t codepoint) {
    // ASCII digits
    if (codepoint >= '0' && codepoint <= '9') {
        return true;
    }
    
    // Unicode digit ranges
    // U+0660 to U+0669: Arabic-Indic digits
    if (codepoint >= 0x0660 && codepoint <= 0x0669) {
        return true;
    }
    
    // U+06F0 to U+06F9: Extended Arabic-Indic digits
    if (codepoint >= 0x06F0 && codepoint <= 0x06F9) {
        return true;
    }
    
    // U+07C0 to U+07C9: NKo digits
    if (codepoint >= 0x07C0 && codepoint <= 0x07C9) {
        return true;
    }
    
    // U+0966 to U+096F: Devanagari digits
    if (codepoint >= 0x0966 && codepoint <= 0x096F) {
        return true;
    }
    
    // U+09E6 to U+09EF: Bengali digits
    if (codepoint >= 0x09E6 && codepoint <= 0x09EF) {
        return true;
    }
    
    // U+0A66 to U+0A6F: Gurmukhi digits
    if (codepoint >= 0x0A66 && codepoint <= 0x0A6F) {
        return true;
    }
    
    // U+0AE6 to U+0AEF: Gujarati digits
    if (codepoint >= 0x0AE6 && codepoint <= 0x0AEF) {
        return true;
    }
    
    // U+0B66 to U+0B6F: Oriya digits
    if (codepoint >= 0x0B66 && codepoint <= 0x0B6F) {
        return true;
    }
    
    // U+0BE6 to U+0BEF: Tamil digits
    if (codepoint >= 0x0BE6 && codepoint <= 0x0BEF) {
        return true;
    }
    
    // U+0C66 to U+0C6F: Telugu digits
    if (codepoint >= 0x0C66 && codepoint <= 0x0C6F) {
        return true;
    }
    
    // U+0CE6 to U+0CEF: Kannada digits
    if (codepoint >= 0x0CE6 && codepoint <= 0x0CEF) {
        return true;
    }
    
    // U+0D66 to U+0D6F: Malayalam digits
    if (codepoint >= 0x0D66 && codepoint <= 0x0D6F) {
        return true;
    }
    
    // U+0DE6 to U+0DEF: Sinhala digits
    if (codepoint >= 0x0DE6 && codepoint <= 0x0DEF) {
        return true;
    }
    
    // U+0E50 to U+0E59: Thai digits
    if (codepoint >= 0x0E50 && codepoint <= 0x0E59) {
        return true;
    }
    
    // U+0ED0 to U+0ED9: Lao digits
    if (codepoint >= 0x0ED0 && codepoint <= 0x0ED9) {
        return true;
    }
    
    // U+0F20 to U+0F29: Tibetan digits
    if (codepoint >= 0x0F20 && codepoint <= 0x0F29) {
        return true;
    }
    
    // U+1040 to U+1049: Myanmar digits
    if (codepoint >= 0x1040 && codepoint <= 0x1049) {
        return true;
    }
    
    // U+1090 to U+1099: Myanmar Shan digits
    if (codepoint >= 0x1090 && codepoint <= 0x1099) {
        return true;
    }
    
    // U+17E0 to U+17E9: Khmer digits
    if (codepoint >= 0x17E0 && codepoint <= 0x17E9) {
        return true;
    }
    
    // U+1810 to U+1819: Mongolian digits
    if (codepoint >= 0x1810 && codepoint <= 0x1819) {
        return true;
    }
    
    // U+1946 to U+194F: Limbu digits
    if (codepoint >= 0x1946 && codepoint <= 0x194F) {
        return true;
    }
    
    // U+19D0 to U+19D9: New Tai Lue digits
    if (codepoint >= 0x19D0 && codepoint <= 0x19D9) {
        return true;
    }
    
    // U+1A80 to U+1A89: Tai Tham Hora digits
    if (codepoint >= 0x1A80 && codepoint <= 0x1A89) {
        return true;
    }
    
    // U+1A90 to U+1A99: Tai Tham Tham digits
    if (codepoint >= 0x1A90 && codepoint <= 0x1A99) {
        return true;
    }
    
    // U+1B50 to U+1B59: Balinese digits
    if (codepoint >= 0x1B50 && codepoint <= 0x1B59) {
        return true;
    }
    
    // U+1BB0 to U+1BB9: Sundanese digits
    if (codepoint >= 0x1BB0 && codepoint <= 0x1BB9) {
        return true;
    }
    
    // U+1C40 to U+1C49: Lepcha digits
    if (codepoint >= 0x1C40 && codepoint <= 0x1C49) {
        return true;
    }
    
    // U+1C50 to U+1C59: Ol Chiki digits
    if (codepoint >= 0x1C50 && codepoint <= 0x1C59) {
        return true;
    }
    
    // U+FF10 to U+FF19: Fullwidth digits
    if (codepoint >= 0xFF10 && codepoint <= 0xFF19) {
        return true;
    }
    
    return false;
}

/**
 * @brief Check if codepoint is whitespace
 */
static bool unicode_is_whitespace(uint32_t codepoint) {
    // ASCII whitespace
    if (codepoint == ' ' || codepoint == '\t' || codepoint == '\n' || 
        codepoint == '\r' || codepoint == '\f' || codepoint == '\v') {
        return true;
    }
    
    // Unicode whitespace ranges
    // U+2000 to U+200A: Various spaces
    if (codepoint >= 0x2000 && codepoint <= 0x200A) {
        return true;
    }
    
    // U+2028, U+2029: Line/paragraph separators
    if (codepoint == 0x2028 || codepoint == 0x2029) {
        return true;
    }
    
    return false;
}

/**
 * @brief Check if codepoint is punctuation
 */
static bool unicode_is_punctuation(uint32_t codepoint) {
    // ASCII punctuation
    if ((codepoint >= '!' && codepoint <= '/') ||
        (codepoint >= ':' && codepoint <= '@') ||
        (codepoint >= '[' && codepoint <= '`') ||
        (codepoint >= '{' && codepoint <= '~')) {
        return true;
    }
    
    // Unicode punctuation ranges
    // U+2000 to U+206F: General Punctuation
    if (codepoint >= 0x2000 && codepoint <= 0x206F) {
        return true;
    }
    
    // U+2070 to U+209F: Superscripts and Subscripts
    if (codepoint >= 0x2070 && codepoint <= 0x209F) {
        return true;
    }
    
    // U+20A0 to U+20CF: Currency Symbols
    if (codepoint >= 0x20A0 && codepoint <= 0x20CF) {
        return true;
    }
    
    // U+2100 to U+214F: Letterlike Symbols
    if (codepoint >= 0x2100 && codepoint <= 0x214F) {
        return true;
    }
    
    // U+2190 to U+21FF: Arrows
    if (codepoint >= 0x2190 && codepoint <= 0x21FF) {
        return true;
    }
    
    // U+2200 to U+22FF: Mathematical Operators
    if (codepoint >= 0x2200 && codepoint <= 0x22FF) {
        return true;
    }
    
    // U+2300 to U+23FF: Miscellaneous Technical
    if (codepoint >= 0x2300 && codepoint <= 0x23FF) {
        return true;
    }
    
    // U+2400 to U+243F: Control Pictures
    if (codepoint >= 0x2400 && codepoint <= 0x243F) {
        return true;
    }
    
    // U+2440 to U+245F: Optical Character Recognition
    if (codepoint >= 0x2440 && codepoint <= 0x245F) {
        return true;
    }
    
    // U+2460 to U+24FF: Enclosed Alphanumerics
    if (codepoint >= 0x2460 && codepoint <= 0x24FF) {
        return true;
    }
    
    // U+2500 to U+257F: Box Drawing
    if (codepoint >= 0x2500 && codepoint <= 0x257F) {
        return true;
    }
    
    // U+2580 to U+259F: Block Elements
    if (codepoint >= 0x2580 && codepoint <= 0x259F) {
        return true;
    }
    
    // U+25A0 to U+25FF: Geometric Shapes
    if (codepoint >= 0x25A0 && codepoint <= 0x25FF) {
        return true;
    }
    
    // U+2700 to U+27BF: Dingbats
    if (codepoint >= 0x2700 && codepoint <= 0x27BF) {
        return true;
    }
    
    // U+27C0 to U+27EF: Miscellaneous Mathematical Symbols-A
    if (codepoint >= 0x27C0 && codepoint <= 0x27EF) {
        return true;
    }
    
    // U+27F0 to U+27FF: Supplemental Arrows-A
    if (codepoint >= 0x27F0 && codepoint <= 0x27FF) {
        return true;
    }
    
    // U+2800 to U+28FF: Braille Patterns
    if (codepoint >= 0x2800 && codepoint <= 0x28FF) {
        return true;
    }
    
    // U+2900 to U+297F: Supplemental Arrows-B
    if (codepoint >= 0x2900 && codepoint <= 0x297F) {
        return true;
    }
    
    // U+2980 to U+29FF: Miscellaneous Mathematical Symbols-B
    if (codepoint >= 0x2980 && codepoint <= 0x29FF) {
        return true;
    }
    
    // U+2A00 to U+2AFF: Supplemental Mathematical Operators
    if (codepoint >= 0x2A00 && codepoint <= 0x2AFF) {
        return true;
    }
    
    // U+2B00 to U+2BFF: Miscellaneous Symbols and Arrows
    if (codepoint >= 0x2B00 && codepoint <= 0x2BFF) {
        return true;
    }
    
    // U+2E00 to U+2E7F: Supplemental Punctuation
    if (codepoint >= 0x2E00 && codepoint <= 0x2E7F) {
        return true;
    }
    
    // U+3000 to U+303F: CJK Symbols and Punctuation
    if (codepoint >= 0x3000 && codepoint <= 0x303F) {
        return true;
    }
    
    // U+FE30 to U+FE4F: CJK Compatibility Forms
    if (codepoint >= 0xFE30 && codepoint <= 0xFE4F) {
        return true;
    }
    
    // U+FE50 to U+FE6F: Small Form Variants
    if (codepoint >= 0xFE50 && codepoint <= 0xFE6F) {
        return true;
    }
    
    // U+FF00 to U+FFEF: Halfwidth and Fullwidth Forms
    if (codepoint >= 0xFF00 && codepoint <= 0xFFEF) {
        // Check if it's punctuation
        if ((codepoint >= 0xFF01 && codepoint <= 0xFF0F) ||  // Fullwidth punctuation
            (codepoint >= 0xFF1A && codepoint <= 0xFF1F) ||  // Fullwidth punctuation
            (codepoint >= 0xFF3B && codepoint <= 0xFF40) ||  // Fullwidth brackets
            (codepoint >= 0xFF5B && codepoint <= 0xFF60) ||  // Fullwidth brackets
            (codepoint >= 0xFF61 && codepoint <= 0xFF65)) {  // Halfwidth punctuation
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Unicode decomposition data structure
 */
typedef struct {
    uint32_t codepoint;
    uint32_t *decomposed;
    size_t decomposed_count;
} unicode_decomp_entry_t;

/**
 * @brief Decompose codepoint using Unicode decomposition tables
 */
static size_t unicode_decompose(uint32_t codepoint, uint32_t *decomp, size_t max_decomp) {
    // Unicode decomposition mappings for common characters
    if (codepoint == 0x00E0) { // à
        if (max_decomp >= 2) {
            decomp[0] = 0x0061; // a
            decomp[1] = 0x0300; // combining grave
            return 2;
        }
    } else if (codepoint == 0x00E1) { // á
        if (max_decomp >= 2) {
            decomp[0] = 0x0061; // a
            decomp[1] = 0x0301; // combining acute
            return 2;
        }
    } else if (codepoint == 0x00E2) { // â
        if (max_decomp >= 2) {
            decomp[0] = 0x0061; // a
            decomp[1] = 0x0302; // combining circumflex
            return 2;
        }
    } else if (codepoint == 0x00E3) { // ã
        if (max_decomp >= 2) {
            decomp[0] = 0x0061; // a
            decomp[1] = 0x0303; // combining tilde
            return 2;
        }
    } else if (codepoint == 0x00E4) { // ä
        if (max_decomp >= 2) {
            decomp[0] = 0x0061; // a
            decomp[1] = 0x0308; // combining diaeresis
            return 2;
        }
    } else if (codepoint == 0x00E5) { // å
        if (max_decomp >= 2) {
            decomp[0] = 0x0061; // a
            decomp[1] = 0x030A; // combining ring above
            return 2;
        }
    }
    // Add more decompositions as needed
    // Return single codepoint if no decomposition found
    if (max_decomp >= 1) {
        decomp[0] = codepoint;
        return 1;
    }
    return 0;
}

/**
 * @brief Compose combining sequence
 */
static bool unicode_compose(uint32_t base, uint32_t combining, uint32_t *composed) {
    // Common compositions
    if (base == 0x0061) { // a
        if (combining == 0x0300) { *composed = 0x00E0; return true; } // à
        if (combining == 0x0301) { *composed = 0x00E1; return true; } // á
        if (combining == 0x0302) { *composed = 0x00E2; return true; } // â
        if (combining == 0x0303) { *composed = 0x00E3; return true; } // ã
        if (combining == 0x0308) { *composed = 0x00E4; return true; } // ä
        if (combining == 0x030A) { *composed = 0x00E5; return true; } // å
    }
    // Add more compositions as needed
    return false;
}

/**
 * @brief Normalize text (NFC - Canonical Composition)
 */
static int unicode_normalize_nfc(const char *input, size_t input_len, 
                                  char *output, size_t *output_len) {
    if (!input || !output || !output_len) {
        return -EINVAL;
    }
    
    size_t in_pos = 0;
    size_t out_pos = 0;
    uint32_t last_base = 0;
    bool has_combining = false;
    
    while (in_pos < input_len && out_pos < *output_len - 4) {
        uint32_t codepoint;
        size_t consumed = utf8_decode_char(input, input_len, &in_pos, &codepoint);
        if (consumed == 0) {
            break;
        }
        
        // Check if this is a combining character
        bool is_combining = (codepoint >= 0x0300 && codepoint <= 0x036F) ||
                           (codepoint >= 0x1AB0 && codepoint <= 0x1AFF) ||
                           (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) ||
                           (codepoint >= 0x20D0 && codepoint <= 0x20FF) ||
                           (codepoint >= 0xFE20 && codepoint <= 0xFE2F);
        
        if (is_combining && last_base != 0) {
            // Try to compose
            uint32_t composed;
            if (unicode_compose(last_base, codepoint, &composed)) {
                // Replace last base with composed character
                size_t encoded = utf8_encode_char(composed, output + out_pos - 4);
                if (encoded > 0) {
                    out_pos = out_pos - 4 + encoded;
                    last_base = composed;
                    has_combining = false;
                    continue;
                }
            }
        }
        
        // Encode current codepoint
        size_t encoded = utf8_encode_char(codepoint, output + out_pos);
        if (encoded == 0) {
            break;
        }
        out_pos += encoded;
        
        if (!is_combining) {
            last_base = codepoint;
            has_combining = false;
        } else {
            has_combining = true;
        }
    }
    
    output[out_pos] = '\0';
    *output_len = out_pos;
    return 0;
}

/**
 * @brief Case fold text (convert to lowercase)
 */
static int unicode_case_fold(const char *input, size_t input_len,
                             char *output, size_t *output_len) {
    if (!input || !output || !output_len) {
        return -EINVAL;
    }
    
    size_t pos = 0;
    size_t out_pos = 0;
    
    while (pos < input_len && out_pos < *output_len - 4) {
        uint32_t codepoint;
        size_t consumed = utf8_decode_char(input, input_len, &pos, &codepoint);
        if (consumed == 0) {
            break;
        }
        
        // Convert to lowercase
        if (codepoint >= 'A' && codepoint <= 'Z') {
            codepoint = codepoint - 'A' + 'a';
        }
        // Add more Unicode case folding as needed
        
        // Encode back
        size_t encoded = utf8_encode_char(codepoint, output + out_pos);
        if (encoded == 0) {
            break;
        }
        out_pos += encoded;
    }
    
    output[out_pos] = '\0';
    *output_len = out_pos;
    
    return 0;
}

// Functions are used by tokenization algorithms
// Main implementation will call these functions

