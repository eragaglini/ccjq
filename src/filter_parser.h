#ifndef FILTER_PARSER_H
#define FILTER_PARSER_H
#include "json_value.h"
#include "filter.h"

/// @brief Error codes for library "parser"
typedef enum filter_parser_error {
    /// No error
    FILTER_PARSER_ERROR_OK = 0,
    /// Out of memory (RAM)
    FILTER_PARSER_ERROR_NOMEM,
    FILTER_PARSER_ERROR_SYNTAX,
    // etc etc
    /// Total # of errors in this list (NOT AN ACTUAL ERROR CODE);
    /// NOTE: that for this to work, it assumes your first error code is value 0 and you let it
    /// naturally increment from there, as is done above, without explicitly altering any error
    /// values above
    FILTERPARSER_ERROR_COUNT,
} filter_parser_error_t;

Filter* filter_compile(const char* filter_str);

#endif // FILTER_PARSER_H
