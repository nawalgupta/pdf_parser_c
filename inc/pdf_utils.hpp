#pragma once

#include <list>
#include <string>
#include <memory>
#include <optional>
#include <regex>
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

#include "parseargs.hpp"

#ifndef TITLE_FORMAT_INDENT_DELTA
#define TITLE_FORMAT_INDENT_DELTA 0.1
#endif

static int titleMaxLength = 100;
static int pageFooterHeight = 60.0;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static double resolution = 72.0;

static const ArgDesc argDesc[] = {
    {
        "-L",       argInt,      &titleMaxLength,     0,
        "title max length"
    },
    {
        "-pfh",     argFP,       &pageFooterHeight, 0,
        "page footer height"
    },
    {
        "-opw",     argString,   ownerPassword,       sizeof(ownerPassword),
        "owner password (for encrypted files)"
    },
    {
        "-upw",     argString,   userPassword,        sizeof(userPassword),
        "user password (for encrypted files)"
    },
    {
        "-r",       argFP,       &resolution,         0,
        "resolution, in DPI (default is 72.0)"
    }
};

struct TitleFormat {
        enum class CASE {ALL_UPPER, FIRST_ONLY_UPPER, ALL_LOWER};
        enum class FONT_STYLE {NONE, BOLD, ITALIC, UNDERLINE, BOLD_ITALIC, BOLD_UNDERLINE, ITALIC_UNDERLINE, BOLD_ITALIC_UNDERLINE};
        enum class PREFIX {NONE, BULLET, NUMBERING};
        enum class EMPHASIZE_STYLE {NONE, SINGLE_QUOTE, DOUBLE_QUOTE};
        const double INDENT_DELTA = TITLE_FORMAT_INDENT_DELTA;

        double font_size;
        std::string font_family;
        FONT_STYLE font_style;
        CASE title_case;
        PREFIX prefix;
        std::string prefix_format;
        EMPHASIZE_STYLE emphasize_style;
        bool same_line_with_content;
        double indent;

        bool operator==(const TitleFormat& title_format);

        bool operator!=(const TitleFormat& title_format);

        TitleFormat(const TitleFormat& other);

        TitleFormat(TitleFormat&& other);

        TitleFormat& operator=(const TitleFormat& other);

        TitleFormat& operator=(TitleFormat&& other);
};

struct TextBlockInformation {
    bool is_page_number = false;
    std::optional<TitleFormat> title_format;
    std::list<std::string> emphasized_words;
    std::string partial_paragraph_content;
};

struct PDFSection {
    std::string title;
    std::string content;
    std::list<std::string> emphasized_words;
};

struct PDFDocument {
    std::list<PDFSection> sections;
};

// trim from start (in place)
static inline void ltrim(std::string& s);

// trim from end (in place)
static inline void rtrim(std::string& s);

// trim from both ends (in place)
static inline void trim(std::string& s);

// trim from start (copying)
static inline std::string ltrim_copy(std::string s);

// trim from end (copying)
static inline std::string rtrim_copy(std::string s);

// trim from both ends (copying)
static inline std::string trim_copy(std::string s);

// convert Unicode character to UTF-8 encoded string
static inline std::string UnicodeToUTF8(Unicode codepoint);

// extract text block information from text block
TextBlockInformation* extract_text_block_information(TextBlock* text_block, bool analyze_page_number, double y0);

PDFDoc* open_pdf_document(char *file_name);

std::string parse_pdf_document(std::unique_ptr<PDFDocument> pdf_ptr);
