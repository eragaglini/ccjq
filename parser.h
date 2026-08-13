#pragma once
#include <stdbool.h>
#include "json_value.h"

#include "tokenizer.h"

/// @brief Error codes for library "parser"
typedef enum json_parser_error {
    /// No error
    JSONPARSER_ERROR_OK = 0,
    /// Out of memory (RAM)
    JSONPARSER_ERROR_NOMEM,
    JSONPARSER_ERROR_SYNTAX,
    // etc etc
    /// Total # of errors in this list (NOT AN ACTUAL ERROR CODE);
    /// NOTE: that for this to work, it assumes your first error code is value 0 and you let it
    /// naturally increment from there, as is done above, without explicitly altering any error
    /// values above
    JSONPARSER_ERROR_COUNT,
} json_parser_error_t;

JsonValue* parse_json(FILE *stream);
JsonObject* parse_object(Tokenizer* tokenizer);
JsonValue* parse_array(Tokenizer* tokenizer);
