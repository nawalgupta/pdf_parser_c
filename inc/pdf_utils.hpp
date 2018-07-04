#pragma once

#include <list>
#include <string>
#include <memory>
#include <poppler/CharTypes.h>

struct TextBlockInformation {
    bool is_page_number = false;
    bool has_title = false;
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

std::string parse_pdf_document(std::unique_ptr<PDFDocument> pdf_ptr);
