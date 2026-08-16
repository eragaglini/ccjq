#ifndef FILTER_ENGINE_H
#define FILTER_ENGINE_H
#include "json_value.h"
#include "filter.h"

JsonValue* run_filter(JsonValue* jv, Filter* filter);

#endif // FILTER_ENGINE_H
