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
 
    PangoLayoutIter* iter = pango_layout_get_iter(layout);
 
    do {
        PangoLayoutRun* run = pango_layout_iter_get_run_readonly(iter);
        if (!run) continue;
 
        PangoRectangle logical_rect;
        pango_layout_iter_get_char_extents(iter, &logical_rect);
        if (logical_rect.width <= 0) continue;
 
        // Find which glyph(s) in the run belong to this cluster
        int cluster_byte_index = pango_layout_iter_get_index(iter);
 
        // Collect all glyph indices in this run that belong to this cluster
        PangoGlyphString* run_glyphs = run->glyphs;
        int start_glyph = -1, end_glyph = -1;
        for (int gi_idx = 0; gi_idx < run_glyphs->num_glyphs; gi_idx++) {
            if (run_glyphs->log_clusters[gi_idx] == 
                (cluster_byte_index - run->item->offset)) {
                if (start_glyph == -1) start_glyph = gi_idx;
                end_glyph = gi_idx + 1;
            }
        }
 
        if (start_glyph == -1) {
            // Fallback: no matching cluster found, skip
            continue;
        }
 
        // Build a new PangoGlyphString containing only this cluster's glyphs
        int n = end_glyph - start_glyph;
        PangoGlyphString* cluster_gs = pango_glyph_string_new();
        pango_glyph_string_set_size(cluster_gs, n);
        for (int k = 0; k < n; k++) {
            cluster_gs->glyphs[k]      = run_glyphs->glyphs[start_glyph + k];
            cluster_gs->log_clusters[k] = 0;
        }
 
        GlyphInfo gi;
        gi.cluster_index   = cluster_byte_index;
        gi.natural_x       = (double)logical_rect.x / PANGO_SCALE;
        gi.natural_y       = 0.0;
        gi.width           = (double)logical_rect.width  / PANGO_SCALE;
        gi.height          = (double)logical_rect.height / PANGO_SCALE;
        gi.baseline_offset = (double)pango_layout_iter_get_baseline(iter) / PANGO_SCALE
                           - (double)logical_rect.y / PANGO_SCALE;
        gi.baseline        = (double)pango_layout_iter_get_baseline(iter) / PANGO_SCALE;
        gi.glyph_string    = cluster_gs;          // single-cluster glyph string
        gi.item            = pango_item_copy(run->item);
 
        double hue = (double)result.size() / std::max(1, (int)text.size());
        hue_to_rgb(hue, gi.params.r, gi.params.g, gi.params.b);
        result.push_back(gi);
 
    } while (pango_layout_iter_next_cluster(iter));
 
    printf("extract_glyphs: input='%s' → %zu glyphs extracted\n",
           text.c_str(), result.size());
 
    pango_layout_iter_free(iter);
    g_object_unref(layout);
 
    if (!result.empty()) {
        double first_baseline = result[0].baseline;
        for (auto& g : result)
            g.natural_y = g.baseline - first_baseline;
    }
    return result;
}
