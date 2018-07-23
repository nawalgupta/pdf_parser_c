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

#include <fstream>
#include "pdf_utils.hpp"

int main(int argc, char* argv[]) {
    PDFDoc* doc;

    char owner_password[33] = "\001";
    char user_password[33] = "\001";
    char* file_path = argv[1];

    // parse args
    if (argc > 1) {
        doc = open_pdf_document(file_path, owner_password, user_password);
    } else {
        return EXIT_FAILURE;
    }

    std::string output_file_name(std::string(file_path) + ".json");
    std::ofstream pdf_document_json_file(output_file_name);
    pdf_document_json_file << std::move(parse_pdf_document(doc));
    pdf_document_json_file.close();

    return EXIT_SUCCESS;
}
