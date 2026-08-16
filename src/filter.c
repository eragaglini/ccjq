
#include "filter.h"
#include <stdlib.h>

void filter_free(Filter *filter) {
  while (filter != NULL) {
    Filter *next =
        filter->next; // Salva il puntatore successivo prima di liberare

    if (filter->key) {
      free(filter->key); // Libera la stringa se duplicata con strdup()
    }

    free(filter); // Libera il nodo corrente
    filter = next;
  }
}
