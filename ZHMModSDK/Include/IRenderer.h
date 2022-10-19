#pragma once

#include "Glacier/ZMath.h"
#include "Glacier/ZString.h"

enum class TextAlignment
{
	Left,
	Center,
	Right,
};

class IRenderer
{
public:
	virtual void DrawLine3D(const SVector3& p_From, const SVector3& p_To, const SVector4& p_FromColor, const SVector4& p_ToColor) = 0;
	virtual void DrawText2D(const ZString& p_Text, const SVector2& p_Pos, const SVector4& p_Color, float p_Rotation = 0.f, float p_Scale = 1.f, TextAlignment p_Alignment = TextAlignment::Center) = 0;
	virtual bool WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) = 0;
	virtual bool ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) = 0;
	virtual void DrawBox3D(const SVector3& p_Min, const SVector3& p_Max, const SVector4& p_Color) = 0;
	virtual void DrawOBB3D(const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform, const SVector4& p_Color) = 0;
};
