#include <gtk/gtk.h>
#include "glyph_layer.h"
#include "renderer.h"
#include <chrono>
#include <string>
#include <vector>
#include <cstdio>

// Forward declarations
static void on_draw(GtkDrawingArea*, cairo_t*, int, int, gpointer);
static gboolean on_tick(GtkWidget*, GdkFrameClock*, gpointer);

static std::string g_current_text = "SYNFIG\nSTUDIO";
static std::vector<GlyphInfo> g_glyphs;
static EffectMode  g_mode    = MODE_WAVE;
static double      g_stagger = 0.08;
static double      g_speed   = 1.1;
static GtkWidget*  g_canvas  = nullptr;
static auto        g_start   = std::chrono::steady_clock::now();
static int g_selected_glyph  = -1; 
static int g_frame_count     = 0;
static std::chrono::steady_clock::time_point g_fps_time = std::chrono::steady_clock::now();
static GtkWidget* g_fps_label= nullptr;
static double g_amplitude = 28.0;
static double g_fontsize  = 72.0;
static double      g_fps          = 0.0;
static double g_rotation = 0.0; 
static double g_text_offset_x = 0.0;
static double g_text_offset_y = 0.0;
static double g_drag_start_x  = 0.0;
static double g_drag_start_y  = 0.0;
static double g_char_spacing = 0.0;
static double g_global_opacity = 1.0;
static bool g_paused = false;
static double g_paused_t = 0.0;


static void on_rotation(GtkRange* r, gpointer) {
    g_rotation = gtk_range_get_value(r) * M_PI / 180.0;  // degrees → radians
}
static void on_amplitude(GtkRange* r, gpointer) {
    g_amplitude = gtk_range_get_value(r);
}

static void on_fontsize(GtkRange* r, gpointer) {
    g_fontsize = gtk_range_get_value(r);

    cairo_surface_t* tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* tmp_cr = cairo_create(tmp);
    std::string font_str = "Sans Bold " + std::to_string((int)g_fontsize);
    g_glyphs = extract_glyphs(g_current_text, font_str, tmp_cr);
    cairo_destroy(tmp_cr); cairo_surface_destroy(tmp);
}

static gboolean on_tick(GtkWidget* widget, GdkFrameClock*, gpointer) {
    gtk_widget_queue_draw(widget);
    return G_SOURCE_CONTINUE;
}

static void on_drag_begin(GtkGestureDrag*, double x, double y, gpointer) {
    // store where drag started relative to current text position
    g_drag_start_x =  g_text_offset_x;
    g_drag_start_y =  g_text_offset_y;
}

static void on_drag_update(GtkGestureDrag*, double offset_x, double offset_y, gpointer) {

    g_text_offset_x = g_drag_start_x + offset_x;
    g_text_offset_y = g_drag_start_y + offset_y;
}

static void on_pause_toggle(GtkToggleButton* btn, gpointer) {
    g_paused = gtk_toggle_button_get_active(btn);
    if (g_paused) {
        // snapshot current time when pausing
        auto now = std::chrono::steady_clock::now();
        g_paused_t = std::chrono::duration<double>(now - g_start).count();
    }
}

static void apply_dark_theme(GtkWidget* window){
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(
        css,"window, .view{"
        " background-color: #1e1e2e;"
        " color: #cdd6f4;"
        "}"
        "entry{"
        " background-color: #12121e;"
        "  color: #a78bfa;"
        "  border: 1px solid #3a3a5e;"
        "  border-radius: 4px;"
        "  font-size: 15px;"
        "}"
        "scale trough {"
        "  background-color: #313244;"
        "  min-height: 3px;"
        "}"
        "scale highlight {"
        "  background-color: #a78bfa;"
        "}"
        "scale slider {"
        "  background-color: #cdd6f4;"
        "  min-width: 10px; min-height: 10px;"
        "  border-radius: 50%;"
        "}"
        "label { color: #888fa8; }"
        "button {"
        "  background-color: #313244;"
        "  color: #888fa8;"
        "  border: 1px solid #444;"
        "  border-radius: 3px;"
        "}"
        "button:checked {"
        "  background-color: #1e1a2e;"
        "  color: #a78bfa;"
        "  border-color: #a78bfa;"
        "}"
        "separator { background-color: #313244; }"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
   


static void on_draw(GtkDrawingArea*, cairo_t* cr,
                    int w, int h, gpointer)
{
    auto now = std::chrono::steady_clock::now();
	double t = g_paused ? g_paused_t 
                        : std::chrono::duration<double>(now - g_start).count();
    
	render_frame(cr, g_glyphs, g_mode, t, g_stagger, g_speed, w, h, g_selected_glyph,g_amplitude,  g_rotation,g_text_offset_x,g_text_offset_y,g_char_spacing);
	
    // FPS counter in top-right corner
    g_frame_count++;
    double elapsed = std::chrono::duration<double>(now - g_fps_time).count();
    if (elapsed >= 0.5) {
        g_fps = g_frame_count / elapsed;
        g_frame_count = 0;
        g_fps_time = now;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.0f fps", g_fps);
        if (g_fps_label) gtk_label_set_text(GTK_LABEL(g_fps_label), buf);
    }
}

static void on_mode_changed(GtkToggleButton* btn, gpointer data) {
    if (gtk_toggle_button_get_active(btn))
        g_mode = (EffectMode)(gintptr)data;
}

static void on_stagger(GtkRange* r, gpointer) {
    g_stagger = gtk_range_get_value(r);
}
static void on_speed(GtkRange* r, gpointer) {
    g_speed = gtk_range_get_value(r);
}
static void on_spacing(GtkRange* r, gpointer) {
    g_char_spacing = gtk_range_get_value(r);
}
static void on_buffer_changed(GtkTextBuffer* buf, gpointer) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    char* text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
    if (!text || strlen(text) == 0) { g_free(text); return; }
    g_current_text = std::string(text);
    g_free(text);

    std::string font_str = "Sans Bold " + std::to_string((int)g_fontsize);
    cairo_surface_t* tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* tmp_cr = cairo_create(tmp);
    g_glyphs = extract_glyphs(g_current_text, font_str, tmp_cr);
    cairo_destroy(tmp_cr);
    cairo_surface_destroy(tmp);
    g_selected_glyph = -1;
}

static void activate(GtkApplication* app, gpointer) {
    // Initial glyph extraction
    cairo_surface_t* tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* tmp_cr = cairo_create(tmp);
    g_glyphs = extract_glyphs(g_current_text, "Sans Bold 72", tmp_cr);
    cairo_destroy(tmp_cr); cairo_surface_destroy(tmp);

    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Synfig — TextGroupLayer PoC v2");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 640);
    apply_dark_theme(window);

    
    GtkWidget* root_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    
    GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(paned, TRUE);
    gtk_paned_set_position(GTK_PANED(paned), 780);

    GtkWidget* canvas_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Canvas top bar with fps label
    GtkWidget* canvas_topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(canvas_topbar, 10);
    gtk_widget_set_margin_top(canvas_topbar, 4);
    gtk_widget_set_margin_bottom(canvas_topbar, 4);
    GtkWidget* canvas_info = gtk_label_new("Canvas 960×400  ·  TextGroupLayer");
    gtk_box_append(GTK_BOX(canvas_topbar), canvas_info);
    g_fps_label = gtk_label_new("-- fps");
    gtk_widget_set_margin_start(g_fps_label, 16);
    gtk_box_append(GTK_BOX(canvas_topbar), g_fps_label);
    gtk_box_append(GTK_BOX(canvas_vbox), canvas_topbar);

    // Drawing area
    g_canvas = gtk_drawing_area_new();
    GtkGesture* drag = gtk_gesture_drag_new();
gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), 1); // left mouse button
g_signal_connect(drag, "drag-begin",  G_CALLBACK(on_drag_begin),  nullptr);
g_signal_connect(drag, "drag-update", G_CALLBACK(on_drag_update), nullptr);
gtk_widget_add_controller(g_canvas, GTK_EVENT_CONTROLLER(drag));
    gtk_widget_set_vexpand(g_canvas, TRUE);
    gtk_widget_set_hexpand(g_canvas, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(g_canvas), on_draw, nullptr, nullptr);
    gtk_widget_add_tick_callback(g_canvas, on_tick, nullptr, nullptr);
    gtk_box_append(GTK_BOX(canvas_vbox), g_canvas);

    // Text entry bar
    GtkWidget* entry_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(entry_bar, 10);
    gtk_widget_set_margin_end(entry_bar, 10);
    gtk_widget_set_margin_top(entry_bar, 6);
    gtk_widget_set_margin_bottom(entry_bar, 6);
    GtkWidget* entry_label = gtk_label_new("text  ");
    GtkWidget* textview = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
	gtk_widget_set_hexpand(textview, TRUE);
	gtk_widget_set_size_request(textview, -1, 60);
	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_set_text(buf, "SYNFIG\nSTUDIO", -1);
	g_signal_connect(buf, "changed", G_CALLBACK(on_buffer_changed), nullptr);

	gtk_box_append(GTK_BOX(entry_bar), entry_label);
	gtk_box_append(GTK_BOX(entry_bar), textview);
	
    // gtk_widget_set_hexpand(entry, TRUE);
    // g_signal_connect(entry, "changed", G_CALLBACK(on_text_changed), nullptr);
    gtk_box_append(GTK_BOX(entry_bar), entry_label);
    // gtk_box_append(GTK_BOX(entry_bar), entry);
    gtk_box_append(GTK_BOX(canvas_vbox), entry_bar);

    gtk_paned_set_start_child(GTK_PANED(paned), canvas_vbox);

    // RIGHT: parameters panel 
    GtkWidget* panel_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(panel_vbox, 280, -1);

    GtkWidget* panel_title = gtk_label_new("Parameters — GlyphLayer");
    gtk_widget_set_margin_top(panel_title, 8);
    gtk_widget_set_margin_bottom(panel_title, 6);
    gtk_box_append(GTK_BOX(panel_vbox), panel_title);

    // Mode buttons
    GtkWidget* mode_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(mode_grid), 4);
    gtk_grid_set_row_spacing(GTK_GRID(mode_grid), 4);
    gtk_widget_set_margin_start(mode_grid, 8);
    gtk_widget_set_margin_end(mode_grid, 8);
    gtk_widget_set_margin_bottom(mode_grid, 8);


    const char* mlabels[] = {"Wave", "Scale pop-in", "Color cycle", "All"};
    EffectMode  mmodes[]  = {MODE_WAVE, MODE_SCALE_POPIN, MODE_COLOR, MODE_ALL};
    GtkWidget* first_btn = nullptr;
    for (int i = 0; i < 4; i++) {
        GtkWidget* btn = gtk_toggle_button_new_with_label(mlabels[i]);
        if (i == 0) { gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), TRUE); first_btn = btn; }
        else gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(btn), GTK_TOGGLE_BUTTON(first_btn));
        g_signal_connect(btn, "toggled", G_CALLBACK(on_mode_changed), (gpointer)(gintptr)mmodes[i]);
        gtk_grid_attach(GTK_GRID(mode_grid), btn, i % 2, i / 2, 1, 1);
        gtk_widget_set_hexpand(btn, TRUE);
    }
    GtkWidget* pause_btn = gtk_toggle_button_new_with_label("Pause / Play");
	g_signal_connect(pause_btn, "toggled", G_CALLBACK(on_pause_toggle), nullptr);
	gtk_grid_attach(GTK_GRID(mode_grid), pause_btn, 0, 2, 2, 1);  // spans both columns, row 2
	gtk_widget_set_hexpand(pause_btn, TRUE);
	gtk_box_append(GTK_BOX(panel_vbox), mode_grid);

    // Separator
    gtk_box_append(GTK_BOX(panel_vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Sliders
    GtkWidget* sliders = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(sliders), 8);
    gtk_grid_set_row_spacing(GTK_GRID(sliders), 6);
    gtk_widget_set_margin_start(sliders, 10);
    gtk_widget_set_margin_end(sliders, 10);
    gtk_widget_set_margin_top(sliders, 10);

    auto add_slider = [&](int row, const char* name,
                          double lo, double hi, double val,
                          GCallback cb) {
        GtkWidget* lbl = gtk_label_new(name);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(sliders), lbl, 0, row, 1, 1);
        GtkWidget* sc = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, lo, hi,
                                                  (hi - lo) / 100.0);
        gtk_range_set_value(GTK_RANGE(sc), val);
        gtk_scale_set_draw_value(GTK_SCALE(sc), TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(sc), GTK_POS_RIGHT);
        gtk_widget_set_hexpand(sc, TRUE);
        g_signal_connect(sc, "value-changed", cb, nullptr);
        gtk_grid_attach(GTK_GRID(sliders), sc, 1, row, 1, 1);
    };

    add_slider(0, "stagger (s)",  0.0,  0.5,  0.08, G_CALLBACK(on_stagger));
    add_slider(1, "speed",        0.2,  4.0,  1.1,  G_CALLBACK(on_speed));
    add_slider(2, "amplitude",    0.0,  60.0, 28.0, G_CALLBACK(on_amplitude));
    add_slider(3, "font size",    24.0, 120.0,72.0, G_CALLBACK(on_fontsize));
	add_slider(4, "rotation (°)", -45.0, 45.0, 0.0, G_CALLBACK(on_rotation));
	add_slider(5, "char spacing", -20.0, 40.0, 0.0, G_CALLBACK(on_spacing));

    gtk_box_append(GTK_BOX(panel_vbox), sliders);
    gtk_paned_set_end_child(GTK_PANED(paned), panel_vbox);

    gtk_box_append(GTK_BOX(root_vbox), paned);

    // Status bar
    GtkWidget* statusbar = gtk_label_new(
        "TextGroupLayer PoC v2  ·  Pango/Cairo testbed  ·  Production: FreeType+HarfBuzz  ·  GTK4");
    gtk_widget_set_margin_top(statusbar, 4);
    gtk_widget_set_margin_bottom(statusbar, 4);
    gtk_box_append(GTK_BOX(root_vbox), statusbar);

    gtk_window_set_child(GTK_WINDOW(window), root_vbox);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new("org.synfig.glyph-poc",
                                               G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
