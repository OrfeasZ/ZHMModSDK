#include "ZActor.h"

uint16_t* ZActor::s_nextActorID = nullptr;

ZActor::OnOutfitChanged_t ZActor::OnOutfitChanged = nullptr;
ZActor::ReviveActor_t ZActor::ReviveActor = nullptr;