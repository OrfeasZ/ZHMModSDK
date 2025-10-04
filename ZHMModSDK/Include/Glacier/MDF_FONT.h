#pragma once

namespace MDF_FONT {
    struct SFontHeader {
        unsigned int m_anTexRes[2];
        unsigned int m_anLowRes[2];
        unsigned int m_anHighRes[2];
        unsigned int m_nSpreadFactor;
        unsigned int m_nNumGlyphs;
        bool m_bASCII;
    };

    struct SGlyphData {
        float m_afTexU[2];
        float m_afTexV[2];
        float m_afSize[2];
        float m_afBearing[2];
        float m_afOffsets[2];
        float m_fAdvance;
        unsigned int m_nCharcode;
    };

    struct STextBoundingBox {
        float m_fMinX;
        float m_fMaxX;
        float m_fMinY;
        float m_fMaxY;
    };

    const SGlyphData* GetGlyphData(const unsigned int nCharcode);
    void CalcBoundingBox(STextBoundingBox& TextBoundingBox, const char* pszString);
    void RenderQuad(
        unsigned int nCharcode, const float fScale, float& fPenX, const float fPenY, float* pVertices, float* pTexCoords
    );

    float ComputeLineHeightFromMetrics();
    float GetAdvanceWidth(const unsigned int charcode);

    extern SFontHeader g_FontHeader;
    extern SGlyphData g_GlyphData[95];
    extern unsigned char g_DistanceField[393216];
}