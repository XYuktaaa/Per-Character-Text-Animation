#include "renderer.h"
#include <cmath>


void render_frame(cairo_t* cr,
                  std::vector<GlyphInfo>& glyphs,
                  EffectMode mode,
                  double t,          // time in seconds
                  double stagger,    // seconds between each char
                  double speed,
                  int canvas_w, 
                  int canvas_h,
                  int selected_glyph,
                  double amplitude,
                  double global_rotation,
                  double text_offset_x,
                  double text_offset_y,
                  double char_spacing)
{
    // Clear background
    cairo_set_source_rgb(cr, 0.10, 0.10, 0.18);
    cairo_paint(cr);

    double baseline_y = canvas_h * 0.62;  // vertical anchor
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.4, 0.6, 1.0, 0.25);
    cairo_set_line_width(cr, 0.5);
    double dashes[] = {6.0, 4.0};
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_move_to(cr, 20, baseline_y + text_offset_y);
    cairo_line_to(cr, canvas_w - 20, baseline_y + text_offset_y);
    cairo_stroke(cr);
    cairo_set_dash(cr, nullptr, 0, 0);  // reset dash
    cairo_set_source_rgba(cr, 0.4, 0.6, 1.0, 0.4);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, 24, baseline_y + text_offset_y - 4);
    cairo_show_text(cr, "baseline");
    cairo_restore(cr);

	double total_width = 0.0;
	for (auto& gi : glyphs) total_width += gi.width;
	double start_x = (canvas_w - total_width) / 2.0;

    for (int i = 0; i < (int)glyphs.size(); i++) {
        GlyphInfo& g = glyphs[i];
        double phase = t * speed - i * stagger;  // staggered time per char
        double dy     = 0.0;
        double dscale = 1.0;
        double opacity = 1.0;

        if (mode == MODE_WAVE) {
            dy = -std::sin(phase * 2.5) * amplitude;     // bounce amplitude 28px
            dscale = 1.0 + std::sin(phase * 2.5) * 0.06;
        }
        else if (mode == MODE_SCALE_POPIN) {
            // Each char pops in sequentially, then settles
            double local_t = fmod(phase, 2.5);
            if (local_t < 0) local_t += 2.5;
            if (local_t < 0.3)
                dscale = local_t / 0.3 * 1.4;       // overshoot to 1.4×
            else if (local_t < 0.5)
                dscale = 1.4 - (local_t - 0.3) / 0.2 * 0.4;  // settle to 1.0×
            else
                dscale = 1.0;
            opacity = (local_t < 0.1) ? local_t / 0.1 : 1.0;
        }
        else if (mode == MODE_COLOR) {
            // Hue continuously rotates per character
            double hue = fmod(t * speed * 0.3 + (double)i / glyphs.size(), 1.0);
            double r, gg, b;
            
            double h6 = hue * 6.0;
            int seg = (int)h6 % 6;
            double f = h6 - (int)h6;
            switch(seg) {
                case 0: r=1;    gg=f;   b=0;   break;
                case 1: r=1-f;  gg=1;   b=0;   break;
                case 2: r=0;    gg=1;   b=f;   break;
                case 3: r=0;    gg=1-f; b=1;   break;
                case 4: r=f;    gg=0;   b=1;   break;
                default:r=1;    gg=0;   b=1-f; break;
            }
            g.params.r = r; g.params.g = gg; g.params.b = b;
        }
        else if (mode == MODE_ALL) {
            // All three simultaneously
            dy     = -std::sin(phase * 2.5) * 22.0;
            dscale = 1.0 + std::sin(phase * 2.5) * 0.1;
            double hue = fmod(t * speed * 0.25 + (double)i / glyphs.size(), 1.0);
            double h6 = hue * 6.0;
            int seg = (int)h6 % 6; double f = h6-(int)h6;
            switch(seg){
                case 0:g.params.r=1;g.params.g=f;g.params.b=0;break;
                case 1:g.params.r=1-f;g.params.g=1;g.params.b=0;break;
                case 2:g.params.r=0;g.params.g=1;g.params.b=f;break;
                case 3:g.params.r=0;g.params.g=1-f;g.params.b=1;break;
                case 4:g.params.r=f;g.params.g=0;g.params.b=1;break;
                default:g.params.r=1;g.params.g=0;g.params.b=1-f;break;
            }
        }
        double cx = start_x + g.natural_x + g.width / 2.0 + text_offset_x + (i * char_spacing);
		double cy = baseline_y + g.natural_y + g.height / 2.0 + text_offset_y;
      
        double final_scale   = g.params.scale * dscale;
        double final_y       = cy + g.params.origin_y + dy;
        double final_opacity = g.params.opacity * opacity;

        cairo_save(cr);
        cairo_translate(cr, cx + g.params.origin_x, final_y);
        // cairo_rotate(cr, g.params.rotation);
        cairo_rotate(cr, g.params.rotation + global_rotation);
        cairo_scale(cr, final_scale, final_scale);
        cairo_translate(cr, -g.width / 2.0, -g.height / 2.0);
        cairo_set_source_rgba(cr, g.params.r, g.params.g, g.params.b, final_opacity);
        cairo_move_to(cr, 0, 0);
        pango_cairo_show_glyph_string(cr, g.item->analysis.font, g.glyph_string);
        cairo_restore(cr);
       // Draw selection highlight box
if (selected_glyph == i || selected_glyph == -1 && false) {
    cairo_save(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.25);
    cairo_set_line_width(cr, 1.0);
    double bx = cx + g.params.origin_x - g.width  / 2.0 * final_scale;
    double by = final_y               - g.height / 2.0 * final_scale;
    double bw = g.width  * final_scale;
    double bh = g.height * final_scale;
    cairo_rectangle(cr, bx, by, bw, bh);
    cairo_stroke(cr);
    cairo_restore(cr);
} 
    }
}
