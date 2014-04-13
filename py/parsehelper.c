// these functions are separate from parse.c to keep parser independent of mp_obj_t

#include <stdint.h>
#include <stdio.h>

#include "misc.h"
#include "mpconfig.h"
#include "qstr.h"
#include "lexer.h"
#include "parse.h"
#include "obj.h"
#include "parsehelper.h"

#define STR_MEMORY "parser could not allocate enough memory"
#define STR_UNEXPECTED_INDENT "unexpected indent"
#define STR_UNMATCHED_UNINDENT "unindent does not match any outer indentation level"
#define STR_INVALID_SYNTAX "invalid syntax"

void mp_parse_show_exception(mp_lexer_t *lex, mp_parse_error_kind_t parse_error_kind) {
    printf("  File \"%s\", line %d, column %d\n", qstr_str(mp_lexer_source_name(lex)), mp_lexer_cur(lex)->src_line, mp_lexer_cur(lex)->src_column);
    switch (parse_error_kind) {
        case MP_PARSE_ERROR_MEMORY:
            printf("MemoryError: %s\n", STR_MEMORY);
            break;

        case MP_PARSE_ERROR_UNEXPECTED_INDENT:
            printf("IndentationError: %s\n", STR_UNEXPECTED_INDENT);
            break;

        case MP_PARSE_ERROR_UNMATCHED_UNINDENT:
            printf("IndentationError: %s\n", STR_UNMATCHED_UNINDENT);
            break;

        case MP_PARSE_ERROR_INVALID_SYNTAX:
        default:
            printf("SyntaxError: %s\n", STR_INVALID_SYNTAX);
            break;
    }
}

mp_obj_t mp_parse_make_exception(mp_lexer_t *lex, mp_parse_error_kind_t parse_error_kind) {
    // make exception object
    mp_obj_t exc;
    switch (parse_error_kind) {
        case MP_PARSE_ERROR_MEMORY:
            exc = mp_obj_new_exception_msg(&mp_type_MemoryError, STR_MEMORY);
            break;

        case MP_PARSE_ERROR_UNEXPECTED_INDENT:
            exc = mp_obj_new_exception_msg(&mp_type_IndentationError, STR_UNEXPECTED_INDENT);
            break;

        case MP_PARSE_ERROR_UNMATCHED_UNINDENT:
            exc = mp_obj_new_exception_msg(&mp_type_IndentationError, STR_UNMATCHED_UNINDENT);
            break;

        case MP_PARSE_ERROR_INVALID_SYNTAX:
        default:
            exc = mp_obj_new_exception_msg(&mp_type_SyntaxError, STR_INVALID_SYNTAX);
            break;
    }

    // add traceback to give info about file name and location
    // we don't have a 'block' name, so just pass the NULL qstr to indicate this
    mp_obj_exception_add_traceback(exc, mp_lexer_source_name(lex), mp_lexer_cur(lex)->src_line, MP_QSTR_NULL);

    return exc;
}
