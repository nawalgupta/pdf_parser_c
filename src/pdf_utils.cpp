#include "pdf_utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <FontInfo.h>

const double TitleFormat::INDENT_DELTA_THRESHOLD = TITLE_FORMAT_INDENT_DELTA;

inline std::string UnicodeToUTF8(Unicode codepoint) {
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
    return font_ref.num == title_format.font_ref.num &&
           title_case == title_format.title_case &&
           prefix == title_format.prefix &&
           emphasize_style == title_format.emphasize_style &&
           numbering_level == title_format.numbering_level &&
           !(same_line_with_content ^ title_format.same_line_with_content);
//    &&
//               (title_case == TitleFormat::CASE::ALL_UPPER ||  // indent doesn't affect style since text is centered
//                   (title_case == TitleFormat::CASE::FIRST_ONLY_UPPER &&
//                    std::fabs(indent - title_format.indent) <= INDENT_DELTA_THRESHOLD));
}

bool TitleFormat::operator !=(const TitleFormat& title_format) {
    return font_ref.num != title_format.font_ref.num ||
           title_case != title_format.title_case ||
           prefix != title_format.prefix ||
           emphasize_style != title_format.emphasize_style ||
           numbering_level != title_format.numbering_level ||
           (same_line_with_content ^ title_format.same_line_with_content);
//    ||
//               (title_case == TitleFormat::CASE::FIRST_ONLY_UPPER &&
//                std::fabs(indent - title_format.indent) > INDENT_DELTA_THRESHOLD);
}

TitleFormat::TitleFormat() {
}

TitleFormat::TitleFormat(const TitleFormat& other) :
    font_ref(other.font_ref),
    title_case(other.title_case),
    prefix(other.prefix),
    emphasize_style(other.emphasize_style),
    numbering_level(other.numbering_level),
    same_line_with_content(other.same_line_with_content),
    indent(other.indent) {

}

TitleFormat::TitleFormat(TitleFormat&& other) :
    font_ref(other.font_ref),
    title_case(other.title_case),
    prefix(other.prefix),
    emphasize_style(other.emphasize_style),
    numbering_level(other.numbering_level),
    same_line_with_content(other.same_line_with_content),
    indent(other.indent) {

}

TitleFormat& TitleFormat::operator=(const TitleFormat& other) {
    font_ref = other.font_ref;
    title_case = other.title_case;
    prefix = other.prefix;
    emphasize_style = other.emphasize_style;
    numbering_level = other.numbering_level;
    same_line_with_content = other.same_line_with_content;
    indent = other.indent;
    return *this;
}

TitleFormat& TitleFormat::operator=(TitleFormat&& other) {
    font_ref = other.font_ref;
    title_case = other.title_case;
    prefix = other.prefix;
    emphasize_style = other.emphasize_style;
    numbering_level = other.numbering_level;
    same_line_with_content = other.same_line_with_content;
    indent = other.indent;
    return *this;
}

std::ostream& operator<<(std::ostream& os, const TitleFormat& tf) {
    os << "\nFont ref: " << tf.font_ref.num << " " << tf.font_ref.gen
       << "\nTitle case: " << static_cast<unsigned int>(tf.title_case)
       << "\nTitle prefix: " << static_cast<unsigned int>(tf.prefix)
       << "\nEmphasize style: " << static_cast<unsigned int>(tf.emphasize_style)
       << "\nNumbering level: " << tf.numbering_level
       << "\nIs same line with content: " << tf.same_line_with_content
       << "\nIndent: " << tf.indent << std::endl;
    return os;
}

// recursive
nlohmann::json add_json_node(DocumentNode& current_node) {
    nlohmann::json json_pdf_section;
    json_pdf_section["id"] = current_node.main_section->id;
    json_pdf_section["title"] = current_node.main_section->title;
    json_pdf_section["content"] = current_node.main_section->content;
    json_pdf_section["parent_id"] = current_node.parent_node->main_section->id;
    for (std::string emphasized_word : current_node.main_section->emphasized_words) {
        json_pdf_section["keywords"] += emphasized_word;
    }

    if(current_node.sub_sections) {
        for (DocumentNode& node : current_node.sub_sections.value()) {
            json_pdf_section["subnodes"] += add_json_node(node);
        }
    }

    return json_pdf_section;
}

nlohmann::json add_json_node_list(DocumentNode& current_node) {
    nlohmann::json json_node_list;
    std::list<DocumentNode*> doc_node_stack;
    doc_node_stack.push_back(&current_node);
    unsigned int id = 0;
    while (!doc_node_stack.empty()) {
        // take 1 element
        DocumentNode *current_node = doc_node_stack.back();
        doc_node_stack.pop_back();

        nlohmann::json json_pdf_section;
        current_node->main_section->id = id++;
        json_pdf_section["id"] = current_node->main_section->id;
        json_pdf_section["title"] = current_node->main_section->title;
        json_pdf_section["content"] = current_node->main_section->content;
        for (std::string emphasized_word : current_node->main_section->emphasized_words) {
            json_pdf_section["keywords"] += emphasized_word;
        }
        if (current_node->parent_node)
            json_pdf_section["parent_id"] = current_node->parent_node->main_section->id;
        // process
        json_node_list.push_back(json_pdf_section);

        if (current_node->sub_sections) {
            for (DocumentNode& node : current_node->sub_sections.value()) {
                doc_node_stack.push_back(&node);
            }
        }
    }
    return json_node_list;
}

// extract text block information from text block
TextBlockInformation* extract_text_block_information(TextBlock* text_block, bool analyze_page_number, double y0, unsigned int title_max_length)  {
    TextBlockInformation* text_block_information = new TextBlockInformation;

    // check if text block is page number
    double xMinA, xMaxA, yMinA, yMaxA;
    double txMinA, txMaxA, tyMinA, tyMaxA; // for title's first character
    Ref font_ref;
    std::optional<double> title_indent, title_baseline;
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
            if (std::regex_match(line_string, page_number_regex_match, page_number_regex)) {
                text_block_information->is_page_number = true;
            }
        }
    } else if (yMinA < y0) {
        std::stringstream partial_paragraph_content_string_stream;
        std::stringstream emphasized_word_string_stream;
        bool parsing_emphasized_word = false;
        TextFontInfo* font_info, *prev_font_info = nullptr;
        std::optional<std::string> title_prefix;
        for (TextLine* line = text_block->getLines(); line; line = line->getNext()) {
            for (TextWord* word = line->getWords(); word; word = word->getNext()) {
                // extract a partition of emphasized word from word
                int word_length = word->getLength();
                for (int i = 0; i < word_length; ++i) {
                    std::string character = UnicodeToUTF8(*(word->getChar(i)));

                    // TODO: linhlt: temporary fix
                    if (character.compare("“") == 0 || character.compare("”") == 0) {
                        character = "\"";
                    }

                    font_info = word->getFontInfo(i);
                    if (parsing_emphasized_word && prev_font_info) {  // just need to compare to font of previous character
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
                            if (!title_prefix) {
                                // update txMinA & tyMaxA of this character to use later, txMinA is indent, tyMaxA is baseline to determine is_same_line later
                                word->getCharBBox(i, &txMinA, &tyMinA, &txMaxA, &tyMaxA);
                                title_indent = txMinA;
                                title_baseline = tyMaxA;
                                font_ref = *(word->getFontInfo(i)->gfxFont->getID());

                                if (!partial_paragraph_content_string_stream.str().empty()) {
                                    title_prefix = partial_paragraph_content_string_stream.str();
                                }
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
            !is_all_lower_case(text_block_information->emphasized_words.front()) &&
            text_block_information->emphasized_words.front().length() < title_max_length) {
            std::smatch title_prefix_match_result;

            if (title_prefix) {
                // case 1: prefix is in following format: bullet/numbering space single/double quote
                // step 1: find first word, using regex to match, check if it is bullet or numbering
                unsigned int pos = 0;
                std::string_view title_prefix_view(title_prefix.value());;
                size_t p_length = title_prefix_view.length();
                for (unsigned int i = 0; i < p_length; ++i) {
                    if (std::isspace(title_prefix_view[i])) {
                        pos = i;
                        break;
                    }
                }

                if (pos > 0) {
                    // check if the rest is single quote/double quoute
                    std::string_view the_rest_title_prefix_view(title_prefix_view.substr(pos + 1, p_length - pos));
                    if (the_rest_title_prefix_view.empty()) {
                        std::string first_word_title_prefix_view(title_prefix_view.substr(0, pos));

                        // bullet match
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("[\\*\\+\\-]"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::BULLET;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using latin characters (a) (b) (c)
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([a-z]{1}\\)"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::ALPHABET_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using roman numerals (i), longest presentation might be (xviii)
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([ivx]{1,5}\\)"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::ROMAN_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using number with dots ex 1 2 3 or 1. 2. 3. or 1.1 1.2 1.3 or 1.1. 1.2. 1.3.
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\d+(\\.\\d+)*\\.?"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::NUMBER_DOT_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
                            text_block_information->title_format = std::move(title_format);
                        }
                    } else if (the_rest_title_prefix_view.compare("'") == 0 &&
                               text_block_information->partial_paragraph_content[text_block_information->emphasized_words.front().length() + title_prefix_view.length()] == '\'') {
                        std::string first_word_title_prefix_view(title_prefix_view.substr(0, pos));

                        // bullet match
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("[\\*\\+\\-]"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::BULLET;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::SINGLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using latin characters (a) (b) (c)
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([a-z]{1}\\)"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::ALPHABET_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::SINGLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using roman numerals (i), longest presentation might be (xviii)
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([ivx]{1,5}\\)"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::ROMAN_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::SINGLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using number with dots ex 1 2 3 or 1. 2. 3. or 1.1 1.2 1.3 or 1.1. 1.2. 1.3.
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\d+(\\.\\d+)*\\.?"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::NUMBER_DOT_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::SINGLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                    } else if (the_rest_title_prefix_view.compare("\"") == 0 &&
                               text_block_information->partial_paragraph_content[text_block_information->emphasized_words.front().length() + title_prefix_view.length()] == '\"') {
                        std::string first_word_title_prefix_view(title_prefix_view.substr(0, pos));

                        // bullet match
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("[\\*\\+\\-]"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::BULLET;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::DOUBLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using latin characters (a) (b) (c)
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([a-z]{1}\\)"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::ALPHABET_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::DOUBLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using roman numerals (i), longest presentation might be (xviii)
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\([ivx]{1,5}\\)"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::ROMAN_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::DOUBLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                        // numbering using number with dots ex 1 2 3 or 1. 2. 3. or 1.1 1.2 1.3 or 1.1. 1.2. 1.3.
                        if (std::regex_match(first_word_title_prefix_view, title_prefix_match_result, std::regex("\\d+(\\.\\d+)*\\.?"))) {
                            TitleFormat title_format;
                            title_format.prefix = TitleFormat::PREFIX::NUMBER_DOT_NUMBERING;
                            title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::DOUBLE_QUOTE;
                            text_block_information->title_format = std::move(title_format);
                        }
                    }
                } else { // no space in prefix
                    if (title_prefix_view.compare("'") == 0 &&
                        text_block_information->partial_paragraph_content[text_block_information->emphasized_words.front().length() + 1] == '\'') {
                        TitleFormat title_format;
                        title_format.prefix = TitleFormat::PREFIX::NONE;
                        title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::SINGLE_QUOTE;
                        text_block_information->title_format = std::move(title_format);
                    } else if (title_prefix_view.compare("\"") == 0 &&
                               text_block_information->partial_paragraph_content[text_block_information->emphasized_words.front().length() + 1] == '\"') {
                        TitleFormat title_format;
                        title_format.prefix = TitleFormat::PREFIX::NONE;
                        title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::DOUBLE_QUOTE;
                        text_block_information->title_format = std::move(title_format);
                    }
                }

                if (text_block_information->title_format) {
                    text_block_information->partial_paragraph_content.erase(0, text_block_information->emphasized_words.front().length() + title_prefix_view.length());
                    if (text_block_information->title_format->emphasize_style > TitleFormat::EMPHASIZE_STYLE::NONE) {
                        text_block_information->partial_paragraph_content.erase(0, 1);
                    }
                }
            } else {
                // case 2: no prefix: first emphasize word is in begining of the block, the character after first emphasized word must be colon or space
                size_t pos = text_block_information->emphasized_words.front().length();
                size_t p_length = text_block_information->partial_paragraph_content.length();
                if (pos == p_length) {
                    TitleFormat title_format;
                    title_format.prefix = TitleFormat::PREFIX::NONE;
                    title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
                    title_format.same_line_with_content = false;
                    text_block_information->title_format = std::move(title_format);

                    // cut title out of content
                    text_block_information->partial_paragraph_content = "";
                } else if (pos < p_length &&
                           (text_block_information->partial_paragraph_content[pos] == ' ' ||
                            text_block_information->partial_paragraph_content[pos] == ':')) {
                    TitleFormat title_format;
                    title_format.prefix = TitleFormat::PREFIX::NONE;
                    title_format.emphasize_style = TitleFormat::EMPHASIZE_STYLE::NONE;
                    text_block_information->title_format = std::move(title_format);

                    // cut title out of content
                    text_block_information->partial_paragraph_content = text_block_information->partial_paragraph_content.substr(pos + 1);
                }
            }

            if (text_block_information->title_format) {
                // case
                if (is_all_upper_case(text_block_information->emphasized_words.front())) {
                    text_block_information->title_format->title_case = TitleFormat::CASE::ALL_UPPER;
                    text_block_information->title_format->same_line_with_content = false;
                } else {
                    text_block_information->title_format->title_case = TitleFormat::CASE::FIRST_ONLY_UPPER;
                }

                // indentation
                text_block_information->title_format->indent = title_indent.value();

                // font ref
                text_block_information->title_format->font_ref = font_ref;
            }
        }
    }

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

std::string parse_pdf_document(PDFDoc *doc) {
    TextOutputDev* textOut;
    unsigned int title_max_length = 100;
    int page_footer_height = 60.0;
    double resolution = 72.0;

    // create text output device
    if (doc->isOk()) {
        textOut = new TextOutputDev(nullptr, gFalse, 0.0, gFalse, gFalse);
    } else {
        delete doc;
        return "{}";
    }

    // process if textOut is ok
    if (textOut->isOk()) {
        globalParams = new GlobalParams();
//        globalParams->setTextPageBreaks(gTrue);
//        globalParams->setErrQuiet(gFalse);
        int number_of_pages = doc->getNumPages();

        PDFDocument pdf_document;
        PDFSection pdf_section;
        bool start_parse = false;

//        std::cout << "Parsing " << number_of_pages << " pages of " << argv[1] << std::endl;

        for (int page = 1; page <= number_of_pages; ++page) {
            PDFRectangle* page_mediabox =  doc->getPage(page)->getMediaBox();
            double y0 = page_mediabox->y2 - page_footer_height;
            doc->displayPage(textOut, page, resolution, resolution, 0, gTrue, gFalse, gFalse);


            TextPage* textPage = textOut->takeText();
            std::list<TextBlockInformation*> text_block_information_list;

            for (TextFlow* flow = textPage->getFlows(); flow; flow = flow->getNext()) {
                for (TextBlock* text_block = flow->getBlocks(); text_block; text_block = text_block->getNext()) {

                    // must process text_block here as it'll expire after parsing page
                    TextBlockInformation* text_block_information = extract_text_block_information(text_block, !start_parse, y0, title_max_length);
                    text_block_information_list.push_back(text_block_information);

                    // if atleast 1 text block is page number block
                    if (text_block_information->is_page_number) {
                        start_parse = true; // first page that have page number
                    }
                }
            }
            textPage->decRefCnt();

            // after first page which has page number
            if (start_parse) {
                for (TextBlockInformation* text_block_information : text_block_information_list) {
                    // only add blocks that is not page number
                    if (!(text_block_information->is_page_number)) {
                        if (text_block_information->title_format) {
                            if (pdf_section.title.length() > 0) {
                                trim(pdf_section.content);
                                pdf_document.sections.push_back(pdf_section);
                            }

                            pdf_section.title = text_block_information->emphasized_words.front();
                            pdf_section.title_format = text_block_information->title_format.value();
                            text_block_information->emphasized_words.pop_front();
                            pdf_section.emphasized_words = text_block_information->emphasized_words;
                            pdf_section.content = text_block_information->partial_paragraph_content;
                        } else if (pdf_section.title.length() > 0) {
                            pdf_section.emphasized_words.insert(pdf_section.emphasized_words.end(), text_block_information->emphasized_words.begin(), text_block_information->emphasized_words.end());
                            pdf_section.content += text_block_information->partial_paragraph_content;
                        }
                    }
                }
            }

            // cleanup
            for (TextBlockInformation* text_block_information : text_block_information_list) {
                delete text_block_information;
            }
        }

        if (pdf_section.title.length() > 0) {
            trim(pdf_section.content);
            pdf_document.sections.push_back(pdf_section);
        }

        // all sections in a list, construct a tree from pdf_document.sections
        PDFSection root_section;
        root_section.title = doc->getDocInfoTitle()->toStr();
        root_section.content = "";
        root_section.id = 0;
        DocumentNode doc_root;
        doc_root.main_section = &root_section;
        doc_root.parent_node = nullptr;
        std::list<TitleFormat> title_format_stack;
        DocumentNode* current_node = &doc_root;
        for (PDFSection& section : pdf_document.sections) {
            // if this section's title format hasn't appear in title_format_stack
            std::list<TitleFormat>::iterator it = std::find(title_format_stack.begin(), title_format_stack.end(), section.title_format);

            // create subnode
            DocumentNode node;
            node.main_section = &section;

            if (it == title_format_stack.end()) { // not exist yet, create a subnode to add it to current node
                // add to current node
                if (!current_node->sub_sections) {
                    current_node->sub_sections = std::list<DocumentNode>();
                }
                node.parent_node = current_node;
                current_node->sub_sections.value().push_back(std::move(node));

                current_node = &(current_node->sub_sections.value().front());
                title_format_stack.push_back(section.title_format);
            } else {
                // Up until this title_format is the last element
                // save the iterator
                std::list<TitleFormat>::iterator tmp_it = it;
                // it modified
                while (it != title_format_stack.end()){
                    current_node = current_node->parent_node;
                    ++it;
                }
                // it = end() here
                ++tmp_it;
                title_format_stack.erase(tmp_it, it);

                node.parent_node = current_node;

                current_node->sub_sections.value().push_back(std::move(node));
                current_node = &(current_node->sub_sections.value().back());
            }
        }

        // present as tree
        // nlohmann::json json_pdf_document = add_json_node(doc_root);

        // present as list
        nlohmann::json json_pdf_document = add_json_node_list(doc_root);

        delete textOut;
        delete doc;
        delete globalParams;
        return json_pdf_document.dump();
    } else {
        delete textOut;
        delete doc;
        return "{}";
    }
}

inline void print_all_fonts(PDFDoc *doc)
{
    FontInfoScanner font_info_scanner(doc);
    GooList *fonts = font_info_scanner.scan(doc->getNumPages());

    printf("List fonts used:\n");
    printf("name                                 type              encoding         emb sub uni object ID\n");
    printf("------------------------------------ ----------------- ---------------- --- --- --- ---------\n");
    if (fonts) {
      for (int i = 0; i < fonts->getLength(); ++i) {
        FontInfo *font = static_cast<FontInfo *>(fonts->get(i));
        printf("%-36s %-17s %-16s %-3s %-3s %-3s",
               font->getName() ? font->getName()->getCString() : "[none]",
               fontTypeNames[font->getType()],
               font->getEncoding()->getCString(),
               font->getEmbedded() ? "yes" : "no",
               font->getSubset() ? "yes" : "no",
               font->getToUnicode() ? "yes" : "no");
        const Ref fontRef = font->getRef();
        if (fontRef.gen >= 100000) {
          printf(" [none]\n");
        } else {
          printf(" %6d %2d\n", fontRef.num, fontRef.gen);
        }
        delete font;
      }
      delete fonts;
    }
}
