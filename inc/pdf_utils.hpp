#pragma once

#include <list>
#include <string>
#include <memory>
#include <optional>
#include <regex>
#include <algorithm>
#include <poppler-config.h>
#include <goo/GooString.h>
#include <goo/gmem.h>
#include <GlobalParams.h>
#include <Object.h>
#include <Stream.h>
#include <Array.h>
#include <Dict.h>
#include <XRef.h>
#include <Catalog.h>
#include <Page.h>
#include <TextOutputDev.h>
#include <CharTypes.h>
#include <UnicodeMap.h>
#include <PDFDocEncoding.h>
#include <Parser.h>
#include <Lexer.h>
#include <Error.h>
#include <CharTypes.h>
#include <PDFDoc.h>
#include <Page.h>
#include <PDFDocFactory.h>
#include <GfxFont.h>

#ifndef TITLE_FORMAT_INDENT_DELTA
#define TITLE_FORMAT_INDENT_DELTA 0.2
#endif

static const char *fontTypeNames[] = {
  "unknown",
  "Type 1",
  "Type 1C",
  "Type 1C (OT)",
  "Type 3",
  "TrueType",
  "TrueType (OT)",
  "CID Type 0",
  "CID Type 0C",
  "CID Type 0C (OT)",
  "CID TrueType",
  "CID TrueType (OT)"
};

struct TitleFormat {
        static const double INDENT_DELTA_THRESHOLD;

        enum class CASE {ALL_UPPER, FIRST_ONLY_UPPER};
        enum class PREFIX {NONE, BULLET, ROMAN_NUMBERING, NUMBER_DOT_NUMBERING, ALPHABET_NUMBERING};
        enum class EMPHASIZE_STYLE {NONE, SINGLE_QUOTE, DOUBLE_QUOTE};

        Ref font_ref;
        CASE title_case;
        PREFIX prefix;
        EMPHASIZE_STYLE emphasize_style;
        unsigned int numbering_level = 0;
        bool same_line_with_content = true;
        double indent;

        TitleFormat();

        TitleFormat(const TitleFormat& other);

        TitleFormat(TitleFormat&& other);

        TitleFormat& operator=(const TitleFormat& other);

        TitleFormat& operator=(TitleFormat&& other);

        bool operator==(const TitleFormat& title_format);

        bool operator!=(const TitleFormat& title_format);

        friend std::ostream& operator<<(std::ostream& os, const TitleFormat& tf);
};

struct TextBlockInformation {
    bool is_page_number = false;
    std::optional<TitleFormat> title_format;
    std::list<std::string> emphasized_words;
    std::string partial_paragraph_content;
};

struct PDFSection {
    unsigned int id;
    std::string title;
    TitleFormat title_format;
    std::string content;
    std::list<std::string> emphasized_words;
};

struct PDFDocument {
    std::list<PDFSection> sections;
};

struct DocumentNode {
    PDFSection* main_section;
    std::optional<std::list<DocumentNode>> sub_sections;
    DocumentNode* parent_node;
};

// trim from start (in place)
inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

inline bool is_all_upper_case(std::string& s) {
    return std::none_of(s.begin(), s.end(), &::islower);
}

inline bool is_all_lower_case(std::string& s) {
    return std::none_of(s.begin(), s.end(), &isupper);
}

// convert Unicode character to UTF-8 encoded string
static inline std::string UnicodeToUTF8(Unicode codepoint);

// extract text block information from text block
TextBlockInformation* extract_text_block_information(TextBlock* text_block, bool analyze_page_number, double y0, unsigned int title_max_length);

PDFDoc* open_pdf_document(char *file_name, char *owner_password, char *user_password);

std::string parse_pdf_document(std::unique_ptr<PDFDocument> pdf_ptr);
