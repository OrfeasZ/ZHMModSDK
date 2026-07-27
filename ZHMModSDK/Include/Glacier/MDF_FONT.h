#pragma once

#include <cstdint>

/*
 * Font metrics and glyph data originate from **Roboto Regular** (Google Fonts).
 *
 * MSDF/SDF atlas and glyph metadata were generated using:
 *     msdf-atlas-gen — https://github.com/Chlumsky/msdf-atlas-gen
 *
 * Command used to produce the raw generator output:
 *
 *     msdf-atlas-gen.exe ^
 *         -font Roboto-Regular.ttf ^
 *         -type sdf ^
 *         -dimensions 1024 512 ^
 *         -charset charset.txt ^
 *         -json glyphs_raw.json ^
 *         -imageout atlas.png
 *
 * The final g_GlyphData and g_DistanceField arrays were generated using
 * the Python conversion script located at:
 *
 *     Tools/MDF_Font/generate_mdf_font.py
 *
 * The script converts msdf-atlas-gen output (atlas.png + glyphs_raw.json)
 * into the internal MDF_FONT C++ data format.
 *
 * Unicode ranges included (as defined in charset.txt):
 *     Basic Latin (U+0020–U+007E)
 *     Latin-1 Supplement (U+00A0–U+00FF)
 */
namespace MDF_FONT {
    struct SFontHeader {
        uint32_t m_anTexRes[2];
        uint32_t m_anLowRes[2];
        uint32_t m_anHighRes[2];
        uint32_t m_nSpreadFactor;
        uint32_t m_nNumGlyphs;
        bool m_bASCII;
    };

    struct SGlyphData {
        float m_TexU0, m_TexV0;
        float m_TexU1, m_TexV1;

        float m_PlaneLeft, m_PlaneBottom;
        float m_PlaneRight, m_PlaneTop;

        float m_Advance;
        uint32_t m_Charcode;
    };

    struct STextBoundingBox {
        float m_fMinX;
        float m_fMaxX;
        float m_fMinY;
        float m_fMaxY;
    };

    void Initialize();

    const SGlyphData* GetGlyphData(const uint32_t p_Charcode);
    bool HasGlyph(const uint32_t c);
    uint32_t DecodeUTF8(const char*& p);

    void CalcBoundingBox(STextBoundingBox& p_TextBoundingBox, const char* p_Text);
    void RenderQuad(
        uint32_t p_Charcode,
        const float p_Scale,
        float& p_PenX,
        const float p_PenY,
        float* pVertices,
        float* pTexCoords
    );

    float GetAdvanceWidth(const uint32_t charcode);

    extern SFontHeader g_FontHeader;
    extern SGlyphData g_GlyphData[191];
    extern int32_t g_GlyphLookup[256];
    extern uint8_t g_DistanceField[1024 * 512];
    extern const float g_LineHeight;
}