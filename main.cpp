/*
 * Parse pdf file, write output to .json file using poppler
 * with one pdf file:
 * user password: the password to open/read pdf file
 * owner password: the password to print, edit, extract, comment, ...
 * ctm: Current Transformation Matrix
 * gfx: Graphic
 * To extract page in body of pages, specify -x, -y, -W, -H flag
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <list>
#include <regex>
#include "config.h"
#include "poppler-config.h"
#include "utils/parseargs.h"
#include "utils/printencodings.h"
#include "utils/Win32Console.h"
#include "goo/GooString.h"
#include "goo/gmem.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "Page.h"
#include "PDFDocFactory.h"
#include "TextOutputDev.h"
#include "CharTypes.h"
#include "UnicodeMap.h"
#include "PDFDocEncoding.h"
#include "Parser.h"
#include "Lexer.h"
#include "Error.h"

static int firstPage = 1;
static int lastPage = 0;
static double resolution = 72.0;
static int x = 0;
static int y = 0;
static int w = 0;
static int h = 0;
static GBool bbox = gFalse;
static GBool bboxLayout = gFalse;
static GBool physLayout = gFalse;
static double fixedPitch = 0;
static GBool rawOrder = gFalse;
static GBool htmlMeta = gFalse;
static char textEncName[128] = "";
static char textEOL[16] = "";
static GBool noPageBreaks = gFalse;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static GBool quiet = gFalse;
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;
static GBool printEnc = gFalse;

static const ArgDesc argDesc[] = {
    {
        "-f",       argInt,      &firstPage,     0,
        "first page to convert"
    },
    {
        "-l",       argInt,      &lastPage,      0,
        "last page to convert"
    },
    {
        "-r",      argFP,       &resolution,    0,
        "resolution, in DPI (default is 72)"
    },
    {
        "-x",      argInt,      &x,             0,
        "x-coordinate of the crop area top left corner"
    },
    {
        "-y",      argInt,      &y,             0,
        "y-coordinate of the crop area top left corner"
    },
    {
        "-W",      argInt,      &w,             0,
        "width of crop area in pixels (default is 0)"
    },
    {
        "-H",      argInt,      &h,             0,
        "height of crop area in pixels (default is 0)"
    },
    {
        "-layout",  argFlag,     &physLayout,    0,
        "maintain original physical layout"
    },
    {
        "-fixed",   argFP,       &fixedPitch,    0,
        "assume fixed-pitch (or tabular) text"
    },
    {
        "-raw",     argFlag,     &rawOrder,      0,
        "keep strings in content stream order"
    },
    {
        "-htmlmeta", argFlag,   &htmlMeta,       0,
        "generate a simple HTML file, including the meta information"
    },
    {
        "-enc",     argString,   textEncName,    sizeof(textEncName),
        "output text encoding name"
    },
    {
        "-listenc", argFlag,     &printEnc,      0,
        "list available encodings"
    },
    {
        "-eol",     argString,   textEOL,        sizeof(textEOL),
        "output end-of-line convention (unix, dos, or mac)"
    },
    {
        "-nopgbrk", argFlag,     &noPageBreaks,  0,
        "don't insert page breaks between pages"
    },
    {
        "-bbox", argFlag,     &bbox,  0,
        "output bounding box for each word and page size to html.  Sets -htmlmeta"
    },
    {
        "-bbox-layout", argFlag,     &bboxLayout,  0,
        "like -bbox but with extra layout bounding box data.  Sets -htmlmeta"
    },
    {
        "-opw",     argString,   ownerPassword,  sizeof(ownerPassword),
        "owner password (for encrypted files)"
    },
    {
        "-upw",     argString,   userPassword,   sizeof(userPassword),
        "user password (for encrypted files)"
    },
    {
        "-q",       argFlag,     &quiet,         0,
        "don't print any messages or errors"
    },
    {
        "-v",       argFlag,     &printVersion,  0,
        "print copyright and version info"
    },
    {
        "-h",       argFlag,     &printHelp,     0,
        "print usage information"
    },
    {
        "-help",    argFlag,     &printHelp,     0,
        "print usage information"
    },
    {
        "--help",   argFlag,     &printHelp,     0,
        "print usage information"
    },
    {
        "-?",       argFlag,     &printHelp,     0,
        "print usage information"
    },
    {}
};

std::string UnicodeToUTF8(Unicode codepoint) {
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

struct TextBlockInformation {
    bool has_title = false;
    std::list<std::string> emphasized_words;
    std::string partial_paragraph_content;
};

TextBlockInformation* extract_text_block_information(TextBlock* text_block) {
    TextBlockInformation* text_block_information = new TextBlockInformation;
    std::stringstream partial_paragraph_content_string_stream;
    std::stringstream emphasized_word_string_stream;
    bool parsing_emphasized_word = false;
    std::regex font_regex(".*([bB]old|[iI]talic).*");
    std::smatch base_match; // string match
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
                if (std::regex_match(font_name, base_match, font_regex)) {
                    parsing_emphasized_word = true;
                    emphasized_word_string_stream << character;
                } else if (parsing_emphasized_word) {
                    // TODO: check stripped emphasized word, not emphasized word
                    if (emphasized_word_string_stream.str().length() > 0) {
                        // TODO: push stripped emphasized word, not emphasized word
                        text_block_information->emphasized_words.push_back(emphasized_word_string_stream.str());
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
    // TODO: check stripped emphasized word, not emphasized word
    if (parsing_emphasized_word && emphasized_word_string_stream.str().length() > 0) {
        // TODO: push stripped emphasized word, not emphasized word
        text_block_information->emphasized_words.push_back(emphasized_word_string_stream.str());
    }

    // test
    for (std::string emphasized_word : text_block_information->emphasized_words) {
        std::cout << emphasized_word << std::endl;
    }
    std::cout << text_block_information->partial_paragraph_content << std::endl;

    return text_block_information;
}

int main(int argc, char* argv[]) {

//    // Test UTF-8 encoded Unicode
//    std::string s = UnicodeToUTF8(67222);
//    std::cout << s << " " << s.length() << std::endl;
//    exit(0);

    PDFDoc* doc;
    GooString* fileName;
    GooString* textFileName;
    GooString* ownerPW, *userPW;
    TextOutputDev* textOut;
    FILE* f;
    UnicodeMap* uMap;
    Object info;
    GBool ok;
    char* p;
    int exitCode;
    Win32Console win32Console(&argc, &argv);
    exitCode = 99;

    // parse args
    ok = parseArgs(argDesc, &argc, argv);

    if (bboxLayout) {
        bbox = gTrue;
    }

    if (bbox) {
        htmlMeta = gTrue;
    }

    if (!ok || (argc < 2 && !printEnc) || argc > 3 || printVersion || printHelp) {
        fprintf(stderr, "pdftotext version %s\n", PACKAGE_VERSION);
        fprintf(stderr, "%s\n", popplerCopyright);
        fprintf(stderr, "%s\n", xpdfCopyright);
        if (!printVersion) {
            printUsage("pdftotext", "<PDF-file> [<text-file>]", argDesc);
        }
        if (printVersion || printHelp)
            exitCode = 0;
        goto err0;
    }

    // read config file
    globalParams = new GlobalParams();
    if (printEnc) {
        printEncodings();
        delete globalParams;
        exitCode = 0;
        goto err0;
    }

    fileName = new GooString(argv[1]);

    if (fixedPitch) {
        physLayout = gTrue;
    }

    if (textEncName[0]) {
        globalParams->setTextEncoding(textEncName);
    }

    if (textEOL[0]) {
        if (!globalParams->setTextEOL(textEOL)) {
            fprintf(stderr, "Bad '-eol' value on command line\n");
        }
    }

    if (noPageBreaks) {
        globalParams->setTextPageBreaks(gFalse);
    }

    if (quiet) {
        globalParams->setErrQuiet(quiet);
    }

    // get mapping to output encoding
    if (!(uMap = globalParams->getTextEncoding())) {
        error(errCommandLine, -1, "Couldn't get text encoding");
        delete fileName;
        goto err1;
    }

    // open PDF file
    if (ownerPassword[0] != '\001') {
        ownerPW = new GooString(ownerPassword);
    } else {
        ownerPW = nullptr;
    }

    if (userPassword[0] != '\001') {
        userPW = new GooString(userPassword);
    } else {
        userPW = nullptr;
    }

    if (fileName->cmp("-") == 0) {
        delete fileName;
        fileName = new GooString("fd://0");
    }

    doc = PDFDocFactory().createPDFDoc(*fileName, ownerPW, userPW);

    if (userPW) {
        delete userPW;
    }

    if (ownerPW) {
        delete ownerPW;
    }

    if (!doc->isOk()) {
        exitCode = 1;
        goto err2;
    }

#ifdef ENFORCE_PERMISSIONS
    // check for copy permission
    if (!doc->okToCopy()) {
        error(errNotAllowed, -1, "Copying of text from this document is not allowed.");
        exitCode = 3;
        goto err2;
    }
#endif

    // construct text file name
    if (argc == 3) {
        textFileName = new GooString(argv[2]);
    } else if (fileName->cmp("fd://0") == 0) {
        error(errCommandLine, -1, "You have to provide an output filename when reading form stdin.");
        goto err2;
    } else {
        p = fileName->getCString() + fileName->getLength() - 4;
        if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")) {
            textFileName = new GooString(fileName->getCString(),
                                         fileName->getLength() - 4);
        } else {
            textFileName = fileName->copy();
        }
        textFileName->append(htmlMeta ? ".html" : ".txt");
    }

    // get page range
    if (firstPage < 1) {
        firstPage = 1;
    }

    if (lastPage < 1 || lastPage > doc->getNumPages()) {
        lastPage = doc->getNumPages();
    }

    if (lastPage < firstPage) {
        error(errCommandLine, -1,
              "Wrong page range given: the first page ({0:d}) can not be after the last page ({1:d}).",
              firstPage, lastPage);
        goto err3;
    }

    textOut = new TextOutputDev(nullptr, physLayout, fixedPitch, rawOrder, htmlMeta);

    if (textOut->isOk()) {
        for (int page = firstPage; page <= lastPage; ++page) {
            PDFRectangle* page_mediabox =  doc->getPage(page)->getMediaBox();
//            std::cout << "Parsing page " << page << " " << page_mediabox->x1 << ", " << page_mediabox->y1 << ", " << page_mediabox->x2 << ", " << page_mediabox->y2 << std::endl;
            if ((w == 0) && (h == 0) && (x == 0) && (y == 0)) {
                doc->displayPage(textOut, page, resolution, resolution, 0, gTrue, gFalse, gFalse);
            } else {
                doc->displayPageSlice(textOut, page, resolution, resolution, 0, gTrue, gFalse, gFalse, x, y, w, h);
            }

            TextPage* textPage = textOut->takeText();

            for (TextFlow* flow = textPage->getFlows(); flow; flow = flow->getNext()) {
                for (TextBlock* text_block = flow->getBlocks(); text_block; text_block = text_block->getNext()) {
                    // must process text_block here as it'll expire after parsing page
                    TextBlockInformation* text_block_information = extract_text_block_information(text_block);

                    delete text_block_information;
                }
            }
            textPage->decRefCnt();
            break;
        }
    } else {
        delete textOut;
        exitCode = 2;
        goto err3;
    }

    delete textOut;

    // finish success
    exitCode = 0;

    // clean up
err3:
    delete textFileName;

err2:
    delete doc;
    delete fileName;
    uMap->decRefCnt();

err1:
    delete globalParams;

err0:
    // check for memory leaks
    Object::memCheck(stderr);
    gMemReport(stderr);
    return exitCode;
}
