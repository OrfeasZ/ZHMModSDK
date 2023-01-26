#pragma once

#include "IO/ZBinaryDeserializer.h"

void* ZBinaryDeserializer::Deserialize(const std::string& filePath, const TArray<ZResourceIndex>* referenceIndices)
{
	ZBinaryReader binaryReader = ZBinaryReader(filePath);

	return Deserialize(binaryReader, referenceIndices);
}

void* ZBinaryDeserializer::Deserialize(std::vector<char>* buffer, const TArray<ZResourceIndex>* referenceIndices)
{
	ZBinaryReader binaryReader = ZBinaryReader(buffer);

	return Deserialize(binaryReader, referenceIndices);
}

void* ZBinaryDeserializer::Deserialize(ZBinaryReader& binaryReader, const TArray<ZResourceIndex>* referenceIndices)
{
	const unsigned int magic = binaryReader.Read<unsigned int>();

	if (magic != 'BIN1')
	{
		throw std::invalid_argument("File format not supported!");
	}

	const unsigned char endian = binaryReader.Read<unsigned char>();
	const unsigned char alignment = binaryReader.Read<unsigned char>();
	const unsigned char sectionsCount = binaryReader.Read<unsigned char>();

	const unsigned char unusedByte = binaryReader.Read<unsigned char>();

	const unsigned char dataLength0 = binaryReader.Read<unsigned char>();
	const unsigned char dataLength1 = binaryReader.Read<unsigned char>();
	const unsigned char dataLength2 = binaryReader.Read<unsigned char>();
	const unsigned char dataLength3 = binaryReader.Read<unsigned char>();

	const unsigned int dataLength = (dataLength0 << 24) + (dataLength1 << 16) + (dataLength2 << 8) + dataLength3;

	unsigned int unusedDWORD = binaryReader.Read<unsigned int>();

	void* data = operator new(dataLength, std::align_val_t(alignment));

	binaryReader.ReadBytes(data, dataLength);

	ZBinaryReader dataSectionBinaryReader = ZBinaryReader(data, dataLength);
	ZBinaryWriter dataSectionBinaryWriter = ZBinaryWriter(data, dataLength);

	for (unsigned char i = 0; i < sectionsCount; ++i)
	{
		const unsigned int sectionType = binaryReader.Read<unsigned int>();
		const unsigned int sectionSize = binaryReader.Read<unsigned int>();

		switch (sectionType)
		{
			case 0x12EBA5ED:
				HandleRebaseSection(binaryReader, dataSectionBinaryReader, dataSectionBinaryWriter);
				break;
			case 0x3989BF9F:
				HandleTypeReindexingSection(binaryReader, dataSectionBinaryReader, dataSectionBinaryWriter);
				break;
			case 0x578FBCEE:
				HandleRuntimeResourceIDReindexingSection(binaryReader, dataSectionBinaryReader, dataSectionBinaryWriter, referenceIndices);
				break;
			default:
			{
				std::stringstream stream;

				stream << std::hex << sectionType;

				throw std::invalid_argument(std::format("Unknown section type: 0x{}!", stream.str()));
			}
		}
	}

	this->alignment = alignment;

	return data;
}

const unsigned char ZBinaryDeserializer::GetAlignment() const
{
	return alignment;
}

void ZBinaryDeserializer::HandleRebaseSection(ZBinaryReader& binaryReader, ZBinaryReader& dataSectionbinaryReader, ZBinaryWriter& dataSectionBinaryWriter)
{
	const unsigned int numberOfRebaseLocations = binaryReader.Read<unsigned int>();

	for (unsigned int i = 0; i < numberOfRebaseLocations; ++i)
	{
		const unsigned int rebaseLocationOffset = binaryReader.Read<unsigned int>();

		dataSectionbinaryReader.Seek(rebaseLocationOffset);
		dataSectionBinaryWriter.Seek(rebaseLocationOffset);

		const long long value = dataSectionbinaryReader.Read<long long>();

		if (value != -1)
		{
			dataSectionBinaryWriter.Write<unsigned long long>(reinterpret_cast<uintptr_t>(dataSectionbinaryReader.GetData()) + value);
		}
		else
		{
			dataSectionBinaryWriter.Write<unsigned long long>(0);
		}
	}
}

void ZBinaryDeserializer::HandleTypeReindexingSection(ZBinaryReader& binaryReader, ZBinaryReader& dataSectionbinaryReader, ZBinaryWriter& dataSectionBinaryWriter)
{
	size_t sectionStartPosition = binaryReader.GetPosition();
	unsigned int numberOfOffsetsToReindex = binaryReader.Read<unsigned int>();
	std::unordered_map<unsigned int, size_t> typeIDsToReindex;

	for (unsigned int i = 0; i < numberOfOffsetsToReindex; ++i)
	{
		const unsigned int typeIDOffset = binaryReader.Read<unsigned int>();

		dataSectionbinaryReader.Seek(typeIDOffset);

		const size_t typeIDIndex = dataSectionbinaryReader.Read<unsigned long long>();

		typeIDsToReindex.insert(std::make_pair(typeIDOffset, typeIDIndex));
	}

	const unsigned int numberOfTypeNames = binaryReader.Read<unsigned int>();
	std::vector<STypeID*> typeIDs = std::vector<STypeID*>(numberOfTypeNames);

	for (unsigned int i = 0; i < numberOfTypeNames; ++i)
	{
		size_t currentPosition = binaryReader.GetPosition() - sectionStartPosition;

		Align(binaryReader, currentPosition, 4);

		const unsigned int typeID = binaryReader.Read<unsigned int>();
		const int typeSize = binaryReader.Read<int>();
		const unsigned int typeNameLength = binaryReader.Read<unsigned int>();
		std::string typeName = binaryReader.ReadString(typeNameLength - 1);

		STypeID* type = GetTypeIDFromTypeName(typeName);

		if (!type)
		{
			throw std::invalid_argument(std::format("Type info for {} isn't available!", typeName));
		}

		typeIDs[typeID] = type;
	}

	for (auto it = typeIDsToReindex.begin(); it != typeIDsToReindex.end(); it++)
	{
		const unsigned int typeIDOffset = it->first;
		const size_t typeIDIndex = it->second;

		dataSectionBinaryWriter.Seek(typeIDOffset);
		dataSectionBinaryWriter.Write<unsigned long long>(reinterpret_cast<uintptr_t>(typeIDs[typeIDIndex]));
	}
}

void ZBinaryDeserializer::HandleRuntimeResourceIDReindexingSection(ZBinaryReader& binaryReader, ZBinaryReader& dataSectionbinaryReader, ZBinaryWriter& dataSectionBinaryWriter, const TArray<ZResourceIndex>* referenceIndices)
{
	if (!referenceIndices)
	{
		return;
	}

	const unsigned int numberOfOffsetsToReindex = binaryReader.Read<unsigned int>();

	for (unsigned int i = 0; i < numberOfOffsetsToReindex; ++i)
	{
		const unsigned int runtimeResourceIDOffset = binaryReader.Read<unsigned int>();

		dataSectionbinaryReader.Seek(runtimeResourceIDOffset);

		const unsigned int idHigh = dataSectionbinaryReader.Read<unsigned int>();
		const unsigned int idLow = dataSectionbinaryReader.Read<unsigned int>(); //Index of resource reference

		if (idLow != UINT32_MAX)
		{
			ZResourceIndex referenceIndex = (*referenceIndices)[idLow];
			ZResourceContainer::SResourceInfo resourceInfo = (*Globals::ResourceContainer)->m_resources[referenceIndex.val];

			dataSectionBinaryWriter.Seek(runtimeResourceIDOffset);
			dataSectionBinaryWriter.Write(resourceInfo.rid);
		}
	}
}

void ZBinaryDeserializer::Align(ZBinaryReader& binaryReader, const size_t currentPosition, const size_t alignment)
{
	size_t misalign = currentPosition % alignment;

	if (misalign != 0)
	{
		binaryReader.Seek(alignment - misalign, ZBinaryReader::ESeekOrigin::current);
	}
}

STypeID* ZBinaryDeserializer::GetTypeIDFromTypeName(const std::string& typeName)
{
	return (*Globals::TypeRegistry)->m_types.find(typeName.c_str())->second;
}
