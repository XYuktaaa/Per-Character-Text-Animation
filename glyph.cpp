#include <pango/pangocairo.h>
#include <cairo.h>
#include <cmath>
#include <vector>
#include <string>
#include <cstdio>

struct GlyphInfo {
    int  char_index;
    double x,y;
    double width, height;
    PangoGlyphString* glyphs;
    PangoItem* item;
    
};

int main(){
    const char* text = "HELLO";
    const int W = 600, H= 200;

    cairo_surface_t* surface= cairo_image_surface_create(CAIRO_FORMAT_ARGB32 , W, H);
    cairo_t* cr = cairo_create(surface);

    cairo_set_source_rgb(cr,1,1,1);
    cairo_paint(cr);

    PangoLayout* layout =pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text , -1);

    PangoFontDescription* font_desc = pango_font_description_from_string("Sans Bold 48");
    pango_layout_set_font_description(layout, font_desc);
    pango_font_description_free(font_desc);

      std::vector<GlyphInfo> glyphs;
    PangoLayoutIter* iter = pango_layout_get_iter(layout);

    do {
        PangoRectangle logical_rect;
        // Gets extents in Pango units (divide by PANGO_SCALE for pixels)
        pango_layout_iter_get_char_extents(iter, &logical_rect);

        PangoLayoutRun* run = pango_layout_iter_get_run_readonly(iter);
        if (!run) continue;  // end-of-line sentinel

        GlyphInfo gi;
        gi.char_index = pango_layout_iter_get_index(iter);
        gi.x      = (double)logical_rect.x      / PANGO_SCALE;
        gi.y      = (double)logical_rect.y      / PANGO_SCALE;
        gi.width  = (double)logical_rect.width  / PANGO_SCALE;
        gi.height = (double)logical_rect.height / PANGO_SCALE;
        gi.glyphs = run->glyphs;
        gi.item   = run->item;
        glyphs.push_back(gi);

    } while (pango_layout_iter_next_cluster(iter));

    pango_layout_iter_free(iter);

    // --- 4. Render each glyph independently with a transform ---
    double baseline = 100.0; // vertical anchor in the PNG

    for (int i = 0; i < (int)glyphs.size(); i++) {
        const GlyphInfo& g = glyphs[i];

        // Per-character animation values (in a real GSoC impl, these come from ValueNodes)
        double y_offset  = std::sin(i * 0.8) * 20.0;   // bounce
        double rotation  = (i - 2) * 0.12;              // subtle tilt per char
        double cx = g.x + g.width  / 2.0 + 40.0;       // glyph center x (+ margin)
        double cy = baseline + g.y + g.height / 2.0;   // glyph center y

        cairo_save(cr);

        // Move to glyph center, apply rotation, draw
        cairo_translate(cr, cx, cy + y_offset);
        cairo_rotate(cr, rotation);
        cairo_translate(cr, -g.width / 2.0, -g.height / 2.0);

        // Color varies per character (hue cycling)
        double hue = (double)i / glyphs.size();
        cairo_set_source_rgb(cr,
            0.5 + 0.5 * std::cos(hue * 6.28),
            0.5 + 0.5 * std::cos(hue * 6.28 + 2.09),
            0.5 + 0.5 * std::cos(hue * 6.28 + 4.19));

        // Position pango at the glyph's origin within our transformed space
        cairo_move_to(cr, 0, 0);

        // Use pango_cairo_show_glyph_string to render just this glyph
        pango_cairo_show_glyph_string(cr, g.item->analysis.font, g.glyphs);

        cairo_restore(cr);
    }

    // --- 5. Save output ---
    cairo_surface_write_to_png(surface, "output.png");
    printf("Written output.png\n");

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return 0;
}

