#pragma once
#include <pango/pangocairo.h>
#include <string>
#include <vector>

struct GlyphParams{
    double origin_x = 0.0;
    double origin_y = 0.0;
    double rotation = 1.0;
    double scale    = 1.0; 
    double opacity  = 1.0;
    double r = 1, g = 1, b = 1;
    
};

struct GlyphInfo {
    int cluster_index;
    double natural_x;
    double natural_y;
    double width, height;
    PangoGlyphString* glyph_string;
    PangoItem* item;
    GlyphParams  params;
};

std::vector<GlyphInfo> extract_glyphs(const std::string& text, const std::string& font_desc_str, cairo_t* cr);


