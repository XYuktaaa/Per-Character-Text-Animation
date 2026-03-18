#pragma once
#include <vector>
#include <cairo.h>
#include "glyph_layer.h"

enum EffectMode {
    MODE_WAVE        = 0,
    MODE_SCALE_POPIN = 1,
    MODE_COLOR       = 2,
    MODE_ALL         = 3
};
void render_frame(cairo_t* cr,
                  std::vector<GlyphInfo>& glyphs,
                  EffectMode mode,
                  double t,
                  double stagger,
                  double speed,
                  int canvas_w,
                  int canvas_h,
                  int selected_glyph = -1,
                  double amplitude = 28.0,
                  double global_rotation =0.0,
                  double text_offset_x= 0.0,
                  double text_offset_y= 0.0,
                  double char_spacing = 0.0);

