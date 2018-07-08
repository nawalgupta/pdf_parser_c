#include "pdf_utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

const double TitleFormat::INDENT_DELTA = TITLE_FORMAT_INDENT_DELTA;

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
    return gfx_font == title_format.gfx_font &&
           title_case == title_format.title_case &&
           prefix == title_format.prefix &&
           emphasize_style == title_format.emphasize_style &&
           prefix_format.compare(title_format.prefix_format) == 0 &&
           !(same_line_with_content ^ title_format.same_line_with_content) &&
           std::fabs(indent - title_format.indent) <= INDENT_DELTA ;
}

bool TitleFormat::operator !=(const TitleFormat& title_format) {
    return gfx_font != title_format.gfx_font ||
           title_case != title_format.title_case ||
           prefix != title_format.prefix ||
           emphasize_style != title_format.emphasize_style ||
           prefix_format.compare(title_format.prefix_format) != 0 ||
           (same_line_with_content ^ title_format.same_line_with_content) ||
           std::fabs(indent - title_format.indent) > INDENT_DELTA;
}

TitleFormat::TitleFormat() {

}

TitleFormat::TitleFormat(const TitleFormat& other) :
    gfx_font(other.gfx_font),
    prefix_format(other.prefix_format),
    title_case(other.title_case),
    prefix(other.prefix),
    emphasize_style(other.emphasize_style),
    same_line_with_content(other.same_line_with_content),
    indent(other.indent) {

}

TitleFormat::TitleFormat(TitleFormat&& other) :
    gfx_font(other.gfx_font),
    prefix_format(std::move(other.prefix_format)),
    title_case(other.title_case),
    prefix(other.prefix),
    emphasize_style(other.emphasize_style),
    same_line_with_content(other.same_line_with_content),
    indent(other.indent) {

}

TitleFormat& TitleFormat::operator=(const TitleFormat& other) {
    gfx_font = other.gfx_font;
    prefix_format = other.prefix_format;
    title_case = other.title_case;
    prefix = other.prefix;
    emphasize_style = other.emphasize_style;
    same_line_with_content = other.same_line_with_content;
    indent = other.indent;
    return *this;
}

TitleFormat& TitleFormat::operator=(TitleFormat&& other) {
    gfx_font = other.gfx_font;
    prefix_format = std::move(other.prefix_format);
    title_case = other.title_case;
    prefix = other.prefix;
    emphasize_style = other.emphasize_style;
    same_line_with_content = other.same_line_with_content;
    indent = other.indent;
    return *this;
}


// extract text block information from text block
TextBlockInformation* extract_text_block_information(TextBlock* text_block, bool analyze_page_number, double y0, unsigned int title_max_length)  {
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
                GooString* text_word = word->getText();
                line_string += text_word->toStr() + " ";
                delete text_word;
            }
            line_string.pop_back();
            // TODO: match using faster regex lib (eg: boost.regex)
            if (std::regex_match(line_string, page_number_regex_match, page_number_regex)) {
                text_block_information->is_page_number = true;
//                std::cout << "Document page number: " << line_string << std::endl;
            }
        }
    } else if (yMinA < y0) {
        std::stringstream partial_paragraph_content_string_stream;
        std::stringstream emphasized_word_string_stream;
        bool parsing_emphasized_word = false;
        TextFontInfo* font_info, *prev_font_info;
        std::optional<std::string> title_prefix;
        for (TextLine* line = text_block->getLines(); line; line = line->getNext()) {
            for (TextWord* word = line->getWords(); word; word = word->getNext()) {
                // extract a partition of emphasized word from word
                int word_length = word->getLength();
                for (int i = 0; i < word_length; ++i) {
                    std::string character = UnicodeToUTF8(*(word->getChar(i)));

                    font_info = word->getFontInfo(i);
                    if (parsing_emphasized_word) {  // just need to compare to font of previous character
                        if (font_info->gfxFont == prev_font_info->gfxFont) { // same as previous character
                            emphasized_word_string_stream << character;
                        } else {
                            std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
                            if (trimmed_string.length() > 0) {
                                text_block_information->emphasized_words.push_back(trimmed_string);
                            }
                            emphasized_word_string_stream.str(std::string());
                            parsing_emphasized_word = false;

                            if (word->getFontInfo(i)->gfxFont->getWeight() > GfxFont::W400 || word->getFontInfo(i)->isItalic()) {
                                parsing_emphasized_word = true;
                                emphasized_word_string_stream << character;
                            }
                        }
                    } else {
                        if (word->getFontInfo(i)->gfxFont->getWeight() > GfxFont::W400 || word->getFontInfo(i)->isItalic()) {
                            parsing_emphasized_word = true;
                            // first time this occured
                            if (!title_prefix && !partial_paragraph_content_string_stream.str().empty()) {
                                title_prefix = partial_paragraph_content_string_stream.str();
                            }
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

                    // add character to partial paragraph content
                    partial_paragraph_content_string_stream << character;

                    prev_font_info = word->getFontInfo(i);
                }
                if (parsing_emphasized_word) {
                    emphasized_word_string_stream << u8" ";
                }
                partial_paragraph_content_string_stream << u8" "; // utf-8 encoded space character
            }
        }
        text_block_information->partial_paragraph_content = partial_paragraph_content_string_stream.str();

        // if emphasized_word is in the end of partial_paragraph
        std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
        if (parsing_emphasized_word && trimmed_string.length() > 0) {
            text_block_information->emphasized_words.push_back(trimmed_string);
        }

        if (!text_block_information->emphasized_words.empty() &&
            text_block_information->emphasized_words.front().length() < title_max_length) {
            std::regex special_characters {R"([-[\]{}()*+?.,\^$|#\s])"};
            std::string replace_rule(R"(\$&)");
            std::smatch title_prefix_match_result;

            if (title_prefix) {
                // case 1: prefix is in following format: bullet/numbering space single/double quote
                // step 1: find first word, using regex to match, check if it is bullet or numbering

                // step 2: next character after space must be ' or "
//                if (std::regex_match(title_prefix.value(), title_prefix_match_result, std::regex("^[\\*\\+\\-]\\s"))) {
//                    TitleFormat title_format;
//                    title_format.prefix = TitleFormat::PREFIX::BULLET;
//                    text_block_information->title_format = std::move(title_format);
//                }

            } else {
                // case 2: no prefix: first emphasize word is in begining of the block, the character after first emphasized word must be colon or space
                unsigned int pos = text_block_information->emphasized_words.front().length();
                unsigned int p_length = text_block_information->partial_paragraph_content.length();
                if (pos == p_length) {
                    TitleFormat title_format;
                    title_format.prefix = TitleFormat::PREFIX::NONE;
                    title_format.same_line_with_content = false;
                    text_block_information->title_format = std::move(title_format);

                    // cut title out of content
                    text_block_information->partial_paragraph_content = "";
                } else if (pos < p_length &&
                           (text_block_information->partial_paragraph_content[pos] == ' ' ||
                            text_block_information->partial_paragraph_content[pos] == ':')) {
                    TitleFormat title_format;
                    title_format.prefix = TitleFormat::PREFIX::NONE;
                    title_format.same_line_with_content = true;
                    text_block_information->title_format = std::move(title_format);

                    // cut title out of content
                    text_block_information->partial_paragraph_content = text_block_information->partial_paragraph_content.substr(pos + 1);
                }
            }
        }
    }


//        if (title_prefix && std::regex_match(title_prefix.value(), title_prefix_match_result, std::regex())) {
//            TitleFormat title_format;
////            title_format.font_size = 16;
//            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
//            text_block_information->title_format = std::move(title_format);
//        }


//        // TODO: find text before the first emphasized word and match with faster regex lib (eg: boost.regex) instead of std regex
//        std::smatch title_match_result;
//        if (!text_block_information->emphasized_words.empty() &&
//            std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^(((\\d+(\\.\\d+)*\\.?)|[\\*\\+\\-]|(\\([a-z]{1,4}\\)))\\s+\"?)?" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")))
////            (   std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
////                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\\d+(\\.\\d+)*\\.?\\s+" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
////                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\\([a-z]{1,4}\\)\\s+" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
////                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^[\\*\\+\\-]\\s" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
////                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\"" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + "\".*"))))
//        {
//            TitleFormat title_format;
////            title_format.font_size = 16;
//            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
//            text_block_information->title_format = std::move(title_format);
//        }

    return text_block_information;
}

PDFDoc* open_pdf_document(char* file_name, char* owner_password, char* user_password) {
    GooString* fileName;
    GooString* ownerPW, *userPW;

    // parse filename, filename is non null pointer
    fileName = new GooString(file_name);

    // owner password
    if (owner_password[0] != '\001') {
        ownerPW = new GooString(owner_password);
    } else {
        ownerPW = nullptr;
    }

    // user password
    if (user_password[0] != '\001') {
        userPW = new GooString(user_password);
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
