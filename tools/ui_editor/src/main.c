// ui_editor: drag-and-drop editor for the RPG's canvas menus.
//
//   ./build/ui_editor [repo_root]
//
// Reads the game's code (never writes it), lets you lay a screen out, and on
// Build hands back two blocks: new textures, and the CanvasElement /
// CanvasElementText tables. Drafts are saved in tools/ui_editor/out/.

#include "cli.h"
#include "model.h"
#include "raylib.h"
#include "raygui.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ─── state ───────────────────────────────────────────────────

#define UNDO_MAX   96
#define ROW_H      22
#define PANEL_W    330
#define TOP_H      38
#define STATUS_H   24
#define FONT_SIZE  16

static Project P;
static Layout  L;
static Screen  cur;
static bool    have_screen;
static bool    dirty;

static Layout undo_stack[UNDO_MAX], redo_stack[UNDO_MAX];
static int    undo_n, redo_n;

static int  sel = -1;            // selected item
static int  lang = 0;            // preview language
static bool hover_preview = true;
static bool snap_on = true;
static int  grid = 1;            // canvas px. Positions snap to multiples of this
static bool show_guides = true;

static Font ui_font, mono_font;
static Font game_font;           // the game's own, for labels

// view: canvas px -> screen px is  pan + p * zoom
static float   zoom = 0.5f;
static Vector2 pan;
static Rectangle view_r;         // the canvas viewport on screen

// assets/ browser
typedef struct { char path[PATH_LEN]; Texture2D tex; bool tried; } Asset;
static Asset *assets;
static int    asset_count;

static int  browser_tab = 0;     // 0 declared textures, 1 assets, 2 texts (labels)
static int  browser_sel[3] = {-1, -1, -1};
static float browser_scroll[3];
static char filter[128];

static float layers_scroll;

// interaction
typedef enum { DRAG_NONE, DRAG_MOVE, DRAG_RESIZE, DRAG_PAN, DRAG_BROWSER } DragKind;
static DragKind drag;
static int       drag_handle;
static Vector2   drag_start_mouse;     // canvas coords (screen coords for browser)
static Rectangle drag_start_rect;
static bool      drag_pushed;          // undo snapshot taken for this drag
static bool      linked[MAX_ITEMS];    // labels that ride along with the dragged button
static bool      guide_x_on, guide_y_on;
static float     guide_x, guide_y;

// text fields
static int  active_field;
static char field_buf[PATH_LEN];
// Clicking field B while editing field A: A must commit first, whatever the
// draw order, so B only becomes active at the end of the frame.
static int  pending_field;
static char pending_buf[PATH_LEN];

// modals
typedef enum { MODAL_NONE, MODAL_OPEN, MODAL_NEW, MODAL_BUILD, MODAL_CONFIRM } Modal;
typedef enum { ACT_NONE, ACT_OPEN, ACT_NEW, ACT_RELOAD, ACT_QUIT } Action;
static Modal  modal = MODAL_OPEN;
static Action pending;
static int    pending_screen;
static char   new_name[NAME_LEN] = "settings_menu";
static bool   new_name_edit = true;
static char  *build_tex, *build_tables, *build_warn;
// clickable buttons: listed in red in the Build dialog, and a big popup on copying the tables
static int    click_n;
static char   click_line[1024], click_lines[2048];
static bool   click_popup;
static char   build_path[PATH_LEN * 2];
static float  build_scroll;
static float  open_scroll;
static bool   quit;

static char   status[256];
static double status_until;
static bool   status_bad;

static void set_status(bool bad, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(status, sizeof(status), fmt, ap);
    va_end(ap);
    status_until = GetTime() + 5.0;
    status_bad = bad;
}

// ─── small helpers ───────────────────────────────────────────

static void text(const char *s, float x, float y, Color c) {
    DrawTextEx(ui_font, s, (Vector2){x, y}, FONT_SIZE, 1, c);
}

static float text_w(const char *s) { return MeasureTextEx(ui_font, s, FONT_SIZE, 1).x; }

// Long asset paths: keep the end, it's the interesting part.
static const char *fit(const char *s, float max_w) {
    static char out[PATH_LEN];
    if (text_w(s) <= max_w) return s;
    const char *t = s;
    while (*t) {
        snprintf(out, sizeof(out), "...%s", t);
        if (text_w(out) <= max_w) return out;
        t++;
    }
    return "...";
}

static Vector2 to_canvas(Vector2 m) { return (Vector2){(m.x - pan.x) / zoom, (m.y - pan.y) / zoom}; }
static Rectangle to_screen(Rectangle r) { return (Rectangle){pan.x + r.x * zoom, pan.y + r.y * zoom, r.width * zoom, r.height * zoom}; }

static void fit_view(void) {
    float z = fminf(view_r.width / P.canvas_w, view_r.height / P.canvas_h) * 0.94f;
    zoom = z;
    pan.x = view_r.x + (view_r.width  - P.canvas_w * z) / 2.0f;
    pan.y = view_r.y + (view_r.height - P.canvas_h * z) / 2.0f;
}

static float snapv(float v) {
    if (!snap_on || grid <= 0) return roundf(v * 2.0f) / 2.0f;    // still keep it to half pixels
    return roundf(v / grid) * grid;
}

static bool rect_eq(Rectangle a, Rectangle b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

// ─── textures ────────────────────────────────────────────────

typedef struct { Texture2D t; float fw, fh; bool missing; } Drawn;

// Same fallback as texture_load_all(): a texture that won't load becomes TEX_NONE.
static Drawn tex_drawn(int i) {
    if (i < 0 || i >= P.tex_count) i = P.tex_none;
    TexEntry *e = &P.tex[i];
    if (!e->load_tried) {
        e->load_tried = true;
        char full[PATH_LEN * 2];
        snprintf(full, sizeof(full), "%s/%s", P.root, e->path);
        if (e->path[0] && FileExists(full)) {
            e->tex = LoadTexture(full);
            if (e->tex.id && e->smooth) SetTextureFilter(e->tex, TEXTURE_FILTER_BILINEAR);
        }
    }
    if (e->tex.id == 0) {
        if (i == P.tex_none) return (Drawn){0};
        Drawn d = tex_drawn(P.tex_none);
        d.missing = true;
        return d;
    }
    Drawn d = {e->tex, (float)e->frame_w, (float)e->frame_h, false};
    if (d.fw <= 0) d.fw = (float)e->tex.width;
    if (d.fh <= 0) d.fh = (float)e->tex.height;
    return d;
}

static Texture2D asset_tex(int a) {
    Asset *as = &assets[a];
    if (!as->tried) {
        as->tried = true;
        char full[PATH_LEN * 2];
        snprintf(full, sizeof(full), "%s/%s", P.root, as->path);
        as->tex = LoadTexture(full);
        if (as->tex.id) SetTextureFilter(as->tex, TEXTURE_FILTER_BILINEAR);
    }
    return as->tex;
}

static void draw_tex(Drawn d, Rectangle canvas_rect, Color tint) {
    if (d.t.id == 0) { DrawRectangleRec(to_screen(canvas_rect), MAGENTA); return; }
    DrawTexturePro(d.t, (Rectangle){0, 0, d.fw, d.fh}, to_screen(canvas_rect), (Vector2){0, 0}, 0, tint);
}

static int cmp_asset(const void *a, const void *b) { return strcmp(((const Asset *)a)->path, ((const Asset *)b)->path); }

static void scan_assets(void) {
    for (int i = 0; i < asset_count; i++) if (assets[i].tex.id) UnloadTexture(assets[i].tex);
    free(assets);
    assets = NULL;
    asset_count = 0;
    char dir[PATH_LEN * 2];
    snprintf(dir, sizeof(dir), "%s/assets", P.root);
    if (!DirectoryExists(dir)) return;
    FilePathList fl = LoadDirectoryFilesEx(dir, ".png", true);
    assets = calloc(fl.count ? fl.count : 1, sizeof(Asset));
    size_t skip = strlen(P.root) + 1;
    for (unsigned i = 0; i < fl.count; i++) {
        const char *rel = strlen(fl.paths[i]) > skip ? fl.paths[i] + skip : fl.paths[i];
        snprintf(assets[asset_count++].path, PATH_LEN, "%s", rel);
    }
    UnloadDirectoryFiles(fl);
    qsort(assets, asset_count, sizeof(Asset), cmp_asset);
}

// The browser's selection as a registry index, registering a new texture
// the first time an asset png is used.
static int browser_resolve(void) {
    if (browser_tab == 2) return -1;
    if (browser_tab == 0) return browser_sel[0];
    int a = browser_sel[1];
    if (a < 0) return -1;
    int found = tex_find_path(&P, assets[a].path);
    if (found >= 0) return found;
    Texture2D t = asset_tex(a);
    if (t.id == 0) { set_status(true, "Couldn't load %s", assets[a].path); return -1; }
    int idx = tex_add_new(&P, assets[a].path, t.width, t.height);
    if (idx < 0) { set_status(true, "Too many textures"); return -1; }
    set_status(false, "New texture %s (rename it on the left if you want)", P.tex[idx].name);
    return idx;
}

// ─── layout edits ────────────────────────────────────────────

static void undo_push(void) {
    if (undo_n == UNDO_MAX) { memmove(undo_stack, undo_stack + 1, sizeof(Layout) * (UNDO_MAX - 1)); undo_n--; }
    undo_stack[undo_n++] = L;
    redo_n = 0;
    dirty = true;
}

static void undo(void) {
    if (!undo_n) { set_status(false, "Nothing to undo"); return; }
    redo_stack[redo_n++] = L;
    L = undo_stack[--undo_n];
    if (sel >= L.count) sel = -1;
    dirty = true;
}

static void redo(void) {
    if (!redo_n) { set_status(false, "Nothing to redo"); return; }
    undo_stack[undo_n++] = L;
    L = redo_stack[--redo_n];
    if (sel >= L.count) sel = -1;
    dirty = true;
}

static int image_count(void) {
    int n = 0;
    for (int i = 0; i < L.count; i++) if (L.items[i].kind == ITEM_IMAGE) n++;
    return n;
}

// Images always sit before labels in the array: that's the order the game
// draws them (all of UIElements, then all of UITexts).
static int insert_item(Item it, int at) {
    if (L.count >= MAX_ITEMS) { set_status(true, "Too many items"); return -1; }
    int lo = it.kind == ITEM_IMAGE ? 0 : image_count();
    int hi = it.kind == ITEM_IMAGE ? image_count() : L.count;
    if (at < lo || at > hi) at = hi;
    memmove(&L.items[at + 1], &L.items[at], sizeof(Item) * (L.count - at));
    L.items[at] = it;
    L.count++;
    return at;
}

static void delete_item(int i) {
    memmove(&L.items[i], &L.items[i + 1], sizeof(Item) * (L.count - i - 1));
    L.count--;
}

static Item new_item(ItemKind k) {
    Item it = {0};
    it.kind = k;
    it.tex = it.hover_tex = P.tex_none;
    it.text = -1;
    return it;
}

static const char *label_str(const Item *it) {
    const char *s = text_str(&P, it->text, lang);
    return s[0] ? s : "(no text)";
}

static Vector2 label_size(const char *s) {
    return MeasureTextEx(game_font, s, P.text_h, 0);
}

// Same as text_draw() in src/text.c: font size MENU_TEXT_HEIGHT, centred in area.
static Rectangle label_rect(const Item *it) {
    Vector2 sz = label_size(label_str(it));
    return (Rectangle){it->area.x + (it->area.width - sz.x) / 2.0f, it->area.y + (it->area.height - sz.y) / 2.0f, sz.x, sz.y};
}

static void draw_label(const char *s, Rectangle canvas_rect) {
    Rectangle r = to_screen(canvas_rect);
    float size = P.text_h * zoom, ow = P.text_outline * zoom;
    Color fill = {251, 228, 168, 255}, outline = {29, 5, 28, 255};   // same as src/text.c
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            DrawTextEx(game_font, s, (Vector2){r.x + dx * ow, r.y + dy * ow}, size, 0, outline);
    DrawTextEx(game_font, s, (Vector2){r.x, r.y}, size, 0, fill);
}

static Rectangle hit_rect(const Item *it) { return it->kind == ITEM_TEXT ? label_rect(it) : it->area; }

// Labels sharing a button's exact rect move and resize with it.
static void collect_links(int i) {
    memset(linked, 0, sizeof(linked));
    if (L.items[i].kind != ITEM_IMAGE) return;
    for (int k = 0; k < L.count; k++)
        if (L.items[k].kind == ITEM_TEXT && rect_eq(L.items[k].area, L.items[i].area)) linked[k] = true;
}

static void set_area(int i, Rectangle r) {
    for (int k = 0; k < L.count; k++) if (linked[k]) L.items[k].area = r;
    L.items[i].area = r;
}

static void set_area_from_field(int i, Rectangle r) {
    collect_links(i);
    set_area(i, r);
    memset(linked, 0, sizeof(linked));
}

static int hit_test(Vector2 c, bool clickable_images_only) {
    for (int i = L.count - 1; i >= 0; i--) {
        const Item *it = &L.items[i];
        if (it->locked) continue;
        if (clickable_images_only && (it->kind != ITEM_IMAGE || !it->clickable)) continue;
        if (CheckCollisionPointRec(c, hit_rect(it))) return i;
    }
    return -1;
}

static int find_image_under(Rectangle r) {
    Vector2 c = {r.x + r.width / 2, r.y + r.height / 2};
    for (int i = L.count - 1; i >= 0; i--)
        if (L.items[i].kind == ITEM_IMAGE && L.items[i].clickable && CheckCollisionPointRec(c, L.items[i].area)) return i;
    for (int i = L.count - 1; i >= 0; i--)
        if (L.items[i].kind == ITEM_IMAGE && !rect_eq(L.items[i].area, (Rectangle){0, 0, P.canvas_w, P.canvas_h}) &&
            CheckCollisionPointRec(c, L.items[i].area)) return i;
    return -1;
}

static void move_in_kind(int i, int dir) {
    int j = i + dir;
    if (j < 0 || j >= L.count || L.items[j].kind != L.items[i].kind) return;
    undo_push();
    Item t = L.items[i]; L.items[i] = L.items[j]; L.items[j] = t;
    sel = j;
}

static void duplicate(int i) {
    undo_push();
    Item it = L.items[i];
    it.area.x += 20; it.area.y += 20;
    it.locked = false;
    int at = insert_item(it, i + 1);
    if (at >= 0) sel = at;
}

// Drop a texture from the browser onto the canvas.
static void drop_text(Vector2 c) {
    int t = browser_sel[2];
    if (t < 0) return;

    // Onto an existing label: swap its text
    for (int i = L.count - 1; i >= 0; i--) {
        Item *it = &L.items[i];
        if (it->kind != ITEM_TEXT || !CheckCollisionPointRec(c, it->area)) continue;
        undo_push();
        it->text = t;
        sel = i;
        set_status(false, "Label %d is now %s", layout_index_of_kind(&L, i), P.texts[t].name);
        return;
    }

    Item it = new_item(ITEM_TEXT);
    it.text = t;
    int btn = hit_test(c, true);
    if (btn >= 0) {
        it.area = L.items[btn].area;          // snaps onto the button: same rect, as in game.c
    } else {
        Vector2 sz = label_size(label_str(&it));
        it.area = (Rectangle){snapv(c.x - sz.x / 2), snapv(c.y - sz.y / 2), roundf(sz.x), roundf(sz.y)};
    }
    undo_push();
    int at = insert_item(it, -1);
    if (at >= 0) sel = at;
    set_status(false, btn >= 0 ? "Label snapped onto UIElements[%d]" : "Added label", btn >= 0 ? layout_index_of_kind(&L, btn) : 0);
}

static void drop_texture(Vector2 c) {
    if (browser_tab == 2) { drop_text(c); return; }
    int t = browser_resolve();
    if (t < 0) return;
    Drawn d = tex_drawn(t);

    Item it = new_item(ITEM_IMAGE);
    it.tex = t;
    float w = d.fw, h = d.fh;
    float s = fminf(1.0f, fminf(P.canvas_w / w, P.canvas_h / h));   // never bigger than the canvas
    w *= s; h *= s;
    it.area = (Rectangle){snapv(c.x - w / 2), snapv(c.y - h / 2), roundf(w), roundf(h)};
    undo_push();
    int at = insert_item(it, -1);
    if (at >= 0) sel = at;
    set_status(false, "Added %s", P.tex[t].name);
}

// ─── screens ─────────────────────────────────────────────────

static void open_screen(int s) {
    Layout tmp;
    int warn_before = P.warn_count;
    if (!layout_load(&P, s, &tmp)) {
        set_status(true, "Couldn't read %s: %s", P.screens[s].name, P.warn_count > warn_before ? P.warn[P.warn_count - 1] : "?");
        return;
    }
    L = tmp;
    cur = P.screens[s];
    have_screen = true;
    dirty = false;
    undo_n = redo_n = 0;
    sel = -1;
    modal = MODAL_NONE;
    fit_view();
    set_status(false, "Opened %s (%s), %d items", cur.name, cur.source, L.count);
}

static void new_screen(const char *name) {
    memset(&L, 0, sizeof(L));
    memset(&cur, 0, sizeof(cur));
    snprintf(cur.name, NAME_LEN, "%s", name);
    snprintf(cur.source, PATH_LEN, "tools/ui_editor/out/%s.c", name);
    cur.is_draft = true;
    have_screen = true;
    dirty = false;
    undo_n = redo_n = 0;
    sel = -1;
    modal = MODAL_NONE;
    fit_view();
    set_status(false, "New canvas %s. Drag textures in from the left.", name);
}

static int find_screen_by(const char *name, bool draft) {
    for (int i = 0; i < P.screen_count; i++)
        if (P.screens[i].is_draft == draft && strcmp(P.screens[i].name, name) == 0) return i;
    return -1;
}

// The game's font, with the same characters text.c bakes, so '?' shows up here too.
static void load_game_font(void) {
    if (game_font.texture.id && game_font.texture.id != GetFontDefault().texture.id) UnloadFont(game_font);
    game_font = GetFontDefault();
    char full[PATH_LEN * 2];
    snprintf(full, sizeof(full), "%s/%s", P.root, P.font_path);
    if (!P.font_path[0] || !FileExists(full)) { project_warn(&P, "can't find the game font (%s), labels use raylib's", P.font_path); return; }
    int n = 0;
    int *cps = LoadCodepoints(P.font_charset[0] ? P.font_charset : " ", &n);
    game_font = LoadFontEx(full, (int)P.font_bake_size, P.font_charset[0] ? cps : NULL, P.font_charset[0] ? n : 0);
    UnloadCodepoints(cps);
    SetTextureFilter(game_font.texture, TEXTURE_FILTER_BILINEAR);
}

static void reload(void) {
    char name[NAME_LEN];
    bool was_draft = cur.is_draft, had = have_screen;
    snprintf(name, sizeof(name), "%s", cur.name);
    for (int i = 0; i < P.tex_count; i++) if (P.tex[i].tex.id) UnloadTexture(P.tex[i].tex);
    char root[PATH_LEN];
    snprintf(root, sizeof(root), "%s", P.root);
    if (!project_load(&P, root)) { set_status(true, "Reload failed: can't read the game"); return; }
    scan_assets();
    load_game_font();
    browser_sel[0] = browser_sel[1] = browser_sel[2] = -1;
    int s = had ? find_screen_by(name, was_draft) : -1;
    if (s >= 0) open_screen(s);
    else if (had) new_screen(name);
    set_status(false, "Re-read the game's code (%d textures, %d screens)", P.tex_count, P.screen_count);
}

static void do_build(void) {
    free(build_tex); free(build_tables); free(build_warn);
    export_blocks(&P, &cur, &L, &build_tex, &build_tables, &build_warn);
    if (export_write_draft(&P, &cur, build_tex, build_tables, build_path, sizeof(build_path))) {
        dirty = false;
        if (find_screen_by(cur.name, true) < 0 && P.screen_count < MAX_SCREENS) {
            Screen d = cur;
            d.is_draft = true;
            snprintf(d.source, PATH_LEN, "tools/ui_editor/out/%s.c", cur.name);
            if (!d.elem_table[0]) snprintf(d.elem_table, NAME_LEN, "UIElements");
            if (!d.text_table[0]) snprintf(d.text_table, NAME_LEN, "UITexts");
            P.screens[P.screen_count++] = d;
        }
    } else {
        snprintf(build_path, sizeof(build_path), "(couldn't write the draft file!)");
    }
    printf("\n// ===== BLOCK 1: textures =====\n%s\n// ===== BLOCK 2: tables =====\n%s", build_tex, build_tables);
    if (build_warn[0]) printf("\n%s", build_warn);
    fflush(stdout);
    const char *en = cur.elem_table[0] ? cur.elem_table : "UIElements";
    click_n = export_clickables(&P, &L, en, click_line, sizeof(click_line), false);
    export_clickables(&P, &L, en, click_lines, sizeof(click_lines), true);
    click_popup = false;
    build_scroll = 0;
    modal = MODAL_BUILD;
    status_until = 0;     // don't show a stale message in the dialog
}

static void request(Action a) {
    pending = a;
    if (dirty && a != ACT_NONE) { modal = MODAL_CONFIRM; return; }
    switch (a) {
        case ACT_OPEN:   open_screen(pending_screen); break;
        case ACT_NEW:    new_screen(new_name); break;
        case ACT_RELOAD: reload(); break;
        case ACT_QUIT:   quit = true; break;
        default: break;
    }
    pending = ACT_NONE;
}

// ─── text fields ─────────────────────────────────────────────

// One field edited at a time. Returns true when the edit is committed
// (Enter, or clicking anywhere else).
static bool field_text(int id, Rectangle r, char *value, int size) {
    if (active_field == id) {
        if (GuiTextBox(r, field_buf, size < (int)sizeof(field_buf) ? size : (int)sizeof(field_buf), true)) {
            active_field = 0;
            snprintf(value, size, "%s", field_buf);
            return true;
        }
        return false;
    }
    char shown[PATH_LEN];
    snprintf(shown, sizeof(shown), "%s", value);
    if (GuiTextBox(r, shown, sizeof(shown), false) && !GuiIsLocked()) {
        pending_field = id;
        snprintf(pending_buf, sizeof(pending_buf), "%s", value);
    }
    return false;
}

static bool field_float(int id, Rectangle r, const char *label, float *v) {
    text(label, r.x, r.y + 3, DARKGRAY);
    float lw = 18;
    char s[64];
    snprintf(s, sizeof(s), "%g", *v);
    Rectangle fr = {r.x + lw, r.y, r.width - lw, r.height};
    if (field_text(id, fr, s, sizeof(s))) {
        char *end;
        float nv = strtof(s, &end);
        if (end != s && nv != *v) { *v = nv; return true; }
    }
    return false;
}

static bool field_int(int id, Rectangle r, const char *label, int *v) {
    float f = (float)*v;
    if (field_float(id, r, label, &f)) { *v = (int)f; return true; }
    return false;
}

// ─── left panel: textures ────────────────────────────────────

static bool matches(const char *s) {
    if (!filter[0]) return true;
    for (const char *h = s; *h; h++) {
        const char *a = h, *b = filter;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
        if (!*b) return true;
    }
    return false;
}

static void left_panel(Rectangle r) {
    bool blocked = modal != MODAL_NONE;
    GuiPanel(r, NULL);
    float x = r.x + 8, w = r.width - 16, y = r.y + 8;

    int prev_tab = browser_tab;
    GuiToggleGroup((Rectangle){x, y, w / 3 - 2, 26}, "Textures;assets/;Texts", &browser_tab);
    if (prev_tab != browser_tab) active_field = 0;
    y += 32;
    text("Filter", x, y + 4, DARKGRAY);
    field_text(1, (Rectangle){x + 48, y, w - 48, 24}, filter, sizeof(filter));
    y += 30;

    // preview area takes the bottom
    float preview_h = 290;
    Rectangle list = {x, y, w, r.y + r.height - preview_h - y - 8};
    DrawRectangleRec(list, (Color){250, 250, 250, 255});
    DrawRectangleLinesEx(list, 1, LIGHTGRAY);

    static int idx[MAX_TEX > 8192 ? MAX_TEX : 8192];
    int n = 0;
    int total = browser_tab == 0 ? P.tex_count : browser_tab == 1 ? asset_count : P.text_count;
    for (int i = 0; i < total && n < (int)(sizeof(idx) / sizeof(idx[0])); i++) {
        if (browser_tab == 2) {
            if (matches(P.texts[i].name) || matches(P.texts[i].str[0])) idx[n++] = i;
            continue;
        }
        const char *s = browser_tab == 0 ? P.tex[i].name : assets[i].path;
        if (matches(s) || (browser_tab == 0 && matches(P.tex[i].path))) idx[n++] = i;
    }

    float *scroll = &browser_scroll[browser_tab];
    Vector2 m = GetMousePosition();
    bool over = CheckCollisionPointRec(m, list) && !blocked;
    if (over) *scroll -= GetMouseWheelMove() * ROW_H * 3;
    float max_scroll = fmaxf(0, n * ROW_H - list.height);
    if (*scroll > max_scroll) *scroll = max_scroll;
    if (*scroll < 0) *scroll = 0;

    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
    int first = (int)(*scroll / ROW_H);
    for (int k = first; k < n; k++) {
        float ry = list.y + k * ROW_H - *scroll;
        if (ry > list.y + list.height) break;
        Rectangle row = {list.x, ry, list.width, ROW_H};
        int i = idx[k];
        bool is_sel = browser_sel[browser_tab] == i;
        bool hot = over && CheckCollisionPointRec(m, row);
        if (is_sel) DrawRectangleRec(row, (Color){180, 210, 255, 255});
        else if (hot) DrawRectangleRec(row, (Color){230, 238, 250, 255});

        Color c = BLACK;
        const char *label;
        if (browser_tab == 2) {
            label = TextFormat("%s   %s", P.texts[i].name, P.texts[i].str[0]);
        } else if (browser_tab == 0) {
            label = P.tex[i].name;
            if (P.tex[i].is_new) c = (Color){0, 120, 40, 255};
            if (!P.tex[i].path[0]) c = GRAY;
        } else {
            label = assets[i].path + (strncmp(assets[i].path, "assets/", 7) == 0 ? 7 : 0);
            if (tex_find_path(&P, assets[i].path) >= 0) c = (Color){30, 60, 160, 255};
        }
        text(fit(label, row.width - 10), row.x + 5, row.y + 3, c);

        if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            browser_sel[browser_tab] = i;
            drag = DRAG_BROWSER;
            drag_start_mouse = m;
        }
    }
    EndScissorMode();
    if (n == 0) text("(nothing matches)", list.x + 8, list.y + 8, GRAY);

    // preview + details of the selection
    y = list.y + list.height + 8;
    Rectangle pv = {x, y, w, 150};
    DrawRectangleRec(pv, (Color){40, 40, 46, 255});
    int s = browser_sel[browser_tab];
    if (browser_tab == 2) {
        if (s < 0) {
            text("Pick a text, then drag it onto a", x, y + 156, GRAY);
            text("button (or double-click it).", x, y + 174, GRAY);
            text("New texts: add them to src/text.c", x, y + 204, GRAY);
            text("(enum in include/text.h), Reload.", x, y + 222, GRAY);
            return;
        }
        const char *str = text_str(&P, s, lang);
        Vector2 sz = MeasureTextEx(game_font, str, 40, 0);
        float fs = sz.x > pv.width - 10 ? 40 * (pv.width - 10) / sz.x : 40;
        sz = MeasureTextEx(game_font, str, fs, 0);
        DrawTextEx(game_font, str, (Vector2){pv.x + (pv.width - sz.x) / 2, pv.y + (pv.height - sz.y) / 2}, fs, 0, (Color){251, 228, 168, 255});
        y += 156;
        text(P.texts[s].name, x, y, BLACK);
        y += 22;
        for (int l = 0; l < P.lang_count; l++) {
            const char *ln = strncmp(P.langs[l], "LANG_", 5) == 0 ? P.langs[l] + 5 : P.langs[l];
            const char *v = P.texts[s].str[l];
            text(TextFormat("%s  %s", ln, v[0] ? v : "(uses the first one)"), x, y, v[0] ? DARKGRAY : GRAY);
            y += 18;
        }
        text("Edit them in src/text.c", x, y + 6, GRAY);
        return;
    }
    if (s >= 0) {
        Texture2D t = {0};
        float fw = 0, fh = 0;
        if (browser_tab == 0) { Drawn d = tex_drawn(s); t = d.t; fw = d.fw; fh = d.fh; }
        else { t = asset_tex(s); fw = (float)t.width; fh = (float)t.height; }
        if (t.id && fw > 0 && fh > 0) {
            float sc = fminf((pv.width - 10) / fw, (pv.height - 10) / fh);
            Rectangle dst = {pv.x + (pv.width - fw * sc) / 2, pv.y + (pv.height - fh * sc) / 2, fw * sc, fh * sc};
            DrawTexturePro(t, (Rectangle){0, 0, fw, fh}, dst, (Vector2){0, 0}, 0, WHITE);
        }
    }
    y += 156;

    if (s < 0) {
        text("Pick a texture, then drag it onto", x, y, GRAY);
        text("the canvas (or double-click it).", x, y + 18, GRAY);
        return;
    }

    if (browser_tab == 1) {
        Texture2D t = asset_tex(s);
        text(fit(assets[s].path, w), x, y, DARKGRAY);
        text(TextFormat("%d x %d px", t.width, t.height), x, y + 20, DARKGRAY);
        int known = tex_find_path(&P, assets[s].path);
        if (known >= 0) text(TextFormat("already %s", P.tex[known].name), x, y + 40, (Color){30, 60, 160, 255});
        else text("not declared yet: becomes a new texture", x, y + 40, (Color){0, 120, 40, 255});
        return;
    }

    TexEntry *e = &P.tex[s];
    if (!e->is_new) {
        text(fit(e->name, w), x, y, BLACK);
        text(fit(e->path[0] ? e->path : "(no texture_defs entry)", w), x, y + 20, DARKGRAY);
        text(TextFormat("frame %d x %d   %s", e->frame_w, e->frame_h, e->smooth ? "smooth" : "pixel"), x, y + 40, DARKGRAY);
        text("declared in the game (read-only)", x, y + 60, GRAY);
        return;
    }

    // a new texture: everything that goes into its texture_defs line is editable
    char name[NAME_LEN];
    snprintf(name, sizeof(name), "%s", e->name);
    if (field_text(2, (Rectangle){x, y, w, 24}, name, sizeof(name))) {
        if (!ident_valid(name)) set_status(true, "\"%s\" isn't a valid C name", name);
        else if (tex_find(&P, name) >= 0 && strcmp(name, e->name) != 0) set_status(true, "%s already exists", name);
        else snprintf(e->name, NAME_LEN, "%s", name);
    }
    y += 28;
    field_int(3, (Rectangle){x, y, w / 2 - 4, 24}, "w", &e->frame_w);
    field_int(4, (Rectangle){x + w / 2 + 4, y, w / 2 - 4, 24}, "h", &e->frame_h);
    y += 28;
    if (GuiCheckBox((Rectangle){x, y + 4, 16, 16}, "smooth (bilinear)", &e->smooth) && e->tex.id)
        SetTextureFilter(e->tex, e->smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    text("NEW", x + w - 36, y + 3, (Color){0, 120, 40, 255});
}

// ─── right panel: properties + layers ────────────────────────

static void tex_slot(int id_base, float x, float y, float w, const char *label, int *slot, bool clearable) {
    text(label, x, y + 4, DARKGRAY);
    float lw = 74;
    const char *name = *slot >= 0 ? P.tex[*slot].name : "(none)";
    DrawRectangle((int)(x + lw), (int)y, (int)(w - lw - (clearable ? 92 : 64)), 24, (Color){245, 245, 245, 255});
    text(fit(name, w - lw - (clearable ? 98 : 70)), x + lw + 4, y + 4, *slot >= 0 && *slot != P.tex_none ? BLACK : RED);
    float bx = x + w - (clearable ? 88 : 60);
    if (GuiButton((Rectangle){bx, y, 58, 24}, "Use sel")) {
        int t = browser_resolve();
        if (t >= 0) { undo_push(); *slot = t; }
        else set_status(true, "Select a texture on the left first");
    }
    if (clearable && GuiButton((Rectangle){bx + 62, y, 26, 24}, "x") && *slot >= 0) { undo_push(); *slot = -1; }
}

static void right_panel(Rectangle r) {
    GuiPanel(r, NULL);
    float x = r.x + 8, w = r.width - 16, y = r.y + 8;
    bool blocked = modal != MODAL_NONE;

    float props_h = 430;
    if (sel < 0 || sel >= L.count) {
        text("Nothing selected", x, y, GRAY);
        y += 22;
        text("Click something on the canvas,", x, y, GRAY); y += 18;
        text("or in the list below.", x, y, GRAY); y += 30;
        text("Drag     move", x, y, DARKGRAY); y += 18;
        text("Handles  resize (Shift: keep ratio)", x, y, DARKGRAY); y += 18;
        text("Alt      no snapping while dragging", x, y, DARKGRAY); y += 18;
        text("Arrows   nudge (Shift: x10)", x, y, DARKGRAY); y += 18;
        text("Ctrl+D dup   Del delete", x, y, DARKGRAY); y += 18;
        text("Ctrl+Z / Ctrl+Y   undo / redo", x, y, DARKGRAY); y += 18;
        text("Wheel zoom   Middle/Space drag pan", x, y, DARKGRAY); y += 18;
        text("F fit   Ctrl+B / Ctrl+S build", x, y, DARKGRAY); y += 18;
    } else {
        Item *it = &L.items[sel];
        int k = layout_index_of_kind(&L, sel);
        bool img = it->kind == ITEM_IMAGE;
        int fid = (sel + 1) * 64;

        GuiLabel((Rectangle){x, y, w, 20}, TextFormat("#%i#%s[%d]   %s", img ? 12 : 10,
                 img ? (cur.elem_table[0] ? cur.elem_table : "UIElements") : (cur.text_table[0] ? cur.text_table : "UITexts"),
                 k, img ? "image" : "label"));
        y += 26;

        if (img) { tex_slot(fid, x, y, w, "Texture", &it->tex, false); y += 30; }

        Rectangle a = it->area;
        bool ch = false;
        ch |= field_float(fid + 1, (Rectangle){x, y, w / 2 - 4, 24}, "x", &a.x);
        ch |= field_float(fid + 2, (Rectangle){x + w / 2 + 4, y, w / 2 - 4, 24}, "y", &a.y);
        y += 28;
        ch |= field_float(fid + 3, (Rectangle){x, y, w / 2 - 4, 24}, "w", &a.width);
        ch |= field_float(fid + 4, (Rectangle){x + w / 2 + 4, y, w / 2 - 4, 24}, "h", &a.height);
        y += 30;
        if (ch) { undo_push(); set_area_from_field(sel, a); }

        float bw = (w - 8) / 3;
        if (GuiButton((Rectangle){x, y, bw, 24}, "Center H")) { undo_push(); a.x = (P.canvas_w - a.width) / 2; set_area_from_field(sel, a); }
        if (GuiButton((Rectangle){x + bw + 4, y, bw, 24}, "Center V")) { undo_push(); a.y = (P.canvas_h - a.height) / 2; set_area_from_field(sel, a); }
        if (img) {
            if (GuiButton((Rectangle){x + 2 * bw + 8, y, bw, 24}, "Native size")) {
                Drawn d = tex_drawn(it->tex);
                undo_push(); a.width = d.fw; a.height = d.fh; set_area_from_field(sel, a);
            }
        } else {
            if (GuiButton((Rectangle){x + 2 * bw + 8, y, bw, 24}, "Fit button")) {
                int b = find_image_under(it->area);
                if (b >= 0) { undo_push(); it->area = L.items[b].area; }
                else set_status(true, "No button under this label");
            }
        }
        y += 32;

        if (img) {
            bool was = it->clickable;
            GuiCheckBox((Rectangle){x, y + 4, 16, 16}, "Clickable (hover_effect)", &it->clickable);
            if (was != it->clickable) {
                bool now = it->clickable;
                it->clickable = was; undo_push(); it->clickable = now;
                if (now && it->hover_tex == P.tex_none) it->hover_tex = it->tex;
            }
            y += 28;
            if (it->clickable) { tex_slot(fid + 10, x, y, w, "Hover", &it->hover_tex, false); y += 30; }
            else y += 30;
        } else {
            text("Text", x, y + 4, DARKGRAY);
            float lw = 74;
            DrawRectangle((int)(x + lw), (int)y, (int)(w - lw - 64), 24, (Color){245, 245, 245, 255});
            text(fit(it->text >= 0 ? P.texts[it->text].name : "(none)", w - lw - 70), x + lw + 4, y + 4, it->text >= 0 ? BLACK : RED);
            if (GuiButton((Rectangle){x + w - 60, y, 58, 24}, "Use sel")) {
                if (browser_tab == 2 && browser_sel[2] >= 0) { undo_push(); it->text = browser_sel[2]; }
                else set_status(true, "Select a text in the Texts tab first");
            }
            y += 30;
            if (it->text >= 0) {
                for (int l = 0; l < P.lang_count; l++) {
                    const char *ln = strncmp(P.langs[l], "LANG_", 5) == 0 ? P.langs[l] + 5 : P.langs[l];
                    const char *v = P.texts[it->text].str[l];
                    text(fit(TextFormat("%s  %s", ln, v[0] ? v : "(uses the first one)"), w), x, y, v[0] ? DARKGRAY : GRAY);
                    y += 18;
                }
            }
            y += 6;
        }

        bool was_locked = it->locked;
        GuiCheckBox((Rectangle){x, y + 4, 16, 16}, "Locked (editor only)", &it->locked);
        if (was_locked != it->locked && it->locked && drag != DRAG_NONE) drag = DRAG_NONE;
        y += 28;

        float qw = (w - 12) / 4;
        if (GuiButton((Rectangle){x, y, qw, 24}, "Up"))            move_in_kind(sel, -1);
        if (GuiButton((Rectangle){x + qw + 4, y, qw, 24}, "Down")) move_in_kind(sel, +1);
        if (GuiButton((Rectangle){x + 2 * qw + 8, y, qw, 24}, "Dup")) duplicate(sel);
        if (GuiButton((Rectangle){x + 3 * qw + 12, y, qw, 24}, "Delete")) { undo_push(); delete_item(sel); sel = -1; active_field = 0; }
    }

    // ── layers: the game's draw order ──
    y = r.y + props_h;
    GuiLine((Rectangle){x, y, w, 12}, "Draw order");
    y += 16;
    Rectangle list = {x, y, w, r.y + r.height - y - 8};
    DrawRectangleRec(list, (Color){250, 250, 250, 255});
    DrawRectangleLinesEx(list, 1, LIGHTGRAY);

    Vector2 m = GetMousePosition();
    bool over = CheckCollisionPointRec(m, list) && !blocked;
    if (over) layers_scroll -= GetMouseWheelMove() * ROW_H * 3;
    float max_scroll = fmaxf(0, L.count * ROW_H - list.height);
    if (layers_scroll > max_scroll) layers_scroll = max_scroll;
    if (layers_scroll < 0) layers_scroll = 0;

    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
    for (int i = 0; i < L.count; i++) {
        float ry = list.y + i * ROW_H - layers_scroll;
        if (ry + ROW_H < list.y || ry > list.y + list.height) continue;
        Rectangle row = {list.x, ry, list.width, ROW_H};
        const Item *it = &L.items[i];
        bool hot = over && CheckCollisionPointRec(m, row);
        if (i == sel) DrawRectangleRec(row, (Color){180, 210, 255, 255});
        else if (hot) DrawRectangleRec(row, (Color){230, 238, 250, 255});
        int k = layout_index_of_kind(&L, i);
        const char *nm = it->kind == ITEM_IMAGE ? P.tex[it->tex].name : (it->text >= 0 ? P.texts[it->text].name : "(no text)");
        if (strncmp(nm, "TEX_", 4) == 0 || strncmp(nm, "TXT_", 4) == 0) nm += 4;
        char line[PATH_LEN];
        snprintf(line, sizeof(line), "%s%d %s%s%s", it->kind == ITEM_IMAGE ? "E" : "T", k,
                 it->locked ? "[lock] " : "", it->kind == ITEM_IMAGE && it->clickable ? "[click] " : "", nm);
        text(fit(line, row.width - 10), row.x + 5, row.y + 3, it->kind == ITEM_IMAGE ? BLACK : (Color){120, 30, 140, 255});
        if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { sel = i; active_field = 0; }
    }
    EndScissorMode();
}

// ─── canvas ──────────────────────────────────────────────────

static void canvas_draw(Vector2 mc, bool mouse_in_view) {
    DrawRectangleRec(view_r, (Color){58, 58, 64, 255});
    BeginScissorMode((int)view_r.x, (int)view_r.y, (int)view_r.width, (int)view_r.height);

    Rectangle cs = to_screen((Rectangle){0, 0, P.canvas_w, P.canvas_h});
    DrawRectangleRec(cs, BLACK);    // the game clears to BLACK

    bool can_hover = hover_preview && mouse_in_view && drag == DRAG_NONE && modal == MODAL_NONE;

    for (int i = 0; i < L.count; i++) {
        const Item *it = &L.items[i];
        if (it->kind != ITEM_IMAGE) continue;
        bool hov = can_hover && it->clickable && CheckCollisionPointRec(mc, it->area);
        draw_tex(tex_drawn(hov ? it->hover_tex : it->tex), it->area, WHITE);
    }
    for (int i = 0; i < L.count; i++) {
        const Item *it = &L.items[i];
        if (it->kind != ITEM_TEXT) continue;
        draw_label(label_str(it), label_rect(it));
    }

    // canvas edge + center lines
    DrawRectangleLinesEx((Rectangle){cs.x - 1, cs.y - 1, cs.width + 2, cs.height + 2}, 1, (Color){120, 120, 130, 255});
    if (show_guides) {
        Color g = (Color){255, 255, 255, 28};
        DrawLineV((Vector2){cs.x + cs.width / 2, cs.y}, (Vector2){cs.x + cs.width / 2, cs.y + cs.height}, g);
        DrawLineV((Vector2){cs.x, cs.y + cs.height / 2}, (Vector2){cs.x + cs.width, cs.y + cs.height / 2}, g);
        for (int i = 0; i < L.count; i++) {
            const Item *it = &L.items[i];
            if (it->kind == ITEM_IMAGE && it->clickable && i != sel)
                DrawRectangleLinesEx(to_screen(it->area), 1, (Color){0, 220, 255, 90});
        }
    }

    // hovered item
    if (mouse_in_view && drag == DRAG_NONE && modal == MODAL_NONE) {
        int h = hit_test(mc, false);
        if (h >= 0 && h != sel) DrawRectangleLinesEx(to_screen(hit_rect(&L.items[h])), 1, (Color){255, 255, 255, 120});
    }

    if (sel >= 0 && sel < L.count) {
        const Item *it = &L.items[sel];
        Rectangle a = to_screen(it->area);
        Color c = it->locked ? ORANGE : (Color){255, 220, 0, 255};
        if (it->kind == ITEM_TEXT) {
            DrawRectangleLinesEx(a, 1, (Color){230, 90, 255, 200});           // area it centres in
            DrawRectangleLinesEx(to_screen(label_rect(it)), 2, c);            // what's actually drawn
        } else {
            DrawRectangleLinesEx(a, 2, c);
        }
        if (!it->locked) {
            float hx[3] = {a.x, a.x + a.width / 2, a.x + a.width};
            float hy[3] = {a.y, a.y + a.height / 2, a.y + a.height};
            int order[8][2] = {{0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1}};
            for (int k = 0; k < 8; k++) {
                Rectangle hr = {hx[order[k][0]] - 5, hy[order[k][1]] - 5, 10, 10};
                DrawRectangleRec(hr, WHITE);
                DrawRectangleLinesEx(hr, 1, BLACK);
            }
        }
    }

    if (guide_x_on) { float sx = pan.x + guide_x * zoom; DrawLineEx((Vector2){sx, view_r.y}, (Vector2){sx, view_r.y + view_r.height}, 1, MAGENTA); }
    if (guide_y_on) { float sy = pan.y + guide_y * zoom; DrawLineEx((Vector2){view_r.x, sy}, (Vector2){view_r.x + view_r.width, sy}, 1, MAGENTA); }

    EndScissorMode();

    if (!have_screen) text("No screen open", view_r.x + 20, view_r.y + 20, RAYWHITE);
}

static int handle_at(Vector2 m) {
    if (sel < 0 || sel >= L.count || L.items[sel].locked) return -1;
    Rectangle a = to_screen(L.items[sel].area);
    float hx[3] = {a.x, a.x + a.width / 2, a.x + a.width};
    float hy[3] = {a.y, a.y + a.height / 2, a.y + a.height};
    int order[8][2] = {{0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1}};
    for (int k = 0; k < 8; k++) {
        Rectangle hr = {hx[order[k][0]] - 7, hy[order[k][1]] - 7, 14, 14};
        if (CheckCollisionPointRec(m, hr)) return k;
    }
    return -1;
}

// Pull a moving rect's left/centre/right (top/middle/bottom) onto the canvas
// edges and centre, and onto other items' edges, when it gets close.
static void smart_snap(Rectangle *r) {
    guide_x_on = guide_y_on = false;
    if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT) || !snap_on) return;
    float thr = 6.0f / zoom;
    float best_dx = thr + 1, best_dy = thr + 1, gx = 0, gy = 0;

    float mine_x[3] = {r->x, r->x + r->width / 2, r->x + r->width};
    float mine_y[3] = {r->y, r->y + r->height / 2, r->y + r->height};
    float tx[3 * (MAX_ITEMS + 1)], ty[3 * (MAX_ITEMS + 1)];
    int nt = 0;
    tx[nt] = 0; ty[nt++] = 0;
    tx[nt] = P.canvas_w / 2; ty[nt++] = P.canvas_h / 2;
    tx[nt] = P.canvas_w; ty[nt++] = P.canvas_h;
    for (int i = 0; i < L.count; i++) {
        if (i == sel || linked[i]) continue;
        Rectangle o = L.items[i].area;
        if (rect_eq(o, (Rectangle){0, 0, P.canvas_w, P.canvas_h})) continue;
        tx[nt] = o.x;               ty[nt++] = o.y;
        tx[nt] = o.x + o.width / 2; ty[nt++] = o.y + o.height / 2;
        tx[nt] = o.x + o.width;     ty[nt++] = o.y + o.height;
    }
    for (int k = 0; k < 3; k++)
        for (int t = 0; t < nt; t++) {
            float dx = tx[t] - mine_x[k], dy = ty[t] - mine_y[k];
            if (fabsf(dx) < fabsf(best_dx)) { best_dx = dx; gx = tx[t]; }
            if (fabsf(dy) < fabsf(best_dy)) { best_dy = dy; gy = ty[t]; }
        }
    if (fabsf(best_dx) <= thr) { r->x += best_dx; guide_x_on = true; guide_x = gx; }
    if (fabsf(best_dy) <= thr) { r->y += best_dy; guide_y_on = true; guide_y = gy; }
}

static void canvas_input(Vector2 m, Vector2 mc, bool in_view) {
    if (modal != MODAL_NONE) { if (drag != DRAG_BROWSER) drag = DRAG_NONE; return; }

    // zoom around the mouse
    float wheel = GetMouseWheelMove();
    if (in_view && wheel != 0 && drag != DRAG_BROWSER) {
        float nz = zoom * (wheel > 0 ? 1.15f : 1.0f / 1.15f);
        if (nz < 0.05f) nz = 0.05f;
        if (nz > 8.0f) nz = 8.0f;
        pan.x = m.x - mc.x * nz;
        pan.y = m.y - mc.y * nz;
        zoom = nz;
    }

    bool space = IsKeyDown(KEY_SPACE) && active_field == 0;

    if (drag == DRAG_NONE && in_view) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) || (space && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
            drag = DRAG_PAN;
            drag_start_mouse = m;
            drag_start_rect = (Rectangle){pan.x, pan.y, 0, 0};
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            active_field = 0;
            int h = handle_at(m);
            if (h >= 0) {
                drag = DRAG_RESIZE;
                drag_handle = h;
            } else {
                int hit = hit_test(mc, false);
                sel = hit;
                if (hit >= 0) drag = DRAG_MOVE;
            }
            if (drag != DRAG_NONE) {
                drag_start_mouse = mc;
                drag_start_rect = L.items[sel].area;
                drag_pushed = false;
                collect_links(sel);
            }
        }
    }

    if (drag == DRAG_PAN) {
        pan.x = drag_start_rect.x + (m.x - drag_start_mouse.x);
        pan.y = drag_start_rect.y + (m.y - drag_start_mouse.y);
        if (!IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) drag = DRAG_NONE;
        return;
    }

    if (drag == DRAG_MOVE || drag == DRAG_RESIZE) {
        float dx = mc.x - drag_start_mouse.x, dy = mc.y - drag_start_mouse.y;
        if (!drag_pushed && (fabsf(dx) * zoom > 2 || fabsf(dy) * zoom > 2)) { undo_push(); drag_pushed = true; }
        if (drag_pushed && sel >= 0) {
            Rectangle r0 = drag_start_rect, r = r0;
            if (drag == DRAG_MOVE) {
                r.x = snapv(r0.x + dx);
                r.y = snapv(r0.y + dy);
                smart_snap(&r);
            } else {
                int h = drag_handle;
                float x0 = r0.x, y0 = r0.y, x1 = r0.x + r0.width, y1 = r0.y + r0.height;
                bool L_ = h == 0 || h == 6 || h == 7, R_ = h == 2 || h == 3 || h == 4;
                bool T_ = h == 0 || h == 1 || h == 2, B_ = h == 4 || h == 5 || h == 6;
                if (L_) x0 = snapv(x0 + dx);
                if (R_) x1 = snapv(x1 + dx);
                if (T_) y0 = snapv(y0 + dy);
                if (B_) y1 = snapv(y1 + dy);
                bool corner = (L_ || R_) && (T_ || B_);
                if (IsKeyDown(KEY_LEFT_SHIFT) && corner && r0.height > 0) {
                    float aspect = r0.width / r0.height;
                    float nw = x1 - x0, nh = nw / aspect;
                    if (T_) y0 = y1 - nh; else y1 = y0 + nh;
                }
                if (x1 - x0 < 1) { if (L_) x0 = x1 - 1; else x1 = x0 + 1; }
                if (y1 - y0 < 1) { if (T_) y0 = y1 - 1; else y1 = y0 + 1; }
                r = (Rectangle){x0, y0, x1 - x0, y1 - y0};
            }
            set_area(sel, r);
        }
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            drag = DRAG_NONE;
            guide_x_on = guide_y_on = false;
            memset(linked, 0, sizeof(linked));
        }
    }

    if (drag == DRAG_BROWSER && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        drag = DRAG_NONE;
        float moved = fabsf(m.x - drag_start_mouse.x) + fabsf(m.y - drag_start_mouse.y);
        static double last_click;
        static int last_click_sel = -2;
        if (in_view && moved > 6) {
            if (!have_screen) set_status(true, "Open or create a screen first");
            else drop_texture(mc);
        } else if (moved <= 6) {
            int cur_sel = browser_tab * 100000 + browser_sel[browser_tab];
            if (GetTime() - last_click < 0.35 && last_click_sel == cur_sel && have_screen)
                drop_texture((Vector2){P.canvas_w / 2, P.canvas_h / 2});     // double click: centre of the canvas
            last_click = GetTime();
            last_click_sel = cur_sel;
        }
    }
}

static void keyboard(void) {
    if (modal != MODAL_NONE) return;
    if (active_field) {
        if (IsKeyPressed(KEY_ESCAPE)) active_field = 0;
        return;
    }
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (ctrl && IsKeyPressed(KEY_Z)) { if (shift) redo(); else undo(); }
    if (ctrl && IsKeyPressed(KEY_Y)) redo();
    if (ctrl && (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_S)) && have_screen) do_build();
    if (ctrl && IsKeyPressed(KEY_O)) { open_scroll = 0; modal = MODAL_OPEN; }
    if (!ctrl && IsKeyPressed(KEY_F)) fit_view();
    if (IsKeyPressed(KEY_ESCAPE)) sel = -1;

    if (sel < 0 || sel >= L.count || drag != DRAG_NONE) return;
    if (ctrl && IsKeyPressed(KEY_D)) duplicate(sel);
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) { undo_push(); delete_item(sel); sel = -1; return; }
    if (IsKeyPressed(KEY_PAGE_UP)) move_in_kind(sel, -1);
    if (IsKeyPressed(KEY_PAGE_DOWN)) move_in_kind(sel, +1);

    if (L.items[sel].locked) return;
    float step = shift ? 10.0f : 1.0f;
    Rectangle a = L.items[sel].area;
    bool moved = false;
    if (IsKeyPressed(KEY_LEFT)  || IsKeyPressedRepeat(KEY_LEFT))  { a.x -= step; moved = true; }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) { a.x += step; moved = true; }
    if (IsKeyPressed(KEY_UP)    || IsKeyPressedRepeat(KEY_UP))    { a.y -= step; moved = true; }
    if (IsKeyPressed(KEY_DOWN)  || IsKeyPressedRepeat(KEY_DOWN))  { a.y += step; moved = true; }
    if (moved) { undo_push(); set_area_from_field(sel, a); }
}

// ─── top bar, status, modals ─────────────────────────────────

static void top_bar(Rectangle r) {
    DrawRectangleRec(r, (Color){225, 228, 234, 255});
    float x = r.x + 8, y = r.y + 6, h = 26;
    if (GuiButton((Rectangle){x, y, 70, h}, "Open...")) { open_scroll = 0; modal = MODAL_OPEN; }
    x += 74;
    if (GuiButton((Rectangle){x, y, 64, h}, "Reload")) request(ACT_RELOAD);
    x += 72;

    const char *title = have_screen ? TextFormat("%s%s   %s", cur.name, dirty ? " *" : "", cur.is_draft ? "(draft)" : cur.source) : "(no screen)";
    text(fit(title, 330), x, y + 5, BLACK);
    x += 340;

    if (have_screen && GuiButton((Rectangle){x, y, 80, h}, "#2#Build")) do_build();
    x += 92;

    text("Lang", x, y + 5, DARKGRAY);
    x += 40;
    static char langs_text[MAX_LANGS * NAME_LEN];
    langs_text[0] = 0;
    for (int i = 0; i < P.lang_count; i++) {
        const char *l = strncmp(P.langs[i], "LANG_", 5) == 0 ? P.langs[i] + 5 : P.langs[i];
        strcat(langs_text, l);
        if (i < P.lang_count - 1) strcat(langs_text, ";");
    }
    GuiToggleGroup((Rectangle){x, y, 42, h}, langs_text, &lang);
    x += 44 * P.lang_count + 14;

    GuiCheckBox((Rectangle){x, y + 5, 16, 16}, "Hover preview", &hover_preview);
    x += 24 + text_w("Hover preview") + 14;
    GuiCheckBox((Rectangle){x, y + 5, 16, 16}, "Snap", &snap_on);
    x += 24 + text_w("Snap") + 14;
    text("grid", x, y + 5, DARKGRAY);
    field_int(5, (Rectangle){x + 34, y, 60, h}, "", &grid);
    if (grid < 0) grid = 0;
    x += 104;
    GuiCheckBox((Rectangle){x, y + 5, 16, 16}, "Guides", &show_guides);
    x += 24 + text_w("Guides") + 14;
    if (GuiButton((Rectangle){x, y, 54, h}, "Undo")) undo();
    if (GuiButton((Rectangle){x + 58, y, 54, h}, "Redo")) redo();
    x += 120;
    if (GuiButton((Rectangle){x, y, 40, h}, "Fit")) fit_view();
}

static void status_bar(Rectangle r, Vector2 mc, bool in_view) {
    DrawRectangleRec(r, (Color){225, 228, 234, 255});
    char left[256];
    snprintf(left, sizeof(left), "canvas %gx%g   zoom %d%%", P.canvas_w, P.canvas_h, (int)(zoom * 100));
    if (in_view) snprintf(left + strlen(left), sizeof(left) - strlen(left), "   mouse %.0f, %.0f", mc.x, mc.y);
    if (sel >= 0 && sel < L.count) {
        Rectangle a = L.items[sel].area;
        snprintf(left + strlen(left), sizeof(left) - strlen(left), "   sel {%g, %g, %g, %g}", a.x, a.y, a.width, a.height);
    }
    text(left, r.x + 8, r.y + 4, DARKGRAY);
    if (GetTime() < status_until) {
        float w = text_w(status);
        text(status, r.x + r.width - w - 10, r.y + 4, status_bad ? RED : (Color){0, 110, 40, 255});
    }
}

static Rectangle centered(float w, float h) {
    return (Rectangle){(GetScreenWidth() - w) / 2, (GetScreenHeight() - h) / 2, w, h};
}

static void modal_open(void) {
    float n = (float)P.screen_count;
    float list_h = fminf(fmaxf(n, 1) * 30 + 8, GetScreenHeight() - 330.0f);
    Rectangle r = centered(640, list_h + 200 + (P.warn_count ? 110 : 0));
    if (GuiWindowBox(r, "Open a screen") && have_screen) { modal = MODAL_NONE; return; }
    float x = r.x + 12, y = r.y + 34, w = r.width - 24;

    text("Screens found in the game (src/*.c) and drafts (tools/ui_editor/out/):", x, y, DARKGRAY);
    y += 24;
    Rectangle list = {x, y, w, list_h};
    if (CheckCollisionPointRec(GetMousePosition(), list)) open_scroll -= GetMouseWheelMove() * 60;
    float max_s = fmaxf(0, n * 30 + 8 - list_h);
    if (open_scroll > max_s) open_scroll = max_s;
    if (open_scroll < 0) open_scroll = 0;
    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
    for (int i = 0; i < P.screen_count; i++) {
        Screen *s = &P.screens[i];
        Rectangle b = {x, y + i * 30 - open_scroll, w, 26};
        const char *label = TextFormat("%s   -   %s", s->name, s->is_draft ? s->source + strlen("tools/ui_editor/") : s->source);
        if (GuiButton(b, label) && CheckCollisionPointRec(GetMousePosition(), list)) {
            pending_screen = i;
            modal = MODAL_NONE;
            request(ACT_OPEN);
        }
    }
    EndScissorMode();
    if (P.screen_count == 0) text("(none found)", x, y + 6, GRAY);
    y += list_h + 12;

    if (GuiButton((Rectangle){x, y, 200, 30}, "New canvas...")) { new_name_edit = true; modal = MODAL_NEW; }
    if (have_screen && GuiButton((Rectangle){x + w - 120, y, 120, 30}, "Cancel")) modal = MODAL_NONE;
    y += 42;

    text(TextFormat("%d textures declared, %d assets, canvas %gx%g, labels %gpx high", P.tex_count, asset_count, P.canvas_w, P.canvas_h, P.text_h), x, y, GRAY);
    y += 24;
    if (P.warn_count) {
        text(TextFormat("%d warnings while reading the game:", P.warn_count), x, y, RED);
        y += 20;
        for (int i = 0; i < P.warn_count && i < 4; i++) { text(fit(P.warn[i], w), x, y, (Color){150, 40, 40, 255}); y += 18; }
    }
}

static void modal_new(void) {
    Rectangle r = centered(460, 190);
    if (GuiWindowBox(r, "New canvas")) { modal = MODAL_OPEN; return; }
    float x = r.x + 12, y = r.y + 36, w = r.width - 24;
    text("Name (also the draft's file name: out/<name>.c)", x, y, DARKGRAY);
    y += 24;
    if (GuiTextBox((Rectangle){x, y, w, 30}, new_name, sizeof(new_name), new_name_edit)) new_name_edit = !new_name_edit;
    y += 44;
    bool ok = GuiButton((Rectangle){x + w - 230, y, 110, 30}, "Create") || (IsKeyPressed(KEY_ENTER) && !new_name_edit);
    if (GuiButton((Rectangle){x + w - 110, y, 110, 30}, "Cancel")) { modal = MODAL_OPEN; return; }
    if (ok) {
        if (!ident_valid(new_name)) set_status(true, "Use letters, digits and _ only");
        else {
            bool clash = false;
            for (int i = 0; i < P.screen_count; i++) if (strcmp(P.screens[i].name, new_name) == 0) clash = true;
            if (clash) set_status(true, "%s already exists, open it instead", new_name);
            else { modal = MODAL_NONE; request(ACT_NEW); }
        }
    }
    if (GetTime() < status_until && status_bad) text(status, x, r.y + r.height - 24, RED);
}

static void draw_block(const char *s, float x, float *y, float clip_top, float clip_bot, Color c) {
    const char *line = s;
    while (*line) {
        const char *nl = strchr(line, '\n');
        int len = nl ? (int)(nl - line) : (int)strlen(line);
        if (*y > clip_top - 20 && *y < clip_bot) {
            char buf[1024];
            snprintf(buf, sizeof(buf), "%.*s", len, line);
            DrawTextEx(mono_font, buf, (Vector2){x, *y}, 15, 0, c);
        }
        *y += 18;
        if (!nl) break;
        line = nl + 1;
    }
}

static void text_sized(const char *s, float x, float y, float size, Color c) {
    DrawTextEx(ui_font, s, (Vector2){x, y}, size, 1, c);
}

// The big one. Shown when the tables get copied and there are clickable buttons.
static void click_popup_draw(void) {
    int lines = 1;
    for (const char *c = click_lines; *c; c++) if (*c == '\n') lines++;
    Rectangle r = centered(760, 300 + lines * 30.0f);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));
    DrawRectangleRec(r, (Color){250, 245, 240, 255});
    DrawRectangle((int)r.x, (int)r.y, (int)r.width, 64, (Color){200, 30, 30, 255});
    DrawRectangleLinesEx(r, 3, (Color){200, 30, 30, 255});
    text_sized("AVISO", r.x + 24, r.y + 14, 40, WHITE);

    float x = r.x + 24, y = r.y + 84;
    text_sized(TextFormat("%d BOT%s CLICKABLE%s:", click_n, click_n > 1 ? "ONES" : "ÓN", click_n > 1 ? "S" : ""), x, y, 28, (Color){160, 20, 20, 255});
    y += 40;
    const char *line = click_lines;
    while (*line) {
        const char *nl = strchr(line, '\n');
        int len = nl ? (int)(nl - line) : (int)strlen(line);
        text_sized(TextFormat("  •  %.*s", len, line), x, y, 24, BLACK);
        y += 30;
        if (!nl) break;
        line = nl + 1;
    }
    y += 12;
    text_sized("HAY QUE PROGRAMAR SU LÓGICA.", x, y, 28, (Color){160, 20, 20, 255});
    y += 40;
    text_sized("El editor solo pone hover_effect. Qué hace cada clic va a mano,", x, y, 18, DARKGRAY);
    y += 22;
    text_sized("por índice. El bloque copiado lleva un esqueleto del if para rellenar.", x, y, 18, DARKGRAY);

    if (GuiButton((Rectangle){r.x + r.width - 184, r.y + r.height - 54, 160, 36}, "Entendido") ||
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
        click_popup = false;
}

static void modal_build(void) {
    if (click_popup) GuiLock();
    Rectangle r = centered(fminf(1100, GetScreenWidth() - 40.0f), GetScreenHeight() - 60.0f);
    if (GuiWindowBox(r, "Build") && !click_popup) { modal = MODAL_NONE; return; }
    float x = r.x + 12, y = r.y + 34, w = r.width - 24;

    if (GuiButton((Rectangle){x, y, 210, 28}, "Copy block 1: textures")) { SetClipboardText(build_tex); set_status(false, "Textures block copied"); }
    if (GuiButton((Rectangle){x + 216, y, 200, 28}, "Copy block 2: tables")) {
        SetClipboardText(build_tables);
        set_status(false, "Tables block copied");
        if (click_n) click_popup = true;
    }
    if (GuiButton((Rectangle){x + w - 100, y, 100, 28}, "Close") && !click_popup) { modal = MODAL_NONE; return; }
    y += 34;
    text(fit(TextFormat("Saved draft: %s   (also printed to the terminal)", build_path), w), x, y, DARKGRAY);
    y += 22;
    if (GetTime() < status_until) text(status, x, y, (Color){0, 110, 40, 255});
    y += 22;

    if (click_n) {
        Rectangle banner = {x, y, w, 52};
        DrawRectangleRec(banner, (Color){200, 30, 30, 255});
        text_sized(fit(TextFormat("AVISO: %s", click_line), w - 20), x + 10, y + 4, 20, WHITE);
        text_sized(TextFormat("%s CLICKABLE%s. HAY QUE PROGRAMAR SU LÓGICA.", click_n > 1 ? "SON" : "ES", click_n > 1 ? "S" : ""), x + 10, y + 27, 20, WHITE);
        y += 60;
    }

    Rectangle area = {x, y, w, r.y + r.height - y - 10};
    DrawRectangleRec(area, (Color){30, 31, 36, 255});
    if (!click_popup && CheckCollisionPointRec(GetMousePosition(), area)) build_scroll -= GetMouseWheelMove() * 54;
    if (build_scroll < 0) build_scroll = 0;

    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    float ty = area.y + 10 - build_scroll;
    if (build_warn[0]) { draw_block(build_warn, area.x + 10, &ty, area.y, area.y + area.height, (Color){255, 120, 110, 255}); ty += 10; }
    draw_block("// ===== BLOCK 1: textures =====", area.x + 10, &ty, area.y, area.y + area.height, (Color){120, 200, 255, 255});
    draw_block(build_tex, area.x + 10, &ty, area.y, area.y + area.height, (Color){220, 220, 220, 255});
    ty += 18;
    draw_block("// ===== BLOCK 2: tables =====", area.x + 10, &ty, area.y, area.y + area.height, (Color){120, 200, 255, 255});
    draw_block(build_tables, area.x + 10, &ty, area.y, area.y + area.height, (Color){220, 220, 220, 255});
    EndScissorMode();
    float content = ty + build_scroll - area.y;
    if (build_scroll > content - area.height + 20) build_scroll = fmaxf(0, content - area.height + 20);

    if (click_popup) { GuiUnlock(); click_popup_draw(); }
}

static void modal_confirm(void) {
    Rectangle r = centered(440, 150);
    if (GuiWindowBox(r, "Unsaved changes")) { modal = MODAL_NONE; pending = ACT_NONE; return; }
    text("This screen has changes that haven't been built.", r.x + 12, r.y + 40, BLACK);
    text("Build saves them as a draft in out/.", r.x + 12, r.y + 60, DARKGRAY);
    float y = r.y + r.height - 42;
    if (GuiButton((Rectangle){r.x + 12, y, 130, 30}, "Build first")) { modal = MODAL_NONE; pending = ACT_NONE; do_build(); return; }
    if (GuiButton((Rectangle){r.x + 150, y, 150, 30}, "Discard changes")) {
        Action a = pending;
        modal = MODAL_NONE;
        dirty = false;
        if (a == ACT_NEW) { new_screen(new_name); }
        else request(a);
        return;
    }
    if (GuiButton((Rectangle){r.x + r.width - 100, y, 88, 30}, "Cancel")) { modal = MODAL_NONE; pending = ACT_NONE; }
}

// ─── test script (UIED_SCRIPT=file) ──────────────────────────
// One command per line: "<frame> <cmd> [args]". Injects real input events via
// raylib's automation API, so it exercises the same code a person would.
//   move X Y | down | up | mdown | mup | key CODE | keyup CODE | wheel D
//   shot FILE | dump | quit

// raylib keeps this enum private to rcore.c; values copied from there
enum { EV_KEY_UP = 1, EV_KEY_DOWN = 2, EV_MOUSE_BUTTON_UP = 5, EV_MOUSE_BUTTON_DOWN = 6,
       EV_MOUSE_POSITION = 7, EV_MOUSE_WHEEL_MOTION = 8 };

static FILE *script;
static char  script_line[256];
static bool  script_have;

static void dump_state(int frame) {
    printf("[frame %d] screen=%s dirty=%d sel=%d tab=%d modal=%d zoom=%.4f pan=%.1f,%.1f items=%d\n",
           frame, have_screen ? cur.name : "-", dirty, sel, browser_tab, modal, zoom, pan.x, pan.y, L.count);
    for (int i = 0; i < L.count; i++) {
        const Item *it = &L.items[i];
        printf("   %c%d {%g,%g,%g,%g}", it->kind == ITEM_IMAGE ? 'E' : 'T', layout_index_of_kind(&L, i),
               it->area.x, it->area.y, it->area.width, it->area.height);
        if (it->kind == ITEM_IMAGE) printf(" %s%s%s\n", P.tex[it->tex].name, it->clickable ? " click->" : "", it->clickable ? P.tex[it->hover_tex].name : "");
        else printf(" %s \"%s\"\n", it->text >= 0 ? P.texts[it->text].name : "-", text_str(&P, it->text, lang));
    }
    fflush(stdout);
}

static void script_step(int frame) {
    while (script) {
        if (!script_have) {
            if (!fgets(script_line, sizeof(script_line), script)) { fclose(script); script = NULL; return; }
            if (script_line[0] == '#' || script_line[0] == '\n') continue;
            script_have = true;
        }
        int f = 0; char cmd[32] = ""; char arg[200] = ""; int a = 0, b = 0;
        sscanf(script_line, "%d %31s %199[^\n]", &f, cmd, arg);
        if (f > frame) return;
        script_have = false;
        sscanf(arg, "%d %d", &a, &b);
        AutomationEvent ev = {0};
        ev.frame = frame;
        if      (!strcmp(cmd, "move"))  { ev.type = EV_MOUSE_POSITION; ev.params[0] = a; ev.params[1] = b; }
        else if (!strcmp(cmd, "down"))  { ev.type = EV_MOUSE_BUTTON_DOWN; ev.params[0] = MOUSE_BUTTON_LEFT; }
        else if (!strcmp(cmd, "up"))    { ev.type = EV_MOUSE_BUTTON_UP;   ev.params[0] = MOUSE_BUTTON_LEFT; }
        else if (!strcmp(cmd, "mdown")) { ev.type = EV_MOUSE_BUTTON_DOWN; ev.params[0] = MOUSE_BUTTON_MIDDLE; }
        else if (!strcmp(cmd, "mup"))   { ev.type = EV_MOUSE_BUTTON_UP;   ev.params[0] = MOUSE_BUTTON_MIDDLE; }
        else if (!strcmp(cmd, "key"))   { ev.type = EV_KEY_DOWN; ev.params[0] = a; }
        else if (!strcmp(cmd, "keyup")) { ev.type = EV_KEY_UP;   ev.params[0] = a; }
        else if (!strcmp(cmd, "wheel")) { ev.type = EV_MOUSE_WHEEL_MOTION; ev.params[1] = a; }
        else if (!strcmp(cmd, "dump"))  { dump_state(frame); continue; }
        else if (!strcmp(cmd, "quit"))  { quit = true; continue; }
        else if (!strcmp(cmd, "shot"))  {
            Image shot = LoadImageFromScreen();   // the previous frame's back buffer is what's on screen
            ExportImage(shot, arg);
            UnloadImage(shot);
            continue;
        } else { printf("script: unknown command %s\n", cmd); continue; }
        PlayAutomationEvent(ev);
    }
}

// ─── main ────────────────────────────────────────────────────

static bool is_root(const char *dir) {
    char p[PATH_LEN * 2];
    snprintf(p, sizeof(p), "%s/include/texture.h", dir);
    return FileExists(p);
}

int main(int argc, char **argv) {
    // root: first non-flag argument, else look around
    const char *root = NULL;
    int first_flag = 1;
    if (argc > 1 && argv[1][0] != '-') { root = argv[1]; first_flag = 2; }
    static char guess[PATH_LEN * 2];
    if (!root) {
        const char *tries[] = {".", "../..", "../../.."};
        for (int i = 0; i < 3 && !root; i++) if (is_root(tries[i])) root = tries[i];
        if (!root) {
            snprintf(guess, sizeof(guess), "%s../../..", GetApplicationDirectory());
            if (is_root(guess)) root = guess;
        }
    }
    if (!root || !is_root(root)) {
        fprintf(stderr, "usage: ui_editor [path/to/RPG] [--list | --build SCREEN | --selftest]\n"
                        "couldn't find include/texture.h in %s\n", root ? root : "., ../.. or next to the binary");
        return 1;
    }

    int cli = cli_run(root, argc - first_flag, argv + first_flag);
    if (cli >= 0) return cli;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1600, 920, "RPG ui_editor");
    SetWindowMinSize(1200, 700);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    if (!project_load(&P, root)) { CloseWindow(); return 1; }
    scan_assets();
    load_game_font();

    const char *ui_paths[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    const char *mono_paths[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", "/usr/share/fonts/TTF/DejaVuSansMono.ttf"};
    ui_font = mono_font = GetFontDefault();
    for (int i = 0; i < 2; i++) if (FileExists(ui_paths[i])) { ui_font = LoadFontEx(ui_paths[i], FONT_SIZE * 2, NULL, 256); break; }
    for (int i = 0; i < 2; i++) if (FileExists(mono_paths[i])) { mono_font = LoadFontEx(mono_paths[i], 30, NULL, 256); break; }
    SetTextureFilter(ui_font.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(mono_font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(ui_font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, FONT_SIZE);

    view_r = (Rectangle){PANEL_W, TOP_H, 1600 - 2 * PANEL_W, 920 - TOP_H - STATUS_H};
    fit_view();

    // Test hooks: UIED_OPEN=screen  UIED_SELECT=item  UIED_LANG=n  UIED_SHOT=out.png
    const char *t_open = getenv("UIED_OPEN"), *t_sel = getenv("UIED_SELECT");
    const char *t_lang = getenv("UIED_LANG"), *t_shot = getenv("UIED_SHOT");
    if (t_open) { int s = find_screen_by(t_open, false); if (s < 0) s = find_screen_by(t_open, true); if (s >= 0) open_screen(s); }
    if (t_sel) sel = atoi(t_sel);
    if (t_lang) lang = atoi(t_lang);
    int frames = 0;
    const char *t_script = getenv("UIED_SCRIPT");
    if (t_script) script = fopen(t_script, "r");
    int script_frame = 0;

    while (!quit) {
        if (WindowShouldClose()) request(ACT_QUIT);
        if (t_script) script_step(script_frame++);

        int sw = GetScreenWidth(), sh = GetScreenHeight();
        Rectangle new_view = {PANEL_W, TOP_H, sw - 2.0f * PANEL_W, sh - (float)TOP_H - STATUS_H};
        if (new_view.width != view_r.width || new_view.height != view_r.height) { view_r = new_view; fit_view(); }

        Vector2 m = GetMousePosition();
        Vector2 mc = to_canvas(m);
        bool in_view = CheckCollisionPointRec(m, view_r);

        // keyboard() first: it ignores everything while a modal is up, so the
        // Esc that closes a modal doesn't also deselect
        keyboard();
        if (modal != MODAL_NONE && IsKeyPressed(KEY_ESCAPE)) {
            if (modal == MODAL_BUILD && click_popup) { /* the popup handles its own Esc */ }
            else if (modal == MODAL_BUILD || (modal == MODAL_OPEN && have_screen)) modal = MODAL_NONE;
            else if (modal == MODAL_NEW) modal = MODAL_OPEN;
            else if (modal == MODAL_CONFIRM) { modal = MODAL_NONE; pending = ACT_NONE; }
        }

        BeginDrawing();
        ClearBackground((Color){200, 203, 210, 255});

        canvas_draw(mc, in_view);

        if (modal != MODAL_NONE) GuiLock();
        left_panel((Rectangle){0, TOP_H, PANEL_W, sh - (float)TOP_H - STATUS_H});
        right_panel((Rectangle){sw - (float)PANEL_W, TOP_H, PANEL_W, sh - (float)TOP_H - STATUS_H});
        top_bar((Rectangle){0, 0, (float)sw, TOP_H});
        GuiUnlock();

        // after the panels: a click that ends a text edit commits it to the
        // item that was selected, before the canvas gets to change selection
        canvas_input(m, mc, in_view);

        if (drag == DRAG_BROWSER) {
            int b = browser_sel[browser_tab];
            if (b >= 0) {
                const char *nm = browser_tab == 0 ? P.tex[b].name : browser_tab == 1 ? GetFileName(assets[b].path) : P.texts[b].name;
                const char *what = browser_tab == 2 ? "label" : "image";
                text(TextFormat("+ %s (%s)", nm, what), m.x + 14, m.y + 10, in_view ? YELLOW : GRAY);
            }
        }

        status_bar((Rectangle){0, sh - (float)STATUS_H, (float)sw, STATUS_H}, mc, in_view);

        if (modal != MODAL_NONE) {
            DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.45f));
            switch (modal) {
                case MODAL_OPEN:    modal_open(); break;
                case MODAL_NEW:     modal_new(); break;
                case MODAL_BUILD:   modal_build(); break;
                case MODAL_CONFIRM: modal_confirm(); break;
                default: break;
            }
        }
        EndDrawing();

        if (t_shot && ++frames == 20) {
            Image shot = LoadImageFromScreen();
            ExportImage(shot, t_shot);
            UnloadImage(shot);
            quit = true;
        }

        if (pending_field) {
            active_field = pending_field;
            snprintf(field_buf, sizeof(field_buf), "%s", pending_buf);
            pending_field = 0;
        }
    }

    CloseWindow();
    return 0;
}
