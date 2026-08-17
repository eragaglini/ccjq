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
    const char* filter_str = ".";  // Default se non viene passato alcun filtro
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

    // 1. Parsing del JSON in ingresso
    JsonValue* root = parse_json(input_stream);

    if (input_stream != stdin) {
        fclose(input_stream);
    }

    if (root == NULL) {
        fprintf(stderr, "Errore: JSON non valido!\n");
        return EXIT_FAILURE;
    }

    // 2. Compilazione della pipeline di filtri
    Filter* filter = filter_compile(filter_str);
    if (filter == NULL) {
        json_value_free(root);
        return EXIT_FAILURE;
    }

    // 3. Esecuzione del filtro
    JsonValue* result = filter_run(root, filter);

    // 4. Stampa del risultato
    if (result != NULL) {
        json_value_dump(result);
        printf("\n");
    } else {
        printf("null\n");
    }

    // 5. Deallocazione della pipeline e dell'albero JSON
    json_value_free(root);
    filter_free(filter);

    return EXIT_SUCCESS;
}
