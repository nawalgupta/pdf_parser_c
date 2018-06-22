/*
 * Parse pdf file, write output to .json file using poppler
 * with one pdf file:
 * user password: the password to open/read pdf file
 * owner password: the password to print, edit, extract, comment, ...
 * ctm: Current Transformation Matrix
 * gfx: Graphic
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

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

void print_node_info(Object* obj) {
    switch (obj->getType()) {
        case objBool:
            std::cout << "Type: BOOL: " << (obj->getBool() ? "TRUE" : "FALSE") << std::endl;
            break;
        case objInt:
            std::cout << "Type: INT: " << obj->getInt() << std::endl;
            break;
        case objReal:
            std::cout << "Type: REAL " << obj->getReal() << std::endl;
            break;
        case objString:
            std::cout << "Type: STRING " << obj->getString()->toStr() << std::endl;
            break;
        case objName:
            std::cout << "Type: NAME: " << obj->getName() << std::endl;
            break;
        case objNull:
            std::cout << "Type: NULL" << std::endl;
            break;
        case objArray: {
                std::cout << "Type: ARRAY" << std::endl;
                Array* arr = obj->getArray();
                int array_length = arr->getLength();
                for (int i = 0; i < array_length; ++i) {
                    Object array_element = arr->get(i);
                    print_node_info(&array_element);
                }
            }
            break;
        case objDict: {
                std::cout << "Type: DICTIONARY" << std::endl;
                Dict* dict = obj->getDict();
                int dict_size = dict->getLength();
                for (int i = 0; i < dict_size; ++i) {
                    std::cout << "Key " << i << ": " << dict->getKey(i) << std::endl;
                    std::cout << "Value " << i << ": " << std::endl;
                    Object dictionary_value = dict->getVal(i);
                    print_node_info(&dictionary_value);
                }
            }
            break;
        case objStream: {
                Stream* stream = obj->getStream();
                std::cout << "Type: STREAM, kind: " << stream->getKind() << std::endl;
                if (stream->isBinary()) {
                    std::cout << "Binary stream" << std::endl;
//                    stream->getRawChars();
                } else {
                    std::cout << "Not binary stream" << std::endl;
                }
            }
            break;
        case objRef:
            std::cout << "Type: INDIRECT REFERENCE: " << obj->getRefNum() << "-" << obj->getRefGen();
            break;
        case objCmd:
            std::cout << "Type: COMMAND" << std::endl;
            break;
        case objError:
            std::cout << "Type: ERROR" << std::endl;
            break;
        case objEOF:
            std::cout << "Type: EOF" << std::endl;
            break;
        case objNone:
            std::cout << "Type: NONE" << std::endl;
            break;
        case objInt64:
            std::cout << "Type: INT64: " << obj->getInt64() << std::endl;
            break;
        case objDead:
            std::cout << "Type: DEAD" << std::endl;
            break;
        default:
            std::cout << "Type: UNKNOWN" << std::endl;
            break;
    }
}

int main(int argc, char* argv[]) {
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
            std::cout << "Parsing page " << page << " " << page_mediabox->x1 << ", " << page_mediabox->y1 << ", " << page_mediabox->x2 << ", " << page_mediabox->y2 << std::endl;
            doc->displayPage(textOut, page, resolution, resolution, 0, gTrue, gFalse, gFalse);
            TextPage* textPage = textOut->takeText();

            for (TextFlow* flow = textPage->getFlows(); flow; flow = flow->getNext()) {
                for (TextBlock* blk = flow->getBlocks(); blk; blk = blk->getNext()) {
                    for (TextLine* line = blk->getLines(); line; line = line->getNext()) {
                        for (TextWord* word = line->getWords(); word; word = word->getNext()) {
                            std::cout << word->getText()->toStr() << " ";
                        }
                        std::cout << std::endl;
                    }
                }
            }
            textPage->decRefCnt();
            //        break;
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
