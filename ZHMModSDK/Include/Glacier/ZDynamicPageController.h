#pragma once

#include "ZObject.h"
#include "ZResourceID.h"

class ZDynamicPageController {
public:
    PAD(0x380);
    ZDynamicObject m_Template;
    ZRuntimeResourceID m_TemplateResourceID;
};

static_assert(offsetof(ZDynamicPageController, m_Template) == 0x380);
static_assert(offsetof(ZDynamicPageController, m_TemplateResourceID) == 0x390);
