#pragma once

#include <stdio.h>
#include "json_value.h"

// Stampa un JsonValue formattato (pretty print con rientro/indentazione)
void json_value_print(FILE *stream, const JsonValue *val, int indent_level);

// Helper comodo per stampare su stdout con indentazione iniziale 0
void json_value_dump(const JsonValue *val);
