#include "THashMap.h"
#include "ZEntity.h"

struct SExternalReferences {
    THashMap<ZRuntimeResourceID, ZEntityRef> externalScenes;
};