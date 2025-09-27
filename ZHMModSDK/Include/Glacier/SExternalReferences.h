#include "THashMap.h"
#include "ZEntity.h"

struct SExternalReferences
{
    THashMap<ZRuntimeResourceID, ZEntityType**, TDefaultHashMapPolicy<ZRuntimeResourceID>> externalScenes;
};
