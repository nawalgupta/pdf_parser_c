/*
 * Parse pdf file, write output to .json file using poppler
 * with one pdf file:
 * user password: the password to open/read pdf file
 * owner password: the password to print, edit, extract, comment, ...
 * ctm: Current Transformation Matrix
 * gfx: Graphic
 * To extract page in body of pages, specify -L,  flag
 * To set title max length, specify -L flag
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <list>
#include <regex>
#include <algorithm>
#include <cctype>
#include <locale>
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
#include <PDFDoc.h>
#include <Page.h>
#include <PDFDocFactory.h>
#include <TextOutputDev.h>
#include <CharTypes.h>
#include <UnicodeMap.h>
#include <PDFDocEncoding.h>
#include <Parser.h>
#include <Lexer.h>
#include <Error.h>
#include <nlohmann/json.hpp>
#include "parseargs.hpp"

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


// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

// convert Unicode character to UTF-8 encoded string
static inline std::string UnicodeToUTF8(Unicode codepoint) {
    std::string out;
    if (codepoint <= 0x7f) {
        out.append(1, static_cast<char>(codepoint));
    } else if (codepoint <= 0x7ff) {
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

// extract text block information from text block
static inline TextBlockInformation* extract_text_block_information(TextBlock* text_block, bool analyze_page_number, double y0) {
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
                line_string += word->getText()->toStr() + " ";
            }
            line_string.pop_back();
            if (std::regex_match(line_string, page_number_regex_match, page_number_regex)) {
                text_block_information->is_page_number = true;
//                std::cout << "Document page number: " << line_string << std::endl;
            }
        }
    } else {
        std::stringstream partial_paragraph_content_string_stream;
        std::stringstream emphasized_word_string_stream;
        bool parsing_emphasized_word = false;
        std::regex font_regex(".*([bB]old|[iI]talic).*");
        std::smatch font_regex_match; // string match
        for (TextLine* line = text_block->getLines(); line; line = line->getNext()) {
            for (TextWord* word = line->getWords(); word; word = word->getNext()) {
                // extract a partition of emphasized word from word
                int word_length = word->getLength();
                for (int i = 0; i < word_length; ++i) {
                    std::string character = UnicodeToUTF8(*(word->getChar(i)));

                    // add character to partial paragraph content
                    partial_paragraph_content_string_stream << character;

                    // process emphasized word
                    std::string font_name = word->getFontName(i)->toStr();
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
                if (parsing_emphasized_word) {
                    emphasized_word_string_stream << u8" ";
                }
                partial_paragraph_content_string_stream << u8" "; // utf-8 encoded space character
            }
            if (parsing_emphasized_word) {
                emphasized_word_string_stream << u8" ";
            }
            partial_paragraph_content_string_stream << u8" "; // utf-8 encoded space character
        }
        text_block_information->partial_paragraph_content = partial_paragraph_content_string_stream.str();

        // if emphasized_word is in the end of partial_paragraph
        std::string trimmed_string = trim_copy(emphasized_word_string_stream.str());
        if (parsing_emphasized_word && trimmed_string.length() > 0) {
            text_block_information->emphasized_words.push_back(trimmed_string);
        }

        std::regex special_characters {R"([-[\]{}()*+?.,\^$|#\s])"};
        std::string replace_rule(R"(\$&)");

        std::smatch title_match_result;
        if (!text_block_information->emphasized_words.empty() && (
                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\\d+(\\.\\d+)*\\.?\\s+" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\\([a-z]{1,4}\\)\\s+" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^[\\*\\+\\-]\\s" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + ".*")) ||
                std::regex_match(text_block_information->partial_paragraph_content, title_match_result, std::regex("^\"" + std::regex_replace(text_block_information->emphasized_words.front(), special_characters, replace_rule) + "\".*")))) {
            text_block_information->has_title = true;
        }

        // linhlt rule: if title length > titleMaxLength -> not a title
        if (text_block_information->has_title && text_block_information->emphasized_words.front().length() > titleMaxLength) {
            text_block_information->has_title = false;
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

int main(int argc, char* argv[]) {
    PDFDoc* doc;
    TextOutputDev* textOut;

    // parse args
    if (argc > 1 && parseArgs(argDesc, &argc, argv)) {
        doc = open_pdf_document(argv[1]);
    } else {
        return EXIT_FAILURE;
    }

    // create text output device
    if (doc->isOk()) {
        textOut = new TextOutputDev(nullptr, gFalse, 0.0, gFalse, gFalse);
    } else {
        delete doc;
        return EXIT_FAILURE;
    }

    // process if textOut is ok
    if (textOut->isOk()) {
        globalParams = new GlobalParams();
//        globalParams->setTextPageBreaks(gTrue);
//        globalParams->setErrQuiet(gFalse);

        PDFDocument pdf_document;
        PDFSection pdf_section;
        bool start_parse = false;

        std::cout << "Processing " << doc->getNumPages() << " pages of " << argv[1] << std::endl;

        for (int page = 1; page <= doc->getNumPages(); ++page) {
            PDFRectangle* page_mediabox =  doc->getPage(page)->getMediaBox();
            double y0 = page_mediabox->y2 - pageFooterHeight;
            doc->displayPage(textOut, page, resolution, resolution, 0, gTrue, gFalse, gFalse);


            TextPage* textPage = textOut->takeText();
            std::list<TextBlockInformation*> text_block_information_list;

            for (TextFlow* flow = textPage->getFlows(); flow; flow = flow->getNext()) {
                for (TextBlock* text_block = flow->getBlocks(); text_block; text_block = text_block->getNext()) {

                    // must process text_block here as it'll expire after parsing page
                    TextBlockInformation* text_block_information = extract_text_block_information(text_block, !start_parse, y0);
                    text_block_information_list.push_back(text_block_information);

                    // if atleast 1 text block is page number block
                    if (text_block_information->is_page_number) {
                        start_parse = true; // first page that have page number
                    }
                }
            }
            textPage->decRefCnt();


            std::cout << page << ": " << start_parse << std::endl;

            // after first page which has page number
            if (start_parse) {
                for (TextBlockInformation* text_block_information : text_block_information_list) {
                    // only add blocks that is not page number
                    if (!(text_block_information->is_page_number)) {
                        if (text_block_information->partial_paragraph_content.length() > 0) {
                            if (text_block_information->has_title) {
                                if (pdf_section.title.length() > 0) {
                                    pdf_document.sections.push_back(pdf_section);
                                }

                                pdf_section.title = text_block_information->emphasized_words.front();
                                pdf_section.emphasized_words = text_block_information->emphasized_words;
                                pdf_section.content = text_block_information->partial_paragraph_content;
                            } else if (pdf_section.title.length() > 0) {
                                pdf_section.emphasized_words.insert(pdf_section.emphasized_words.end(), text_block_information->emphasized_words.begin(), text_block_information->emphasized_words.end());
                                pdf_section.content += text_block_information->partial_paragraph_content;
                            }
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
            pdf_document.sections.push_back(pdf_section);
        }

        // Save pdf_document to json file
        nlohmann::json json_pdf_document;
        for (PDFSection section : pdf_document.sections) {
            nlohmann::json json_pdf_section;
            json_pdf_section["title"] = section.title;
            json_pdf_section["content"] = section.content;
            for (std::string emphasized_word : section.emphasized_words) {
                json_pdf_section["keywords"] += emphasized_word;
            }
            json_pdf_document.push_back(json_pdf_section);
        }

        std::string output_file_name(std::string(argv[1]) + ".json");
        std::cout << "Writing out file to: " << output_file_name << std::endl;
        std::ofstream pdf_document_json_file(output_file_name);
        pdf_document_json_file << json_pdf_document.dump(4);
        pdf_document_json_file.close();

        delete textOut;
        delete doc;
        delete globalParams;
    } else {
        delete textOut;
        delete doc;
        return EXIT_FAILURE;
    }

    // check for memory leaks
    Object::memCheck(stderr);
    gMemReport(stderr);

    return EXIT_SUCCESS;
}
