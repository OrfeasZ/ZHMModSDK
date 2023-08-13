#pragma once

/*
struct SPrimHeader
{
    enum EPrimType
    {
        PTOBJECTHEADER = 0x1,
        PTMESH = 0x2,
        PTSHAPE = 0x5,
    };

    uint8 lPad;
    uint8 lPad1;
    uint16 lType;
};


struct SPrims : SPrimHeader
{
};

struct SPrimObjectHeader : SPrims
{
    enum PROPERTY_FLAGS
    {
        HAS_BONES = 0x1,
        HAS_FRAMES = 0x2,
        IS_LINKED_OBJECT = 0x4,
        IS_WEIGHTED_OBJECT = 0x8,
        USE_BOUNDS = 0x100,
        HAS_HIRES_POSITIONS = 0x200,
    };

    uint32 lPropertyFlags;
    uint32 lBoneRigResourceIndex;
    uint32 lNumObjects;
    uint32 lObjectTable;
    float vMin[3];
    float vMax[3];
};

struct SPrimObject : SPrims
{
    enum SUBTYPE
    {
        SUBTYPE_STANDARD = 0x0,
        SUBTYPE_LINKED = 0x1,
        SUBTYPE_WEIGHTED = 0x2,
        SUBTYPE_STANDARDUV2 = 0x3,
        SUBTYPE_STANDARDUV3 = 0x4,
        SUBTYPE_STANDARDUV4 = 0x5,
        SUBTYPE_SPEEDTREE = 0x6,
    };

    enum PROPERTY_FLAGS
    {
        PROPERTY_XAXISLOCKED = 0x1,
        PROPERTY_YAXISLOCKED = 0x2,
        PROPERTY_ZAXISLOCKED = 0x4,
        PROPERTY_HIRES_POSITIONS = 0x8,
        PROPERTY_UNUSED = 0x10,
        PROPERTY_CONSTANT_VCOLOR = 0x20,
        PROPERTY_ISNOPHYSICSPROP = 0x40,
    };

    uint8 lSubType;
    uint8 lProperties;
    uint8 lLODMask;
    uint8 lUnused;
    uint8 nZBias;
    uint8 nZOffset;
    uint16 lMaterialId;
    uint32 lWireColor;
    uint32 lConstantVColor;
    float vMin[3];
    float vMax[3];
};

struct SPrimSubMesh : SPrimObject
{
    uint32 lNumVertices;
    uint32 lVertices;
    uint32 lNumIndices;
    uint32 lNumCrackIndices;
    uint32 lIndices;
    uint32 lCollision;
    uint32 lClothData;
    uint8 lNumUVChannels;
    uint8 lPad[3];
};

struct SColiBoxHeader
{
    uint16 nNumChunks;
    uint16 nTriPerChunk;
};

struct SColiBox
{
    uint8 min[3];
    uint8 max[3];
};

struct SPrimMesh : SPrimObject
{
    uint32 lSubMeshTable;
    float vPosScale[4];
    float vPosBias[4];
    float vTexScaleBias[4];
    uint8 lClothId;
    uint8 lPad[3];
};

struct __cppobj SPrimMeshWeighted : SPrimMesh
{
    uint32 lNumCopyBones;
    uint32 lCopyBones;
    uint32 lBoneIndices;
    uint32 lBoneInfo;
};

struct SColiBoneHeader
{
    uint16 nTotalSize;
    uint16 nNumBlocks;
    uint16 nTotalChunksAlign;
};

struct SBoneAccel
{
    uint32 nOffset;
    uint32 nNumIndices;
};

struct SBoneInfo
{
    uint16 nTotalSize;
    uint16 nNumAccelEntries;
    uint8 aBoneRemap[255];
    uint8 nPad;
};

struct SVertexSimple
{
    SVector4 vPosition;
    uint32 nColor;
    SVector2 vTexCoord;
};

struct SVertexSimpleFloat
{
    SVector4 vPosition;
    SVector4 vColor;
    SVector2 vTexCoord;
};


struct SRenderSamplerStateDesc
{
    ERenderFilterMode eFilterMode;
    ERenderAddressMode eAddressModeU;
    ERenderAddressMode eAddressModeV;
    ERenderAddressMode eAddressModeW;
    ERenderCompareFunc eComparisonFunc;
    uint32 nMaxAnisotropy;
    float32 BorderColor[4];
    float32 fMinLod;
    float32 fMaxLod;
    float32 fLodBias;
    uint32 nAnisotropicBias;
};
*/
