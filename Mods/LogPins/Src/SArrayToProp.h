#pragma once

#include <map>
#include <numbers>
#include <vector>
#include <string>
#include <sstream>

#include <Glacier/ZEntity.h>

class /*alignas(4)*/ SColorRGB
{
public:
	float32 r; // 0x0
	float32 g; // 0x4
	float32 b; // 0x8
};

enum class PropTypes
{
	t_bool,
	t_int,
	t_enum,
	t_string,
	t_guid,
	t_float,
	t_color,
	t_matrix43,
	t_entityRef,
	t_arrayEntityRef,
};

std::map<std::string, PropTypes> mapOfTypes = {
	{"bool", PropTypes::t_bool},
	{"int32", PropTypes::t_int},
	{"enum", PropTypes::t_enum},
	{"float32", PropTypes::t_float},
	{"ZString", PropTypes::t_string},
	{"ZGuid", PropTypes::t_guid},
	{"SColorRGB", PropTypes::t_color},
	{"SMatrix43", PropTypes::t_matrix43},
	{"SEntityTemplateReference", PropTypes::t_entityRef},
	{"TArray<SEntityTemplateReference>", PropTypes::t_arrayEntityRef},
};

inline SMatrix43 EularToMatrix43(float32 x, float32 y, float32 z, float32 rx, float32 ry, float32 rz)
{
	float pitch = rx * std::numbers::pi / 180.f;
	float roll = ry * std::numbers::pi / 180.f;
	float yaw = rz * std::numbers::pi / 180.f;

	float a = cos(pitch);
	float b = sin(pitch);
	float c = cos(roll);
	float d = sin(roll);
	float e = cos(yaw);
	float f = sin(yaw);
	float ae = a * e, af = a * f, be = b * e, bf = b * f;

	SMatrix43 newTrans = SMatrix43();

	newTrans.XAxis = SVector3(c * e, -c * f, d);
	newTrans.YAxis = SVector3(af + be * d, ae - bf * d, -b * c);
	newTrans.ZAxis = SVector3(bf - ae * d, be + af * d, a * c);

	newTrans.Trans = SVector3(x, y, z);


	return newTrans;
}

inline void OverrideEnumPropValue(ZEntityRef entity, std::string propNameToFind, int32_t value)
{
	TArray<ZEntityProperty>* props = (*entity.m_pEntity)->m_pProperties01;

	if (props != nullptr)
	{
		for (auto prop : *props)
		{
			if (prop.m_pType == nullptr || prop.m_pType->getPropertyInfo() == nullptr) continue;
			auto propInfo = prop.m_pType->getPropertyInfo();

			std::string propName, propTypeName;
			bool validPropName = false;
			[&]()
			{
				__try
				{
					[&]()
					{
						propName = propInfo->m_pName;
						propTypeName = propInfo->m_pType->typeInfo()->m_pTypeName;
						validPropName = true;
					}();
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					propTypeName = "";
				}
			}();

			if (propName.empty() || !validPropName || propName != propNameToFind) continue;

			int32_t* propValue = reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(entity.m_pEntity) + prop.m_nOffset);
			*propValue = value;

			break;
		}
	}
}

inline void OverrideArrayPropValue(ZEntityRef entity, std::string propNameToFind, int firstIndex, int arrayLength, std::vector<std::string> values, std::unordered_map<uint64_t, ZEntityRef>* knownEnts)
{
	TArray<ZEntityProperty>* props = (*entity.m_pEntity)->m_pProperties01;

	if (props != nullptr)
	{
		for (auto prop : *props)
		{
			if (prop.m_pType == nullptr || prop.m_pType->getPropertyInfo() == nullptr) continue;
			auto propInfo = prop.m_pType->getPropertyInfo();

			std::string propName, propTypeName;
			bool validPropName = false;
			[&]()
			{
				__try
				{
					[&]()
					{
						propName = propInfo->m_pName;
						propTypeName = propInfo->m_pType->typeInfo()->m_pTypeName;
						validPropName = true;
					}();
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					propTypeName = "";
				}
			}();

			if (propName.empty() || !validPropName || propName != propNameToFind) continue;

			TArray<ZEntityRef>* propValue = reinterpret_cast<TArray<ZEntityRef>*>(reinterpret_cast<uintptr_t>(entity.m_pEntity) + prop.m_nOffset);

			TArray<ZEntityRef> newArr = TArray<ZEntityRef>();

			for (int i = 1; i <= arrayLength; i++)
			{
				uint64_t entToRef;
				std::istringstream(values[firstIndex + i].c_str()) >> entToRef;

				auto it = knownEnts->find(entToRef);
				if (it != knownEnts->end())
				{
					newArr.push_back((*knownEnts)[entToRef]);
				}
			}

			// entity.SetProperty(propNameToFind.c_str(), newArr);

			propValue->clear();

			// *propValue = *newArr;

			break;
		}
	}
}

inline void SetPropertyFromVectorString(ZEntityRef entity, std::string propName, std::string propType, std::vector<std::string> values, int firstIndex, std::unordered_map<uint64_t, ZEntityRef>* knownEnts)
{
	switch (mapOfTypes[propType])
	{
		case PropTypes::t_bool:
		{
			auto boolValue = values[firstIndex] == "true";
			entity.SetProperty(propName.c_str(), boolValue);
			break;
		}
		case PropTypes::t_enum:
		{
			int32_t enumValueAsInt = std::atoi(values[firstIndex].c_str());
			// Set property doesn't seem to work on enums. So override the pointer value instead
			OverrideEnumPropValue(entity, propName, enumValueAsInt);
			// entity.SetProperty(propName.c_str(), enumValueAsInt);
			entity.GetBaseEntity()->Deactivate(0);
			entity.GetBaseEntity()->Activate(0);
			break;
		}
		case PropTypes::t_int:
		{
			auto intOrEnumValue = std::atoi(values[firstIndex].c_str());
			entity.SetProperty(propName.c_str(), intOrEnumValue);
			break;
		}
		case PropTypes::t_float:
		{
			float32 floatValue = std::stod(values[firstIndex].c_str());
			entity.SetProperty(propName.c_str(), floatValue);
			break;
		}
		case PropTypes::t_string:
		{
			char* inputString = new char[values[firstIndex].size()];
			for (int i = 0; i < values[firstIndex].size(); i++)
			{
				inputString[i] = values[firstIndex].c_str()[i];
			}
			inputString[values[firstIndex].size()] = '\0';

			auto stringValue = ZString(inputString);
			entity.SetProperty(propName.c_str(), stringValue);
			break;
		}
		case PropTypes::t_guid:
		{
			auto guidValue = new ZRepositoryID(values[firstIndex].c_str());
			entity.SetProperty(propName.c_str(), *guidValue);
			break;
		}
		case PropTypes::t_color:
		{
			auto color = SColorRGB();

			color.r = std::stoul(values[firstIndex].substr(1, 2), nullptr, 16);
			color.g = std::stoul(values[firstIndex].substr(3, 2), nullptr, 16);
			color.b = std::stoul(values[firstIndex].substr(5, 2), nullptr, 16);

			entity.SetProperty(propName.c_str(), color);

			break;
		}
		case PropTypes::t_matrix43:
		{
			entity.SetProperty(propName.c_str(), EularToMatrix43(
				std::stod(values[firstIndex]),
				std::stod(values[firstIndex + 1]),
				std::stod(values[firstIndex + 2]),
				std::stod(values[firstIndex + 3]),
				std::stod(values[firstIndex + 4]),
				std::stod(values[firstIndex + 5])
			));

			break;
		}
		case PropTypes::t_entityRef:
		{
			uint64_t entToRef;
			std::istringstream(values[firstIndex].c_str()) >> entToRef;

			auto it = knownEnts->find(entToRef);
			if (it != knownEnts->end())
			{
				entity.SetProperty(propName.c_str(), (*knownEnts)[entToRef]);
			}
			break;
		}
		case PropTypes::t_arrayEntityRef:
		{
			int32_t arrayLength = std::atoi(values[firstIndex].c_str());

			OverrideArrayPropValue(entity, propName, firstIndex, arrayLength, values, knownEnts);

			break;
		}
	}
}
