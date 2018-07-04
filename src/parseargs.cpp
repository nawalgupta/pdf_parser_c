#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <goo/gstrtod.h>
#include <goo/GooString.h>
#include "parseargs.hpp"

static const ArgDesc* findArg(const ArgDesc* args, char* arg);
static GBool grabArg(const ArgDesc* arg, int i, int* argc, char* argv[]);

GBool parseArgs(const ArgDesc* args, int* argc, char* argv[]) {
    const ArgDesc* arg;
    int i, j;
    GBool ok;
    ok = gTrue;
    i = 1;
    while (i < *argc) {
        if (!strcmp(argv[i], "--")) {
            --*argc;
            for (j = i; j < *argc; ++j)
                argv[j] = argv[j + 1];
            break;
        } else if ((arg = findArg(args, argv[i]))) {
            if (!grabArg(arg, i, argc, argv))
                ok = gFalse;
        } else {
            ++i;
        }
    }
    return ok;
}

static const ArgDesc* findArg(const ArgDesc* args, char* arg) {
    const ArgDesc* p;
    for (p = args; p->arg; ++p) {
        if (p->kind < argFlagDummy && !strcmp(p->arg, arg))
            return p;
    }
    return nullptr;
}

static GBool grabArg(const ArgDesc* arg, int i, int* argc, char* argv[]) {
    int n;
    int j;
    GBool ok;
    ok = gTrue;
    n = 0;
    switch (arg->kind) {
        case argFlag:
            *(GBool*)arg->val = gTrue;
            n = 1;
            break;
        case argInt:
            if (i + 1 < *argc && isInt(argv[i + 1])) {
                *(int*)arg->val = atoi(argv[i + 1]);
                n = 2;
            } else {
                ok = gFalse;
                n = 1;
            }
            break;
        case argFP:
            if (i + 1 < *argc && isFP(argv[i + 1])) {
                *(double*)arg->val = gatof(argv[i + 1]);
                n = 2;
            } else {
                ok = gFalse;
                n = 1;
            }
            break;
        case argString:
            if (i + 1 < *argc) {
                strncpy((char*)arg->val, argv[i + 1], arg->size - 1);
                ((char*)arg->val)[arg->size - 1] = '\0';
                n = 2;
            } else {
                ok = gFalse;
                n = 1;
            }
            break;
        case argGooString:
            if (i + 1 < *argc) {
                ((GooString*)arg->val)->Set(argv[i + 1]);
                n = 2;
            } else {
                ok = gFalse;
                n = 1;
            }
            break;
        default:
            fprintf(stderr, "Internal error in arg table\n");
            n = 1;
            break;
    }
    if (n > 0) {
        *argc -= n;
        for (j = i; j < *argc; ++j)
            argv[j] = argv[j + n];
    }
    return ok;
}

GBool isInt(char* s) {
    if (*s == '-' || *s == '+')
        ++s;
    while (isdigit(*s))
        ++s;
    if (*s)
        return gFalse;
    return gTrue;
}

GBool isFP(char* s) {
    int n;
    if (*s == '-' || *s == '+')
        ++s;
    n = 0;
    while (isdigit(*s)) {
        ++s;
        ++n;
    }
    if (*s == '.')
        ++s;
    while (isdigit(*s)) {
        ++s;
        ++n;
    }
    if (n > 0 && (*s == 'e' || *s == 'E')) {
        ++s;
        if (*s == '-' || *s == '+')
            ++s;
        n = 0;
        if (!isdigit(*s))
            return gFalse;
        do {
            ++s;
        } while (isdigit(*s));
    }
    if (*s)
        return gFalse;
    return gTrue;
}
