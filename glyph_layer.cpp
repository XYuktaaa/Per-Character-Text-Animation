#include <cstring>
#include "glyph_layer.h"
#include <cmath>

static void hue_to_rgb (double h, double& r, double& g, double& b){
	h = fmod(h, 1.0) * 6.0;
	int i = (int)h;
	double f = h - i;
	switch( i % 6 ){
		case 0:r=1;    g=f;    b=0;   break;
        case 1: r=1-f;  g=1;    b=0;   break;
        case 2: r=0;    g=1;    b=f;   break;
        case 3: r=0;    g=1-f;  b=1;   break;
        case 4: r=f;    g=0;    b=1;   break;
        case 5: r=1;    g=0;    b=1-f; break;
    }
}

std::vector<GlyphInfo> extract_glyphs(const std::string& text,
                                       const std::string& font_desc_str,
                                       cairo_t* cr)
{
    std::vector<GlyphInfo> result;

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text.c_str(), -1);

    PangoFontDescription* fd = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, fd);
    pango_font_description_free(fd);

    
    PangoRectangle layout_rect;
    pango_layout_get_extents(layout, nullptr, &layout_rect);

    PangoLayoutIter* iter = pango_layout_get_iter(layout);
    int char_index = 0;

    do {
        PangoLayoutRun* run = pango_layout_iter_get_run_readonly(iter);
        if (!run) { char_index++; continue; }  // newline sentinel

        PangoRectangle logical_rect;
        pango_layout_iter_get_char_extents(iter, &logical_rect);

        
        int baseline = pango_layout_iter_get_baseline(iter);

        GlyphInfo gi;
        gi.cluster_index  = pango_layout_iter_get_index(iter);
        gi.natural_x      = (double)logical_rect.x      / PANGO_SCALE;
        gi.natural_y      = (double)baseline             / PANGO_SCALE; // use baseline, not rect.y
        gi.width          = (double)logical_rect.width   / PANGO_SCALE;
        gi.height         = (double)logical_rect.height  / PANGO_SCALE;
        gi.glyph_string   = pango_glyph_string_copy(run->glyphs);
        gi.item           = pango_item_copy(run->item);

        double hue = (double)char_index / std::max(1, (int)text.size());
        hue_to_rgb(hue, gi.params.r, gi.params.g, gi.params.b);

        result.push_back(gi);
        char_index++;

    } while (pango_layout_iter_next_cluster(iter));

    pango_layout_iter_free(iter);
    g_object_unref(layout);
    return result;
}
