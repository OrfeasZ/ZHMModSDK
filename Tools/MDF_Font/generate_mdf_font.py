import json
from PIL import Image
import numpy as np

ATLAS_FILE = "atlas.png"
GLYPHS_FILE = "glyphs_raw.json"
OUTPUT_CPP = "mdf_font_data.cpp"

ATLAS_WIDTH = 1024
ATLAS_HEIGHT = 512

print("Loading atlas...")

img = Image.open(ATLAS_FILE).convert("L")
pixels = np.array(img, dtype=np.uint8)

print("Loading glyph metadata...")

with open(GLYPHS_FILE, "r") as f:
    data = json.load(f)

glyphs = data["glyphs"]
metrics = data["metrics"]

cpp_entries = []
glyph_count = 0

for g in glyphs:
    code = g["unicode"]
    advance = float(g["advance"])

    texU0 = texV0 = texU1 = texV1 = 0.0
    planeLeft = planeBottom = planeRight = planeTop = 0.0

    if "planeBounds" in g:
        pb = g["planeBounds"]
        ab = g["atlasBounds"]
        
        # Convert atlas pixel bounds to normalized UV coordinates.
        # msdf-atlas-gen uses a top-left origin, but our renderer expects bottom-left,
        # so V coordinates must be flipped (1.0 - y/height).
        texU0 = ab["left"] / ATLAS_WIDTH
        texU1 = ab["right"] / ATLAS_WIDTH
        texV0 = 1.0 - (ab["top"] / ATLAS_HEIGHT)
        texV1 = 1.0 - (ab["bottom"] / ATLAS_HEIGHT)

        planeLeft   = pb["left"]
        planeRight  = pb["right"]
        planeBottom = pb["bottom"]
        planeTop    = pb["top"]

    cpp_entries.append(f"""    {{
        {texU0}f, {texV0}f,
        {texU1}f, {texV1}f,
        {planeLeft}f, {planeBottom}f, {planeRight}f, {planeTop}f,
        {advance}f,
        {code}
    }}""")

    glyph_count += 1

print(f"Processed {glyph_count} glyphs.")

flattened = pixels.flatten()
array_values = ", ".join(str(int(v)) for v in flattened)

cpp_output = f"""
namespace MDF_FONT {{

const float g_fLineHeight = {metrics["lineHeight"]}f;

SGlyphData g_GlyphData[{glyph_count}] =
{{
{",\n".join(cpp_entries)}
}};

unsigned char g_DistanceField[{ATLAS_WIDTH * ATLAS_HEIGHT}] =
{{
    {array_values}
}};

}}
"""

with open(OUTPUT_CPP, "w") as f:
    f.write(cpp_output)

print("Created mdf_font_data.cpp successfully!")
