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
#include <GfxFont.h>

#ifndef TITLE_FORMAT_INDENT_DELTA
#define TITLE_FORMAT_INDENT_DELTA 0.1
#endif

struct TitleFormat {
        static const double INDENT_DELTA;

        enum class CASE {ALL_UPPER, FIRST_ONLY_UPPER, ALL_LOWER};
        enum class PREFIX {NONE, BULLET, NUMBERING};
        enum class EMPHASIZE_STYLE {NONE, SINGLE_QUOTE, DOUBLE_QUOTE};

        GfxFont *gfx_font;
        CASE title_case;
        PREFIX prefix;
        EMPHASIZE_STYLE emphasize_style;
        std::string prefix_format;
        bool same_line_with_content;
        double indent;

        bool operator==(const TitleFormat& title_format);

        bool operator!=(const TitleFormat& title_format);

        TitleFormat();

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
TextBlockInformation* extract_text_block_information(TextBlock* text_block, bool analyze_page_number, double y0, unsigned int title_max_length);

PDFDoc* open_pdf_document(char *file_name, char *owner_password, char *user_password);

std::string parse_pdf_document(std::unique_ptr<PDFDocument> pdf_ptr);
