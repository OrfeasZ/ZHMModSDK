#pragma once

#include "Reflection.h"
#include "THashMap.h"
#include "ZString.h"
#include "Reflection.h"
#include "TArray.h"

class ZTypeRegistry
{
public:
	virtual int addRef() = 0;
	virtual int release() = 0;
	virtual void Unk00() = 0;
	virtual void Unk01() = 0;
	virtual ~ZTypeRegistry() = 0;

public:
	PAD(56);
	THashMap<ZString, STypeID*> m_types;
	TArray<IType*> m_unnamedTypes;
};