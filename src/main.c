#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "filter.h"
#include "filter_engine.h"
#include "filter_parser.h"
#include "json_value.h"
#include "parser.h"
#include "print.h"

/**
 * Consuma tutti gli spazi bianchi iniziali nello stream per verificare
 * se abbiamo raggiunto la fine naturale del file (EOF).
 * Restituisce 1 se siamo a EOF, 0 altrimenti (ripristinando il primo carattere non-whitespace).
 */
static int is_stream_at_eof(FILE* fp) {
    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (!isspace(c)) {
            ungetc(c, fp);
            return 0;  // Trovato un altro token da parsare
        }
    }
    return 1;  // Raggiunto EOF reale senza altri caratteri utili
}

int main(int argc, char* argv[]) {
    const char* filter_str = ".";
    FILE* input_fp = stdin;
    int need_close_file = 0;

    // 1. Risoluzione degli argomenti da linea di comando
    if (argc == 1) {
        // Nessun argomento: usa '.' di default e legge da stdin (es. echo '...' | ./bin/ccjq)
        filter_str = ".";
    } else if (argc == 2) {
        // Un solo argomento: è la stringa di filtro, legge da stdin
        filter_str = argv[1];
    } else {
        // Due o più argomenti: il primo è il filtro, il secondo è il percorso file
        filter_str = argv[1];
        input_fp = fopen(argv[2], "r");
        if (input_fp == NULL) {
            fprintf(stderr, "Errore: impossibile aprire il file '%s'\n", argv[2]);
            return 1;
        }
        need_close_file = 1;
    }

    // 2. Compilazione del filtro jq
    Filter* filter = filter_compile(filter_str);
    if (filter == NULL) {
        fprintf(stderr, "Errore: compilazione del filtro '%s' fallita\n", filter_str);
        if (need_close_file) {
            fclose(input_fp);
        }
        return 1;
    }

    int exit_code = 0;

    // 3. Ciclo di lettura per stream multi-JSON / NDJSON
    while (1) {
        // Verifica se ci sono ancora dati da processare
        if (is_stream_at_eof(input_fp)) {
            break;  // Fine regolare dello stream
        }

        // Parsing del singolo documento JSON
        JsonValue* root = parse_json(input_fp);
        if (root == NULL) {
            fprintf(stderr, "Errore: parsing JSON non riuscito o sintassi non valida\n");
            exit_code = 1;
            break;  // Fail-fast al primo errore di sintassi
        }

        // Esecuzione della pipeline di filtri sul JSON corrente
        JsonValue* result = filter_run(root, filter);
        if (result != NULL) {
            json_value_dump(result);
            putchar('\n');
        }

        // Liberiamo l'albero del JSON appena processato
        json_value_free(root);
    }

    // 4. Cleanup finale delle risorse
    filter_free(filter);

    if (need_close_file) {
        fclose(input_fp);
    }

    return exit_code;
}
