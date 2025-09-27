#pragma once

class ISharedPointerTarget
{
public:
	virtual ~ISharedPointerTarget() = 0;
};

class ZSharedPointerTarget : public ISharedPointerTarget
{
public:
	int m_iRefCount;
};
