#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include "ZBinaryReader.h"
#include "ZBinaryWriter.h"
#include <Glacier/ZResource.h>

class ZBinaryDeserializer
{
public:
	void* Deserialize(const std::string& filePath, const TArray<ZResourceIndex>* referenceIndices = nullptr);
	void* Deserialize(std::vector<char>* buffer, const TArray<ZResourceIndex>* referenceIndices = nullptr);
	void* Deserialize(ZBinaryReader& binaryReader, const TArray<ZResourceIndex>* referenceIndices = nullptr);
	const unsigned char GetAlignment() const;

private:
	void HandleRebaseSection(ZBinaryReader& binaryReader, ZBinaryReader& dataSectionbinaryReader, ZBinaryWriter& dataSectionBinaryWriter);
	void HandleTypeReindexingSection(ZBinaryReader& binaryReader, ZBinaryReader& dataSectionbinaryReader, ZBinaryWriter& dataSectionBinaryWriter);
	void HandleRuntimeResourceIDReindexingSection(ZBinaryReader& binaryReader, ZBinaryReader& dataSectionbinaryReader, ZBinaryWriter& dataSectionBinaryWriter, const TArray<ZResourceIndex>* referenceIndices = nullptr);
	static void Align(ZBinaryReader& binaryReader, const size_t currentPosition, const size_t alignment);
	STypeID* GetTypeIDFromTypeName(const std::string& typeName);

	unsigned char alignment;
};
