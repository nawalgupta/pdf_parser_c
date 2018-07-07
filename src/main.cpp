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
#include <algorithm>
#include <cctype>
#include <locale>
#include <nlohmann/json.hpp>
#include "pdf_utils.hpp"

int main(int argc, char* argv[]) {
    PDFDoc* doc;
    TextOutputDev* textOut;

    int title_max_length = 100;
    int page_footer_height = 60.0;
    char owner_password[33] = "\001";
    char user_password[33] = "\001";
    double resolution = 72.0;
    char* file_path = argv[1];

    // parse args
    if (argc > 1) {
        doc = open_pdf_document(file_path, owner_password, user_password);
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


//            std::cout << page << ": " << start_parse << std::endl;

            // after first page which has page number
            if (start_parse) {
                for (TextBlockInformation* text_block_information : text_block_information_list) {
                    // only add blocks that is not page number
                    if (!(text_block_information->is_page_number)) {
                        if (text_block_information->partial_paragraph_content.length() > 0) {
                            if (text_block_information->title_format) {
                                if (pdf_section.title.length() > 0) {
                                    pdf_section.content.pop_back();
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
            pdf_section.content.pop_back();
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

    return EXIT_SUCCESS;
}
