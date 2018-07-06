#include "pdf_utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

static inline std::string UnicodeToUTF8(Unicode codepoint) {
    std::string out;
    if (codepoint <= 0x7f)
        out.append(1, static_cast<char>(codepoint));
    else if (codepoint <= 0x7ff) {
        out.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
        out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else if (codepoint <= 0xffff) {
        out.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
        out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else {
        out.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
        out.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    return out;
}

bool TitleFormat::operator ==(const TitleFormat& title_format) {
    return font_size == title_format.font_size &&
           font_style == title_format.font_style &&
           title_case == title_format.title_case &&
           prefix == title_format.prefix &&
           emphasize_style == title_format.emphasize_style &&
           font_family.compare(title_format.font_family) == 0 &&
           prefix_format.compare(title_format.prefix_format) == 0 &&
           !(same_line_with_content ^ title_format.same_line_with_content) &&
           std::fabs(indent - title_format.indent) < INDENT_DELTA ;
}

bool TitleFormat::operator !=(const TitleFormat& title_format) {
    return font_size != title_format.font_size ||
           font_style != title_format.font_style ||
           title_case != title_format.title_case ||
           prefix != title_format.prefix ||
           emphasize_style != title_format.emphasize_style ||
           font_family.compare(title_format.font_family) != 0 ||
           prefix_format.compare(title_format.prefix_format) != 0 ||
           (same_line_with_content ^ title_format.same_line_with_content) ||
           std::fabs(indent - title_format.indent) >= INDENT_DELTA;
}


// extract text block information from text block
TextBlockInformation* extract_text_block_information(TextBlock* text_block, bool analyze_page_number, double y0)  {
    TextBlockInformation* text_block_information = new TextBlockInformation;

    // check if text block is page number
    double xMinA, xMaxA, yMinA, yMaxA;
    text_block->getBBox(&xMinA, &yMinA, &xMaxA, &yMaxA);
    std::regex page_number_regex("^.{0,2}[0-9]+.{0,2}");
    std::smatch page_number_regex_match;
    if (analyze_page_number && yMinA >= y0) {
        if (text_block->getLineCount() == 1) {  // page number is in 1 line only
            TextLine* line = text_block->getLines();
            std::string line_string;
            for (TextWord* word = line->getWords(); word; word = word->getNext()) {
                GooString *text_word = word->getText();
                line_string += text_word->toStr() + " ";
                delete text_word;
            }
            line_string.pop_back();
            if (std::regex_match(line_string, page_number_regex_match, page_number_regex)) {
                text_block_information->is_page_number = true;
//                std::cout << "Document page number: " << line_string << std::endl;
            }
        }
    } else if (yMinA < y0) {
        std::stringstream partial_paragraph_content_string_stream;
        std::stringstream emphasized_word_string_stream;
        bool parsing_emphasized_word = false;
        std::regex font_regex(".*([bB]old|[iI]talic).*");
        std::smatch font_regex_match; // string match
        GooString *g_font_name, *g_prev_font_name;
        std::string font_name;
        for (TextLine* line = text_block->getLines(); line; line = line->getNext()) {
            for (TextWord* word = line->getWords(); word; word = word->getNext()) {
                // extract a partition of emphasized word from word
                int word_length = word->getLength();
                for (int i = 0; i < word_length; ++i) {
                    std::string character = UnicodeToUTF8(*(word->getChar(i)));

                    // add character to partial paragraph content
                    partial_paragraph_content_string_stream << character;

                    g_font_name = word->getFontName(i);
                    font_name = g_font_name->toStr();
                    if (parsing_emphasized_word) {  // just need to compare to font of previous character
                        if (g_prev_font_name->cmp(g_font_name) == 0) { // same as previous character
                            emphasized_word_string_stream << character;
                        } else {
                            std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
                            if (trimmed_string.length() > 0) {
                                text_block_information->emphasized_words.push_back(trimmed_string);
                            }
                            emphasized_word_string_stream.str(std::string());
                            parsing_emphasized_word = false;

                            if (std::regex_match(font_name, font_regex_match, font_regex)) {
                                parsing_emphasized_word = true;
                                emphasized_word_string_stream << character;
                            }
                        }
                    } else {
                        if (std::regex_match(font_name, font_regex_match, font_regex)) {
                            parsing_emphasized_word = true;
                            emphasized_word_string_stream << character;
                        } else if (parsing_emphasized_word) {
                            std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
                            if (trimmed_string.length() > 0) {
                                text_block_information->emphasized_words.push_back(trimmed_string);
                            }
                            emphasized_word_string_stream.str(std::string());
                            parsing_emphasized_word = false;
                        }
                    }
                    g_prev_font_name = g_font_name;
                }
                if (parsing_emphasized_word) {
                    emphasized_word_string_stream << u8" ";
                }
                partial_paragraph_content_string_stream << u8" "; // utf-8 encoded space character
            }
        }
        text_block_information->partial_paragraph_content = partial_paragraph_content_string_stream.str();
//        text_block_information->partial_paragraph_content.pop_back();

        // if emphasized_word is in the end of partial_paragraph
        std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
        if (parsing_emphasized_word && trimmed_string.length() > 0) {
            text_block_information->emphasized_words.push_back(trimmed_string);
        }

        std::regex special_characters {R"([-[\]{}()*+?.,\^$|#\s])"};
        std::string replace_rule(R"(\$&)");

        std::smatch title_match_result;
        if (!text_block_information->emphasized_words.empty() &&
            std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^(((\\d+(\\.\\d+)*\\.?)|[\\*\\+\\-]|(\\([a-z]{1,4}\\)))\\s+\"?)?" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")))
//            (   std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
//                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\\d+(\\.\\d+)*\\.?\\s+" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
//                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\\([a-z]{1,4}\\)\\s+" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
//                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^[\\*\\+\\-]\\s" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
//                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\"" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + "\".*"))))
        {
            text_block_information->title_format->font_size = 16;
            text_block_information->title_format->emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
        }

        // linhlt rule: if title length > titleMaxLength -> not a title
        if (text_block_information->title_format && text_block_information->emphasized_words.front().length() > titleMaxLength) {
            text_block_information->title_format = std::nullopt;
        }
    }


//    std::cout << y << " | " << (y + h) <<  " | " << yMinA << " | " << yMaxA <<  " | " << xMinA << " | " << xMaxA << " | " << text_block_information->is_page_number << std::endl;

    return text_block_information;
}

PDFDoc* open_pdf_document(char *file_name) {
    GooString* fileName;
    GooString* ownerPW, *userPW;

    // parse filename, filename is non null pointer
    fileName = new GooString(file_name);

    // owner password
    if (ownerPassword[0] != '\001') {
        ownerPW = new GooString(ownerPassword);
    } else {
        ownerPW = nullptr;
    }

    // user password
    if (userPassword[0] != '\001') {
        userPW = new GooString(userPassword);
    } else {
        userPW = nullptr;
    }

    PDFDoc* doc = PDFDocFactory().createPDFDoc(*fileName, ownerPW, userPW);

    delete fileName;

    if (userPW) {
        delete userPW;
    }

    if (ownerPW) {
        delete ownerPW;
    }

    return doc;
}

std::string parse_pdf_document(std::unique_ptr<PDFDocument> pdf_ptr) {
    return "{\"test\": \"adjalskjdalskd\"}";
}
