#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "print.h"
#include "tokenizer.h"

int main(int argc, char* argv[]) {
    FILE* input_stream = stdin;

    if (argc > 1) {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            perror("Errore nell'apertura del file");
            return EXIT_FAILURE;
        }
    }

    // Controllo stream vuoto (funziona sia per file che per stdin)
    int c = fgetc(input_stream);
    if (c == EOF) {
        fprintf(stderr, "Errore: lo stream di input è vuoto!\n");
        if (input_stream != stdin) {
            fclose(input_stream);
        }
        return EXIT_FAILURE;
    }
    ungetc(c, input_stream);

    // 1. Eseguiamo il parsing
    JsonValue* jv = parse_json(input_stream);

    // 2. Pulizia del file
    if (input_stream != stdin) {
        fclose(input_stream);
    }

    // 3. Gestione dell'esito finale
    if (jv == NULL) {
        fprintf(stderr, "JSON non valido!\n");
        return EXIT_FAILURE;
    }

    json_value_dump(jv);

    json_value_free(jv);

    return EXIT_SUCCESS;
}
