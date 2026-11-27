#ifndef UIED_MODEL_H
#define UIED_MODEL_H

// Everything the editor knows, kept apart from the drawing code so the parser
// and the exporter can be run (and tested) without opening a window.
//
// The editor NEVER writes into the game. It reads:
//   include/texture.h   -> enum TextureId          (ids, in order)
//   src/texture.c       -> texture_defs[]           (path, frame size, smooth)
//   include/text.h      -> enum Language, enum TextId
//   src/text.c          -> text_defs[] (the strings), font_path, font_charset
//   include/defines.h   -> CANVAS_WIDTH/HEIGHT, MENU_TEXT_HEIGHT, TEXT_OUTLINE, FONT_BAKE_SIZE
//   src/*.c             -> CanvasElement / CanvasElementText tables, per function
// and writes only into tools/ui_editor/out/.

#include "raylib.h"
#include <stdbool.h>

#define MAX_TEX      2048
#define MAX_ITEMS    256
#define MAX_LANGS    8
#define MAX_SCREENS  64
#define NAME_LEN     96
#define PATH_LEN     512
#define MAX_WARN     64
#define MAX_TEXTS    512
#define STR_LEN      256

typedef struct TexEntry {
    char name[NAME_LEN];   // TEX_MAINMENU_BG
    char path[PATH_LEN];   // assets/Menus/... (relative to the repo root), "" if unknown
    int  frame_w, frame_h, off_x, off_y;
    bool smooth;
    bool in_enum;          // listed in enum TextureId
    bool has_def;          // has an entry in texture_defs[]
    bool is_new;           // added in the editor (or from a draft), not in the game yet
    // runtime, not part of the model proper
    Texture2D tex;
    bool load_tried;
} TexEntry;

// One TXT_ id and its string in every language. "" = not translated (the game uses LANG_ES).
typedef struct TextEntry {
    char name[NAME_LEN];             // TXT_PLAY
    char str[MAX_LANGS][STR_LEN];
} TextEntry;

typedef enum { ITEM_IMAGE, ITEM_TEXT } ItemKind;

typedef struct Item {
    ItemKind  kind;
    Rectangle area;
    // ITEM_IMAGE  -> CanvasElement
    int  tex;              // registry index
    bool clickable;        // hover_effect
    int  hover_tex;
    // ITEM_TEXT   -> CanvasElementText
    int  text;             // texts index, -1 = not set
    // editor only, never exported
    bool locked;
} Item;

typedef struct Screen {
    char name[NAME_LEN];          // function name, or draft name
    char source[PATH_LEN];        // file it was read from
    bool is_draft;                // came from out/, not from the game
    char elem_table[NAME_LEN];    // UIElements
    char text_table[NAME_LEN];    // UITexts
    int  elem_line, text_line;    // where the tables are in the source (0 = none)
} Screen;

typedef struct Project {
    char root[PATH_LEN];          // repo root
    float canvas_w, canvas_h, text_h, text_outline, font_bake_size;

    char langs[MAX_LANGS][NAME_LEN];
    int  lang_count;

    TextEntry texts[MAX_TEXTS];
    int  text_count;
    char font_path[PATH_LEN];     // relative to the repo root, "" if not found
    char font_charset[STR_LEN * 4];

    TexEntry tex[MAX_TEX];
    int  tex_count;
    int  tex_none;                // index of TEX_NONE

    Screen screens[MAX_SCREENS];
    int  screen_count;

    char warn[MAX_WARN][256];
    int  warn_count;
} Project;

// The thing being edited: one screen's worth of items.
typedef struct Layout {
    Item items[MAX_ITEMS];
    int  count;
} Layout;

// ─── parse.c ───
bool project_load(Project *p, const char *root);               // false = fatal (can't find the game)
bool layout_load(Project *p, int screen, Layout *out);          // tables of one screen
int  tex_find(const Project *p, const char *name);              // -1 if missing
int  text_find(const Project *p, const char *name);             // -1 if missing
const char *text_str(const Project *p, int text, int lang);     // falls back to index 0, like text_get()
int  tex_find_path(const Project *p, const char *path);
int  tex_add_new(Project *p, const char *path, int w, int h);   // from an asset png
void tex_name_from_path(const char *path, char *out, int size);
bool ident_valid(const char *s);
void project_warn(Project *p, const char *fmt, ...);

// ─── export.c ───
// Two blocks, as asked: textures to paste into the game, and the tables.
// Both strings are malloc'd; free them.
void export_blocks(const Project *p, const Screen *s, const Layout *l,
                   char **textures_block, char **tables_block, char **warnings);
int  export_clickables(const Project *p, const Layout *l, const char *elem_name, char *out, int size, bool multiline);
void export_button_label(const Project *p, const Layout *l, int item, char *out, int size);
bool export_write_draft(const Project *p, const Screen *s, const char *textures_block,
                        const char *tables_block, char *out_path, int out_size);

// ─── small helpers shared by both ───
int  layout_index_of_kind(const Layout *l, int item);   // UIElements[n] / UITexts[n]

#endif
