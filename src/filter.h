#ifndef FILTER_H
#define FILTER_H
#include <stddef.h>
#include <stdbool.h>

typedef enum {
  FILTER_IDENTITY,   // .
  FILTER_FIELD,      // .chiave
  FILTER_ARRAY_INDEX // .[0]
} FilterType;

typedef struct Filter {
  FilterType type;
  char *key;           // Usato se type == FILTER_FIELD
  size_t index;        // Usato se type == FILTER_ARRAY_INDEX
  struct Filter *next; // Prossimo step nella pipeline
  bool suppress_error; // utilizzato per sopprimere eventuali errori e passare oltre
} Filter;

void filter_free(Filter *filter);

#endif // FILTER_H
