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
#include <FontInfo.h>
#include "pdf_utils.hpp"

// recursive
nlohmann::json add_json_node(DocumentNode& current_node) {
    nlohmann::json json_pdf_section;
    json_pdf_section["title"] = current_node.main_section->title;
    json_pdf_section["content"] = current_node.main_section->content;
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
        int number_of_pages = doc->getNumPages();

        PDFDocument pdf_document;
        PDFSection pdf_section;
        bool start_parse = false;


        /* ------------------------- print all fonts used, for testing only -------------- */
        /*

        FontInfoScanner font_info_scanner(doc);
        GooList *fonts = font_info_scanner.scan(number_of_pages);
        // print the font info
        printf("List fonts used:\n");
        printf("name                                 type              encoding         emb sub uni object ID\n");
        printf("------------------------------------ ----------------- ---------------- --- --- --- ---------\n");
        if (fonts) {
          for (int i = 0; i < fonts->getLength(); ++i) {
            FontInfo *font = (FontInfo *)fonts->get(i);
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

        */

        std::cout << "Parsing " << number_of_pages << " pages of " << argv[1] << std::endl;

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
        root_section.title = file_path;
        root_section.content = "";
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

        // Save pdf_document to json file
        nlohmann::json json_pdf_document = add_json_node(doc_root);

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
