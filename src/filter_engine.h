#ifndef FILTER_ENGINE_H
#define FILTER_ENGINE_H
#include "json_value.h"
#include "filter.h"

JsonValue* filter_run(JsonValue* val, Filter* filter);

#endif // FILTER_ENGINE_H
