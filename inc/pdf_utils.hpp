#pragma once

#include <list>
#include <string>
#include <memory>
#include <poppler/CharTypes.h>

struct TitleFormat {
        enum class CASE {ALL_UPPER, FIRST_ONLY_UPPER, ALL_LOWER};
        enum class FONT_STYLE {NONE, BOLD, ITALIC, UNDERLINE, BOLD_ITALIC, BOLD_UNDERLINE, ITALIC_UNDERLINE, BOLD_ITALIC_UNDERLINE};
        enum class PREFIX {NONE, BULLET, NUMBERING};
        enum class EMPHASIZE_STYLE {NONE, SINGLE_QUOTE, DOUBLE_QUOTE};

        double font_size;
        std::string font_family;
        FONT_STYLE font_style;
        CASE title_case;
        PREFIX prefix;
        std::string prefix_format;
        EMPHASIZE_STYLE emphasize_style;
        bool same_line_with_content;

        bool operator==(const TitleFormat& title_format) {
            return font_size == title_format.font_size &&
                   font_style == title_format.font_style &&
                   title_case == title_format.title_case &&
                   prefix == title_format.prefix &&
                   emphasize_style == title_format.emphasize_style &&
                   font_family.compare(title_format.font_family) == 0 &&
                   prefix_format.compare(title_format.prefix_format) == 0 &&
                   !(same_line_with_content ^ title_format.same_line_with_content);
        }

        bool operator!=(const TitleFormat& title_format) {
            return font_size != title_format.font_size ||
                   font_style != title_format.font_style ||
                   title_case != title_format.title_case ||
                   prefix != title_format.prefix ||
                   emphasize_style != title_format.emphasize_style ||
                   font_family.compare(title_format.font_family) != 0 ||
                   prefix_format.compare(title_format.prefix_format) != 0 ||
                   (same_line_with_content ^ title_format.same_line_with_content);
        }
};

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
