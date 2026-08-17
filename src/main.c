#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "filter.h"
#include "filter_engine.h"
#include "filter_parser.h"
#include "parser.h"
#include "print.h"
#include "tokenizer.h"

int main(int argc, char* argv[]) {
    const char* filter_str = ".";  // Default per argc == 1
    FILE* input_stream = stdin;

    if (argc == 2) {
        filter_str = argv[1];
    } else if (argc == 3) {
        filter_str = argv[1];
        input_stream = fopen(argv[2], "r");
        if (input_stream == NULL) {
            perror("Errore nell'apertura del file");
            return EXIT_FAILURE;
        }
    } else if (argc > 3) {
        fprintf(stderr, "Uso: %s [filtro] [file.json]\n", argv[0]);
        return EXIT_FAILURE;
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

    // 1. Parsing del JSON
    JsonValue* root = parse_json(input_stream);

    if (input_stream != stdin) {
        fclose(input_stream);
    }

    if (root == NULL) {
        fprintf(stderr, "JSON non valido!\n");
        return EXIT_FAILURE;
    }

    // 2. Valutazione del filtro
    Filter* filter = filter_compile(filter_str);
    if (filter == NULL) {
        // Se il filtro è sintatticamente errato
        json_value_free(root);
        return EXIT_FAILURE;
    }

    JsonValue* result = filter_run(root, filter);

    // 3. Stampa del risultato
    json_value_dump(result);

    // 4. Liberazione dell'albero originale
    json_value_free(root);

    // 5. Liberazione della struttura filter
    filter_free(filter);

    // 6. Liberazione del risultato
    json_value_free(result);

    return EXIT_SUCCESS;
}
