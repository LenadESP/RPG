// Reads the game's source. Read-only: nothing in here opens a file for writing.
//
// It's a tiny C tokenizer plus pattern matching, not a C parser. It understands
// exactly the shapes the game uses today:
//
//   typedef enum { A, B, ..., X_COUNT } Name;
//   [TEX_X] = { "path", fw, fh, ox, oy [, smooth] },
//   static const CanvasElement     NAME[] = { [i] = {TEX, {x,y,w,h} [, bool, TEX]}, ... };
//   static const CanvasElementText NAME[] = { [i] = {{x,y,w,h}, TXT_X, TEXT_STYLE}, ... };
//   static const CanvasWidget      NAME[] = { [i] = {WIDGET_X, {x,y,w,h}, SETTING_X [, OPTION]}, ... };
//   [TXT_X] = { [LANG] = "string", ... },                     (src/text.c)
//   static const char *font_path = "assets/...";              (src/text.c)
//
// If a table uses something it can't read (an expression, a macro call, ...), it
// says so in the warnings and refuses that table instead of guessing.

#include "model.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ─── tokens ──────────────────────────────────────────────────

typedef enum { T_EOF, T_IDENT, T_NUM, T_STR, T_PUNCT } TokKind;

typedef struct {
    TokKind kind;
    char    text[PATH_LEN];
    int     line;
} Tok;

typedef struct {
    Tok *t;
    int  n, cap;
} Toks;

static void toks_push(Toks *ts, TokKind k, const char *s, int len, int line) {
    if (ts->n == ts->cap) {
        ts->cap = ts->cap ? ts->cap * 2 : 1024;
        ts->t = realloc(ts->t, sizeof(Tok) * ts->cap);
    }
    Tok *t = &ts->t[ts->n++];
    t->kind = k;
    t->line = line;
    if (len >= (int)sizeof(t->text)) len = sizeof(t->text) - 1;
    memcpy(t->text, s, len);
    t->text[len] = 0;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    size_t got = fread(buf, 1, size, f);
    buf[got] = 0;
    fclose(f);
    return buf;
}

// Comments, preprocessor lines and char literals are dropped.
static bool tokenize(const char *path, Toks *ts) {
    char *src = read_file(path);
    if (!src) return false;
    ts->n = 0;
    int line = 1;
    bool line_start = true;
    const char *c = src;
    while (*c) {
        if (*c == '\n') { line++; line_start = true; c++; continue; }
        if (isspace((unsigned char)*c)) { c++; continue; }
        if (c[0] == '/' && c[1] == '/') { while (*c && *c != '\n') c++; continue; }
        if (c[0] == '/' && c[1] == '*') {
            c += 2;
            while (*c && !(c[0] == '*' && c[1] == '/')) { if (*c == '\n') line++; c++; }
            if (*c) c += 2;
            continue;
        }
        if (*c == '#' && line_start) {   // preprocessor line, with \ continuations
            while (*c && *c != '\n') { if (c[0] == '\\' && c[1] == '\n') { c++; line++; } c++; }
            continue;
        }
        line_start = false;
        if (*c == '"') {
            const char *s = ++c;
            while (*c && *c != '"') { if (*c == '\\' && c[1]) c++; c++; }
            toks_push(ts, T_STR, s, (int)(c - s), line);
            if (*c) c++;
            continue;
        }
        if (*c == '\'') {
            c++;
            while (*c && *c != '\'') { if (*c == '\\' && c[1]) c++; c++; }
            if (*c) c++;
            continue;
        }
        if (isalpha((unsigned char)*c) || *c == '_') {
            const char *s = c;
            while (isalnum((unsigned char)*c) || *c == '_') c++;
            toks_push(ts, T_IDENT, s, (int)(c - s), line);
            continue;
        }
        if (isdigit((unsigned char)*c) || (*c == '.' && isdigit((unsigned char)c[1]))) {
            const char *s = c;
            while (isalnum((unsigned char)*c) || *c == '.' ||
                   ((*c == '+' || *c == '-') && (c[-1] == 'e' || c[-1] == 'E'))) c++;
            toks_push(ts, T_NUM, s, (int)(c - s), line);
            continue;
        }
        toks_push(ts, T_PUNCT, c, 1, line);
        c++;
    }
    toks_push(ts, T_EOF, "", 0, line);
    free(src);
    return true;
}

static bool is_p(const Tok *t, char ch) { return t->kind == T_PUNCT && t->text[0] == ch; }
static bool is_id(const Tok *t, const char *s) { return t->kind == T_IDENT && strcmp(t->text, s) == 0; }

// ─── project helpers ─────────────────────────────────────────

void project_warn(Project *p, const char *fmt, ...) {
    if (p->warn_count >= MAX_WARN) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(p->warn[p->warn_count++], sizeof(p->warn[0]), fmt, ap);
    va_end(ap);
    fprintf(stderr, "ui_editor: %s\n", p->warn[p->warn_count - 1]);
}

int tex_find(const Project *p, const char *name) {
    for (int i = 0; i < p->tex_count; i++)
        if (strcmp(p->tex[i].name, name) == 0) return i;
    return -1;
}

int text_find(const Project *p, const char *name) {
    for (int i = 0; i < p->text_count; i++)
        if (strcmp(p->texts[i].name, name) == 0) return i;
    return -1;
}

const char *text_str(const Project *p, int text, int lang) {
    if (text < 0 || text >= p->text_count) return "";
    const TextEntry *t = &p->texts[text];
    if (lang >= 0 && lang < MAX_LANGS && t->str[lang][0]) return t->str[lang];
    return t->str[0];
}

int tex_find_path(const Project *p, const char *path) {
    for (int i = 0; i < p->tex_count; i++)
        if (p->tex[i].path[0] && strcmp(p->tex[i].path, path) == 0) return i;
    return -1;
}

static int tex_get_or_add(Project *p, const char *name) {
    int i = tex_find(p, name);
    if (i >= 0) return i;
    if (p->tex_count >= MAX_TEX) return -1;
    i = p->tex_count++;
    memset(&p->tex[i], 0, sizeof(p->tex[i]));
    snprintf(p->tex[i].name, NAME_LEN, "%s", name);
    return i;
}

bool ident_valid(const char *s) {
    if (!s[0] || !(isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
    for (; *s; s++) if (!(isalnum((unsigned char)*s) || *s == '_')) return false;
    return true;
}

// assets/Menus/Settings/slider_knob.png -> TEX_MENUS_SETTINGS_SLIDER_KNOB
void tex_name_from_path(const char *path, char *out, int size) {
    const char *s = path;
    if (strncmp(s, "assets/", 7) == 0) s += 7;
    int n = snprintf(out, size, "TEX_");
    const char *dot = strrchr(s, '.');
    bool last_us = true;
    for (; *s && s != dot && n < size - 1; s++) {
        if (isalnum((unsigned char)*s)) { out[n++] = (char)toupper((unsigned char)*s); last_us = false; }
        else if (!last_us)              { out[n++] = '_'; last_us = true; }
    }
    while (n > 4 && out[n - 1] == '_') n--;
    out[n] = 0;
}

int tex_add_new(Project *p, const char *path, int w, int h) {
    int found = tex_find_path(p, path);
    if (found >= 0) return found;
    if (p->tex_count >= MAX_TEX) return -1;

    char base[NAME_LEN], name[NAME_LEN];
    tex_name_from_path(path, base, sizeof(base));
    snprintf(name, sizeof(name), "%s", base);
    for (int k = 2; tex_find(p, name) >= 0; k++) snprintf(name, sizeof(name), "%.80s_%d", base, k);

    int i = p->tex_count++;
    TexEntry *t = &p->tex[i];
    memset(t, 0, sizeof(*t));
    snprintf(t->name, NAME_LEN, "%s", name);
    snprintf(t->path, PATH_LEN, "%s", path);
    t->frame_w = w;
    t->frame_h = h;
    t->smooth  = true;    // UI art is big; nearest-neighbour looks awful scaled down
    t->is_new  = true;
    t->has_def = true;
    return i;
}

int layout_index_of_kind(const Layout *l, int item) {
    int n = 0;
    for (int i = 0; i < item; i++) if (l->items[i].kind == l->items[item].kind) n++;
    return n;
}

// ─── enums, defines, texture_defs ────────────────────────────

// typedef enum { A, B = 3, C } Name;  -> names in order, *_COUNT skipped
static int parse_enum(const Toks *ts, const char *type_name, char names[][NAME_LEN], int max) {
    for (int i = 0; i + 2 < ts->n; i++) {
        if (!is_id(&ts->t[i], "enum")) continue;
        int j = i + 1;
        if (ts->t[j].kind == T_IDENT) j++;          // enum Tag {
        if (!is_p(&ts->t[j], '{')) continue;
        int k = j + 1, depth = 1;
        while (k < ts->n && depth) { if (is_p(&ts->t[k], '{')) depth++; if (is_p(&ts->t[k], '}')) depth--; k++; }
        if (k >= ts->n || ts->t[k].kind != T_IDENT || strcmp(ts->t[k].text, type_name) != 0) continue;

        int count = 0;
        bool expect_name = true;
        for (int m = j + 1; m < k - 1; m++) {
            const Tok *t = &ts->t[m];
            if (expect_name && t->kind == T_IDENT) {
                size_t len = strlen(t->text);
                bool is_count = len > 6 && strcmp(t->text + len - 6, "_COUNT") == 0;
                if (!is_count && count < max) snprintf(names[count++], NAME_LEN, "%.*s", NAME_LEN - 1, t->text);
                expect_name = false;
            } else if (is_p(t, ',')) {
                expect_name = true;
            }
        }
        return count;
    }
    return -1;
}

typedef struct { char name[NAME_LEN]; float value; } Define;
static Define defines[256];
static int    define_count;

static void parse_defines(const char *path) {
    char *src = read_file(path);
    if (!src) return;
    for (char *line = strtok(src, "\n"); line; line = strtok(NULL, "\n")) {
        char name[NAME_LEN], val[128];
        if (sscanf(line, " #define %95s %127s", name, val) == 2 && define_count < 256) {
            char *end;
            float v = strtof(val, &end);
            if (end != val) {
                snprintf(defines[define_count].name, NAME_LEN, "%s", name);
                defines[define_count++].value = v;
            }
        }
    }
    free(src);
}

static bool define_get(const char *name, float *out) {
    for (int i = 0; i < define_count; i++)
        if (strcmp(defines[i].name, name) == 0) { *out = defines[i].value; return true; }
    return false;
}

// a number: 576.0f, -3, .5, or a #define name
static bool read_number(const Toks *ts, int *i, float *out) {
    bool neg = false;
    if (is_p(&ts->t[*i], '-')) { neg = true; (*i)++; }
    else if (is_p(&ts->t[*i], '+')) (*i)++;
    const Tok *t = &ts->t[*i];
    if (t->kind == T_NUM) {
        *out = strtof(t->text, NULL);
    } else if (t->kind == T_IDENT && define_get(t->text, out)) {
        // fine
    } else {
        return false;
    }
    if (neg) *out = -*out;
    (*i)++;
    return true;
}

static bool expect(const Toks *ts, int *i, char ch) {
    if (!is_p(&ts->t[*i], ch)) return false;
    (*i)++;
    return true;
}

// [TEX_X] = { "path", fw, fh, ox, oy [, smooth] }   anywhere in the file
static void parse_texture_defs(Project *p, const Toks *ts, bool from_draft) {
    for (int i = 0; i + 6 < ts->n; i++) {
        int j = i;
        if (!expect(ts, &j, '[')) continue;
        if (ts->t[j].kind != T_IDENT) continue;
        const char *name = ts->t[j].text; j++;
        if (!expect(ts, &j, ']') || !expect(ts, &j, '=') || !expect(ts, &j, '{')) continue;
        if (ts->t[j].kind != T_STR) continue;
        const char *path = ts->t[j].text; j++;

        float v[4] = {0};
        bool ok = true;
        for (int k = 0; k < 4 && ok; k++) ok = expect(ts, &j, ',') && read_number(ts, &j, &v[k]);
        if (!ok) { project_warn(p, "texture def %s (line %d): couldn't read the numbers", name, ts->t[i].line); continue; }

        bool smooth = false;
        if (is_p(&ts->t[j], ',') && ts->t[j + 1].kind == T_IDENT) {
            smooth = is_id(&ts->t[j + 1], "true") || is_id(&ts->t[j + 1], "1");
        }

        int existing = tex_find(p, name);
        if (from_draft && existing >= 0 && p->tex[existing].has_def) continue;   // the game wins
        int idx = tex_get_or_add(p, name);
        if (idx < 0) continue;
        TexEntry *t = &p->tex[idx];
        snprintf(t->path, PATH_LEN, "%s", path);
        t->frame_w = (int)v[0]; t->frame_h = (int)v[1];
        t->off_x   = (int)v[2]; t->off_y   = (int)v[3];
        t->smooth  = smooth;
        t->has_def = true;
        if (from_draft && !t->in_enum) t->is_new = true;
    }
}

// "abc" "def" -> abcdef, like the compiler does
static bool read_string(const Toks *ts, int *i, char *out, int size) {
    if (ts->t[*i].kind != T_STR) return false;
    int len = 0;
    out[0] = 0;
    while (ts->t[*i].kind == T_STR) {
        len += snprintf(out + len, len < size ? size - len : 0, "%s", ts->t[*i].text);
        (*i)++;
    }
    return true;
}

// static const char *NAME = "..." ;   anywhere in the file
static bool find_string_var(const Toks *ts, const char *name, char *out, int size) {
    for (int i = 0; i + 2 < ts->n; i++) {
        if (!is_id(&ts->t[i], name) || !is_p(&ts->t[i + 1], '=')) continue;
        int j = i + 2;
        return read_string(ts, &j, out, size);
    }
    return false;
}

static int lang_index(const Project *p, const Tok *t);

// [TXT_X] = { [LANG] = "string", ... }   anywhere in the file
static void parse_text_defs(Project *p, const Toks *ts) {
    for (int i = 0; i + 6 < ts->n; i++) {
        int j = i;
        if (!expect(ts, &j, '[')) continue;
        if (ts->t[j].kind != T_IDENT) continue;
        const char *name = ts->t[j].text; j++;
        if (!expect(ts, &j, ']') || !expect(ts, &j, '=') || !expect(ts, &j, '{')) continue;
        if (!is_p(&ts->t[j], '[') && ts->t[j].kind != T_STR) continue;   // not a text def

        int idx = text_find(p, name);
        if (idx < 0) continue;   // e.g. the [TXT_COUNT][LANG_COUNT] of the declaration itself
        TextEntry *t = &p->texts[idx];

        int pos = 0;
        while (!is_p(&ts->t[j], '}') && ts->t[j].kind != T_EOF) {
            int lang = pos;
            if (is_p(&ts->t[j], '[')) {
                j++;
                lang = lang_index(p, &ts->t[j]);
                j++;
                if (!expect(ts, &j, ']') || !expect(ts, &j, '=')) break;
            }
            if (lang < 0 || lang >= MAX_LANGS || !read_string(ts, &j, t->str[lang], STR_LEN)) {
                project_warn(p, "%s (line %d): couldn't read its strings", name, ts->t[i].line);
                break;
            }
            pos = lang + 1;
            if (is_p(&ts->t[j], ',')) j++;
        }
        if (!t->str[0][0]) project_warn(p, "%s has no %s string, the game draws nothing", name, p->langs[0]);
    }
}

// ─── text styles ─────────────────────────────────────────────

int style_find(const Project *p, const char *name) {
    for (int i = 0; i < p->style_count; i++) if (strcmp(p->styles[i].name, name) == 0) return i;
    return -1;
}
int setting_find(const Project *p, const char *name) {
    for (int i = 0; i < p->setting_count; i++) if (strcmp(p->settings[i], name) == 0) return i;
    return -1;
}
int wtype_find(const Project *p, const char *name) {
    for (int i = 0; i < p->widget_type_count; i++) if (strcmp(p->widget_types[i], name) == 0) return i;
    return -1;
}

// #define COLOR_X {r, g, b, a}   from include/colors.h
typedef struct { char name[NAME_LEN]; Color c; } NamedColor;
static NamedColor colors[64];
static int color_count;

static void load_colors(const char *path) {
    color_count = 0;
    char *src = read_file(path);
    if (!src) return;
    for (char *line = strtok(src, "\n"); line && color_count < 64; line = strtok(NULL, "\n")) {
        char name[NAME_LEN]; int r, g, b, a;
        if (sscanf(line, " #define %95s {%d , %d , %d , %d}", name, &r, &g, &b, &a) == 5) {
            snprintf(colors[color_count].name, NAME_LEN, "%s", name);
            colors[color_count].c = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
            color_count++;
        }
    }
    free(src);
}

static bool read_bool(const Toks *ts, int *i, bool *b);

// [TEXT_X] = {size, centered, outline, COLOR_X}   anywhere in the file
static void parse_style_defs(Project *p, const Toks *ts) {
    for (int i = 0; i + 6 < ts->n; i++) {
        int j = i;
        if (!expect(ts, &j, '[') || ts->t[j].kind != T_IDENT) continue;
        int idx = style_find(p, ts->t[j].text);
        if (idx < 0) continue;
        j++;
        if (!expect(ts, &j, ']') || !expect(ts, &j, '=') || !expect(ts, &j, '{')) continue;
        StyleEntry *st = &p->styles[idx];
        float size; bool centered, outline;
        if (!read_number(ts, &j, &size) || !expect(ts, &j, ',') || !read_bool(ts, &j, &centered) ||
            !expect(ts, &j, ',') || !read_bool(ts, &j, &outline) || !expect(ts, &j, ',')) {
            project_warn(p, "%s (line %d): couldn't read its style", st->name, ts->t[i].line);
            continue;
        }
        st->size = size; st->centered = centered; st->outline = outline;
        if (ts->t[j].kind == T_IDENT) {
            bool found = false;
            for (int c = 0; c < color_count; c++)
                if (strcmp(colors[c].name, ts->t[j].text) == 0) { st->color = colors[c].c; found = true; }
            if (!found) project_warn(p, "%s: color %s not found in colors.h", st->name, ts->t[j].text);
        }
    }
}

// ─── canvas tables ───────────────────────────────────────────

typedef struct {
    const char *type;    // "CanvasElement" / "CanvasElementText" / "CanvasWidget"
    char name[NAME_LEN];
    char func[NAME_LEN];
    int  start;          // index of the '{' that opens the initializer
    int  line;
} TableRef;

// Finds every table in a file and the function it lives in.
static int find_tables(const Toks *ts, TableRef *out, int max) {
    int count = 0, depth = 0, parens = 0;
    char func[NAME_LEN] = "";
    char candidate[NAME_LEN] = "";

    for (int i = 0; i < ts->n; i++) {
        const Tok *t = &ts->t[i];
        // the name right before the OUTERMOST '(' at file scope. Parameters like
        // void (*cb)(void) also have ident+'(' but sit inside the parens.
        if (depth == 0 && parens == 0 && t->kind == T_IDENT && i + 1 < ts->n && is_p(&ts->t[i + 1], '('))
            snprintf(candidate, sizeof(candidate), "%.*s", NAME_LEN - 1, t->text);
        if (depth == 0 && is_p(t, '(')) parens++;
        if (depth == 0 && is_p(t, ')') && parens > 0) parens--;
        if (is_p(t, '{')) {
            if (depth == 0) {
                if (i > 0 && is_p(&ts->t[i - 1], ')')) snprintf(func, sizeof(func), "%s", candidate);
                else func[0] = 0;
            }
            depth++;
        } else if (is_p(t, '}')) {
            if (depth > 0) depth--;
            if (depth == 0) func[0] = 0;
        }

        if (t->kind == T_IDENT && (strcmp(t->text, "CanvasElement") == 0 || strcmp(t->text, "CanvasElementText") == 0 || strcmp(t->text, "CanvasWidget") == 0)) {
            int j = i + 1;
            if (ts->t[j].kind != T_IDENT) continue;
            const char *name = ts->t[j].text; j++;
            if (!expect(ts, &j, '[')) continue;
            if (ts->t[j].kind == T_NUM) j++;
            if (!expect(ts, &j, ']') || !is_p(&ts->t[j], '=') || !is_p(&ts->t[j + 1], '{')) continue;
            if (count < max) {
                TableRef *r = &out[count++];
                r->type = strcmp(t->text, "CanvasElement") == 0 ? "CanvasElement" : strcmp(t->text, "CanvasWidget") == 0 ? "CanvasWidget" : "CanvasElementText";
                snprintf(r->name, NAME_LEN, "%.*s", NAME_LEN - 1, name);
                snprintf(r->func, NAME_LEN, "%s", func);
                r->start = j + 2;   // first token after the '{'
                r->line = t->line;
            }
        }
    }
    return count;
}

static bool read_rect(const Toks *ts, int *i, Rectangle *r) {
    float v[4];
    if (!expect(ts, i, '{')) return false;
    for (int k = 0; k < 4; k++) {
        if (k && !expect(ts, i, ',')) return false;
        if (!read_number(ts, i, &v[k])) return false;
    }
    if (is_p(&ts->t[*i], ',')) (*i)++;
    if (!expect(ts, i, '}')) return false;
    *r = (Rectangle){v[0], v[1], v[2], v[3]};
    return true;
}

static bool read_bool(const Toks *ts, int *i, bool *b) {
    const Tok *t = &ts->t[*i];
    if (is_id(t, "true"))  { *b = true;  (*i)++; return true; }
    if (is_id(t, "false")) { *b = false; (*i)++; return true; }
    if (t->kind == T_NUM)  { *b = strtol(t->text, NULL, 0) != 0; (*i)++; return true; }
    return false;
}

static bool read_tex(Project *p, const Toks *ts, int *i, int *out) {
    const Tok *t = &ts->t[*i];
    if (t->kind == T_NUM && strtol(t->text, NULL, 0) == 0) { *out = p->tex_none; (*i)++; return true; }
    if (t->kind != T_IDENT) return false;
    int idx = tex_find(p, t->text);
    if (idx < 0) {
        idx = tex_get_or_add(p, t->text);
        project_warn(p, "%s is used in a table but not declared anywhere", t->text);
    }
    if (idx < 0) return false;
    *out = idx;
    (*i)++;
    return true;
}

static bool read_text(Project *p, const Toks *ts, int *i, int *out) {
    const Tok *t = &ts->t[*i];
    if (t->kind != T_IDENT) return false;
    int idx = text_find(p, t->text);
    if (idx < 0) {
        project_warn(p, "%s is used in a table but isn't in enum TextId", t->text);
        return false;
    }
    *out = idx;
    (*i)++;
    return true;
}

static int lang_index(const Project *p, const Tok *t) {
    if (t->kind == T_NUM) return (int)strtol(t->text, NULL, 0);
    for (int i = 0; i < p->lang_count; i++) if (strcmp(p->langs[i], t->text) == 0) return i;
    return -1;
}

// Reads one table into `items` (indexed by its designated [i]).
// Returns the item count, or -1 if it couldn't be read.
static int read_table(Project *p, const Toks *ts, const TableRef *r, Item *items, int max) {
    bool is_text = strcmp(r->type, "CanvasElementText") == 0;
    bool is_widget = strcmp(r->type, "CanvasWidget") == 0;
    bool used[MAX_ITEMS] = {0};
    int i = r->start, next = 0, highest = -1;

    while (!is_p(&ts->t[i], '}')) {
        int idx = next;
        if (is_p(&ts->t[i], '[')) {
            i++;
            float f;
            if (!read_number(ts, &i, &f) || !expect(ts, &i, ']') || !expect(ts, &i, '=')) goto fail;
            idx = (int)f;
        }
        if (idx < 0 || idx >= max) goto fail;
        if (used[idx]) { project_warn(p, "%s line %d: index [%d] appears twice", r->name, r->line, idx); goto fail; }

        Item it = {0};
        it.text = -1;
        it.tex = it.hover_tex = p->tex_none;

        if (!expect(ts, &i, '{')) goto fail;
        if (is_widget) {
            it.kind = ITEM_WIDGET;
            const Tok *wt = &ts->t[i];
            if (wt->kind != T_IDENT || (it.wtype = wtype_find(p, wt->text)) < 0) {
                project_warn(p, "%s isn't in enum WidgetType", wt->text); goto fail;
            }
            i++;
            if (!expect(ts, &i, ',') || !read_rect(ts, &i, &it.area) || !expect(ts, &i, ',')) goto fail;
            const Tok *st = &ts->t[i];
            if (st->kind != T_IDENT || (it.setting = setting_find(p, st->text)) < 0) {
                project_warn(p, "%s isn't in enum SettingId", st->text); goto fail;
            }
            i++;
            if (is_p(&ts->t[i], ',') && !is_p(&ts->t[i + 1], '}')) {
                i++;
                if (ts->t[i].kind != T_IDENT && ts->t[i].kind != T_NUM) goto fail;
                snprintf(it.option, NAME_LEN, "%.*s", NAME_LEN - 1, ts->t[i].text);
                i++;
            }
        } else if (!is_text) {
            it.kind = ITEM_IMAGE;
            if (!read_tex(p, ts, &i, &it.tex) || !expect(ts, &i, ',') || !read_rect(ts, &i, &it.area)) goto fail;
            if (is_p(&ts->t[i], ',') && !is_p(&ts->t[i + 1], '}')) {
                i++;
                if (!read_bool(ts, &i, &it.clickable)) goto fail;
                if (is_p(&ts->t[i], ',') && !is_p(&ts->t[i + 1], '}')) {
                    i++;
                    if (!read_tex(p, ts, &i, &it.hover_tex)) goto fail;
                }
            }
        } else {
            it.kind = ITEM_TEXT;
            if (!read_rect(ts, &i, &it.area) || !expect(ts, &i, ',') || !read_text(p, ts, &i, &it.text)) goto fail;
            if (is_p(&ts->t[i], ',') && !is_p(&ts->t[i + 1], '}')) {
                i++;
                if (ts->t[i].kind != T_IDENT || (it.style = style_find(p, ts->t[i].text)) < 0) {
                    project_warn(p, "%s isn't in enum TextStyle", ts->t[i].text); goto fail;
                }
                i++;
            }
        }
        if (is_p(&ts->t[i], ',')) i++;
        if (!expect(ts, &i, '}')) goto fail;
        if (is_p(&ts->t[i], ',')) i++;

        items[idx] = it;
        used[idx] = true;
        if (idx > highest) highest = idx;
        next = idx + 1;
    }

    for (int k = 0; k <= highest; k++) {
        if (!used[k]) {
            project_warn(p, "%s line %d: index [%d] is missing (a hole in the table)", r->name, r->line, k);
            return -1;
        }
    }
    return highest + 1;

fail:
    project_warn(p, "%s (%s, line %d): couldn't read it near line %d (\"%s\"). Not loading it, so it can't be mangled.",
                 r->name, r->func[0] ? r->func : "file scope", r->line, ts->t[i].line, ts->t[i].text);
    return -1;
}

// ─── screens ─────────────────────────────────────────────────

static void scan_screens(Project *p, const char *path, const char *display, bool is_draft, const char *draft_name) {
    Toks ts = {0};
    if (!tokenize(path, &ts)) return;
    TableRef refs[64];
    int n = find_tables(&ts, refs, 64);
    for (int k = 0; k < n; k++) {
        const char *name = is_draft ? draft_name : refs[k].func;
        if (!name[0]) continue;     // file-scope table in the game: no screen to hang it on
        int s;
        for (s = 0; s < p->screen_count; s++)
            if (p->screens[s].is_draft == is_draft && strcmp(p->screens[s].name, name) == 0) break;
        if (s == p->screen_count) {
            if (p->screen_count >= MAX_SCREENS) break;
            p->screen_count++;
            memset(&p->screens[s], 0, sizeof(Screen));
            snprintf(p->screens[s].name, NAME_LEN, "%s", name);
            snprintf(p->screens[s].source, PATH_LEN, "%s", display);
            p->screens[s].is_draft = is_draft;
        }
        Screen *sc = &p->screens[s];
        bool text = strcmp(refs[k].type, "CanvasElementText") == 0;
        bool widget = strcmp(refs[k].type, "CanvasWidget") == 0;
        char *slot = widget ? sc->widget_table : text ? sc->text_table : sc->elem_table;
        int  *line = widget ? &sc->widget_line : text ? &sc->text_line : &sc->elem_line;
        if (slot[0]) {
            project_warn(p, "%s has more than one %s table; only %s is editable", name, refs[k].type, slot);
            continue;
        }
        snprintf(slot, NAME_LEN, "%s", refs[k].name);
        *line = refs[k].line;
    }
    free(ts.t);
}

// The game keeps its files in subfolders (src/ui/text.c...). Finds one by name
// anywhere under root/sub. Falls back to root/sub/name so the warning shows a path.
static void game_file(const char *root, const char *sub, const char *name, char *out, int size) {
    char dir[PATH_LEN * 2];
    snprintf(dir, sizeof(dir), "%s/%s", root, sub);
    snprintf(out, size, "%s/%s", dir, name);
    if (!DirectoryExists(dir)) return;
    FilePathList fl = LoadDirectoryFilesEx(dir, NULL, true);
    for (unsigned i = 0; i < fl.count; i++) {
        if (strcmp(GetFileName(fl.paths[i]), name) == 0) { snprintf(out, size, "%s", fl.paths[i]); break; }
    }
    UnloadDirectoryFiles(fl);
}

bool project_load(Project *p, const char *root) {
    memset(p, 0, sizeof(*p));
    define_count = 0;
    snprintf(p->root, PATH_LEN, "%s", root);
    char path[PATH_LEN * 2];

    // defines.h
    game_file(root, "include", "defines.h", path, sizeof(path));
    parse_defines(path);
    p->canvas_w = 1672; p->canvas_h = 941; p->text_outline = 3; p->text_shadow = 2; p->font_bake_size = 120;
    if (!define_get("CANVAS_WIDTH", &p->canvas_w))      project_warn(p, "CANVAS_WIDTH not found, using 1672");
    if (!define_get("CANVAS_HEIGHT", &p->canvas_h))     project_warn(p, "CANVAS_HEIGHT not found, using 941");
    if (!define_get("TEXT_SHADOW_OFFSET", &p->text_shadow)) project_warn(p, "TEXT_SHADOW_OFFSET not found, using 2");
    if (!define_get("TEXT_OUTLINE_WIDTH", &p->text_outline))  project_warn(p, "TEXT_OUTLINE_WIDTH not found, using 3");
    if (!define_get("FONT_LOAD_SIZE", &p->font_bake_size)) project_warn(p, "FONT_LOAD_SIZE not found, using 120");

    Toks ts = {0};

    // text.h -> languages
    game_file(root, "include", "text.h", path, sizeof(path));
    if (tokenize(path, &ts)) p->lang_count = parse_enum(&ts, "Language", p->langs, MAX_LANGS);
    if (p->lang_count <= 0) {
        project_warn(p, "enum Language not found in text.h");
        p->lang_count = 1;
        snprintf(p->langs[0], NAME_LEN, "0");
    }

    // text.h -> text ids, text.c -> their strings and the font
    static char text_names[MAX_TEXTS][NAME_LEN];
    int nt = parse_enum(&ts, "TextId", text_names, MAX_TEXTS);
    if (nt < 0) { project_warn(p, "enum TextId not found in text.h"); nt = 0; }
    for (int i = 0; i < nt; i++) snprintf(p->texts[p->text_count++].name, NAME_LEN, "%.*s", NAME_LEN - 1, text_names[i]);

    static char style_names[MAX_STYLES][NAME_LEN];
    int ns = parse_enum(&ts, "TextStyle", style_names, MAX_STYLES);
    if (ns <= 0) { project_warn(p, "enum TextStyle not found in text.h"); ns = 0; }
    for (int i = 0; i < ns; i++) {
        StyleEntry *st = &p->styles[p->style_count++];
        snprintf(st->name, NAME_LEN, "%.*s", NAME_LEN - 1, style_names[i]);
        st->size = 32; st->centered = true; st->outline = false; st->color = (Color){251, 228, 168, 255};
    }
    if (p->style_count == 0) {   // something to point at, so items never index nothing
        snprintf(p->styles[0].name, NAME_LEN, "TEXT_BUTTON");
        p->styles[0].size = 48; p->styles[0].centered = true; p->styles[0].outline = true; p->styles[0].color = (Color){251, 228, 168, 255};
        p->style_count = 1;
    }

    game_file(root, "include", "colors.h", path, sizeof(path));
    load_colors(path);

    game_file(root, "include", "settings.h", path, sizeof(path));
    if (tokenize(path, &ts)) p->setting_count = parse_enum(&ts, "SettingId", p->settings, MAX_SETTINGS);
    if (p->setting_count < 0) p->setting_count = 0;
    if (!p->setting_count) project_warn(p, "enum SettingId not found in settings.h");

    game_file(root, "include", "canvas.h", path, sizeof(path));
    if (tokenize(path, &ts)) p->widget_type_count = parse_enum(&ts, "WidgetType", p->widget_types, MAX_WTYPES);
    if (p->widget_type_count < 0) p->widget_type_count = 0;
    if (!p->widget_type_count) project_warn(p, "enum WidgetType not found in canvas.h");

    game_file(root, "src", "text.c", path, sizeof(path));
    if (tokenize(path, &ts)) {
        parse_text_defs(p, &ts);
        parse_style_defs(p, &ts);
        if (!find_string_var(&ts, "font_path", p->font_path, PATH_LEN)) project_warn(p, "font_path not found in text.c");
        find_string_var(&ts, "font_characters", p->font_charset, sizeof(p->font_charset));
    } else {
        project_warn(p, "can't read text.c");
    }

    // texture.h -> ids
    static char names[MAX_TEX][NAME_LEN];
    game_file(root, "include", "texture.h", path, sizeof(path));
    if (!tokenize(path, &ts)) {
        fprintf(stderr, "ui_editor: can't read %s. Is \"%s\" the RPG repo root?\n", path, root);
        free(ts.t);
        return false;
    }
    int n = parse_enum(&ts, "TextureId", names, MAX_TEX);
    if (n <= 0) { fprintf(stderr, "ui_editor: enum TextureId not found in %s\n", path); free(ts.t); return false; }
    for (int i = 0; i < n; i++) {
        int idx = tex_get_or_add(p, names[i]);
        if (idx >= 0) p->tex[idx].in_enum = true;
    }

    // texture.c -> defs
    game_file(root, "src", "texture.c", path, sizeof(path));
    if (tokenize(path, &ts)) parse_texture_defs(p, &ts, false);
    else project_warn(p, "can't read texture.c");

    for (int i = 0; i < p->tex_count; i++) {
        if (p->tex[i].in_enum && !p->tex[i].has_def) project_warn(p, "%s is in the enum but has no texture_defs entry", p->tex[i].name);
        if (!p->tex[i].in_enum && p->tex[i].has_def) project_warn(p, "%s has a texture_defs entry but isn't in the enum", p->tex[i].name);
    }
    p->tex_none = tex_find(p, "TEX_NONE");
    if (p->tex_none < 0) { project_warn(p, "TEX_NONE not found"); p->tex_none = 0; }

    // drafts: their new textures first, so their tables resolve
    snprintf(path, sizeof(path), "%s/tools/ui_editor/out", root);
    FilePathList drafts = {0};
    if (DirectoryExists(path)) drafts = LoadDirectoryFilesEx(path, ".c", false);
    for (unsigned d = 0; d < drafts.count; d++) {
        if (tokenize(drafts.paths[d], &ts)) parse_texture_defs(p, &ts, true);
    }

    // game screens: every function in src/*.c that owns a table
    snprintf(path, sizeof(path), "%s/src", root);
    FilePathList srcs = LoadDirectoryFilesEx(path, ".c", true);
    size_t root_len = strlen(root);
    for (unsigned f = 0; f < srcs.count; f++) {
        char display[PATH_LEN];
        const char *rel = srcs.paths[f] + root_len;   // "/src/ui/text.c" -> "src/ui/text.c"
        while (*rel == '/') rel++;
        snprintf(display, sizeof(display), "%s", rel);
        scan_screens(p, srcs.paths[f], display, false, NULL);
    }
    UnloadDirectoryFiles(srcs);

    for (unsigned d = 0; d < drafts.count; d++) {
        char display[PATH_LEN];
        snprintf(display, sizeof(display), "tools/ui_editor/out/%s", GetFileName(drafts.paths[d]));
        scan_screens(p, drafts.paths[d], display, true, GetFileNameWithoutExt(drafts.paths[d]));
    }
    if (drafts.paths) UnloadDirectoryFiles(drafts);

    free(ts.t);
    return true;
}

bool layout_load(Project *p, int screen, Layout *out) {
    memset(out, 0, sizeof(*out));
    if (screen < 0 || screen >= p->screen_count) return true;   // new canvas: empty
    Screen *s = &p->screens[screen];

    char path[PATH_LEN * 2];
    snprintf(path, sizeof(path), "%s/%s", p->root, s->source);
    Toks ts = {0};
    if (!tokenize(path, &ts)) { project_warn(p, "can't read %s", s->source); return false; }

    TableRef refs[64];
    int n = find_tables(&ts, refs, 64);
    static Item elems[MAX_ITEMS], texts[MAX_ITEMS], widgets[MAX_ITEMS];
    int ne = 0, nt = 0, nw = 0;
    bool ok = true;
    for (int k = 0; k < n; k++) {
        if (!s->is_draft && strcmp(refs[k].func, s->name) != 0) continue;
        if (strcmp(refs[k].type, "CanvasElement") == 0 && strcmp(refs[k].name, s->elem_table) == 0) {
            ne = read_table(p, &ts, &refs[k], elems, MAX_ITEMS);
            if (ne < 0) { ok = false; ne = 0; }
        } else if (strcmp(refs[k].type, "CanvasElementText") == 0 && strcmp(refs[k].name, s->text_table) == 0) {
            nt = read_table(p, &ts, &refs[k], texts, MAX_ITEMS);
            if (nt < 0) { ok = false; nt = 0; }
        } else if (strcmp(refs[k].type, "CanvasWidget") == 0 && strcmp(refs[k].name, s->widget_table) == 0) {
            nw = read_table(p, &ts, &refs[k], widgets, MAX_ITEMS);
            if (nw < 0) { ok = false; nw = 0; }
        }
    }
    free(ts.t);
    if (!ok) return false;

    // Images first, then labels: that's the order the game draws them in.
    for (int i = 0; i < ne && out->count < MAX_ITEMS; i++) out->items[out->count++] = elems[i];
    for (int i = 0; i < nt && out->count < MAX_ITEMS; i++) out->items[out->count++] = texts[i];
    for (int i = 0; i < nw && out->count < MAX_ITEMS; i++) out->items[out->count++] = widgets[i];
    return true;
}
