// Turns a layout back into C, in the same shape game.c already uses, so it can
// be pasted over the old tables without reformatting anything.

#include "model.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char *s; int len, cap; } Buf;

static void bprintf(Buf *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (b->len + need + 1 > b->cap) {
        b->cap = (b->len + need + 1) * 2;
        b->s = realloc(b->s, b->cap);
    }
    va_start(ap, fmt);
    vsnprintf(b->s + b->len, need + 1, fmt, ap);
    va_end(ap);
    b->len += need;
}

static char *bdone(Buf *b) {
    if (!b->s) { b->s = malloc(1); b->s[0] = 0; }
    return b->s;
}

// 576 -> "576.0f", 523.5 -> "523.5f", 1.25 -> "1.25f"
static void fmt_num(float v, char *out, int size) {
    if (fabsf(v * 10.0f - roundf(v * 10.0f)) < 0.001f) snprintf(out, size, "%.1ff", v);
    else {
        snprintf(out, size, "%.3f", v);
        int n = (int)strlen(out);
        while (out[n - 1] == '0') out[--n] = 0;
        snprintf(out + n, size - n, "f");
    }
}

// The rects, right-aligned per column, like the hand-written tables.
typedef struct { char v[4][24]; } RectText;

static void rect_cols(const Layout *l, ItemKind kind, RectText *rt, int widths[4]) {
    memset(widths, 0, sizeof(int) * 4);
    for (int i = 0; i < l->count; i++) {
        const Rectangle *a = &l->items[i].area;
        float v[4] = {a->x, a->y, a->width, a->height};
        for (int k = 0; k < 4; k++) {
            fmt_num(v[k], rt[i].v[k], sizeof(rt[i].v[k]));
            int len = (int)strlen(rt[i].v[k]);
            if (l->items[i].kind == kind && len > widths[k]) widths[k] = len;
        }
    }
}

static void put_rect(Buf *b, const RectText *rt, const int widths[4]) {
    bprintf(b, "{%*s, %*s, %*s, %*s}", widths[0], rt->v[0], widths[1], rt->v[1], widths[2], rt->v[2], widths[3], rt->v[3]);
}

static bool used_by(const Layout *l, int tex) {
    for (int i = 0; i < l->count; i++) {
        const Item *it = &l->items[i];
        if (it->kind == ITEM_IMAGE && (it->tex == tex || (it->clickable && it->hover_tex == tex))) return true;
    }
    return false;
}

// The label sitting exactly on a button, by its id: "PLAY".
void export_button_label(const Project *p, const Layout *l, int item, char *out, int size) {
    out[0] = 0;
    for (int k = 0; k < l->count; k++) {
        const Item *t = &l->items[k];
        if (t->kind != ITEM_TEXT || t->text < 0) continue;
        const Rectangle a = t->area, b = l->items[item].area;
        if (a.x != b.x || a.y != b.y || a.width != b.width || a.height != b.height) continue;
        const char *n = p->texts[t->text].name;
        snprintf(out, size, "%s", strncmp(n, "TXT_", 4) == 0 ? n + 4 : n);
        return;
    }
}

// "UIElements[2] (JUGAR), UIElements[3] (CONFIGURACION)". One per line if multiline.
int export_clickables(const Project *p, const Layout *l, const char *elem_name, char *out, int size, bool multiline) {
    int n = 0, len = 0;
    out[0] = 0;
    for (int i = 0; i < l->count && len < size - 1; i++) {
        if (l->items[i].kind != ITEM_IMAGE || !l->items[i].clickable) continue;
        char label[NAME_LEN];
        export_button_label(p, l, i, label, sizeof(label));
        len += snprintf(out + len, size - len, "%s%s[%d]%s%s%s", n ? (multiline ? "\n" : ", ") : "",
                        elem_name, layout_index_of_kind(l, i), label[0] ? " (" : "", label, label[0] ? ")" : "");
        n++;
    }
    return n;
}

void export_blocks(const Project *p, const Screen *s, const Layout *l,
                   char **textures_block, char **tables_block, char **warnings) {
    Buf tb = {0}, rb = {0}, wb = {0};
    const char *elem_name = (s && s->elem_table[0]) ? s->elem_table : "UIElements";
    const char *text_name = (s && s->text_table[0]) ? s->text_table : "UITexts";
    const char *widget_name = (s && s->widget_table[0]) ? s->widget_table : "UIWidgets";

    // ── block 1: textures ──
    int new_ids[MAX_TEX], nn = 0;
    for (int i = 0; i < p->tex_count; i++)
        if (p->tex[i].is_new && used_by(l, i)) new_ids[nn++] = i;

    if (nn == 0) {
        bprintf(&tb, "// No new textures: everything this screen uses is already declared.\n");
    } else {
        int w = 0;
        for (int k = 0; k < nn; k++) { int len = (int)strlen(p->tex[new_ids[k]].name); if (len > w) w = len; }
        bprintf(&tb, "// texture.h : add inside enum TextureId, before TEX_COUNT\n");
        for (int k = 0; k < nn; k++) bprintf(&tb, "    %s,\n", p->tex[new_ids[k]].name);
        bprintf(&tb, "\n// texture.c : add inside texture_defs[]\n");
        for (int k = 0; k < nn; k++) {
            const TexEntry *t = &p->tex[new_ids[k]];
            char key[NAME_LEN + 4];
            snprintf(key, sizeof(key), "[%s]", t->name);
            bprintf(&tb, "    %-*s = {\"%s\", %d, %d, %d, %d, %s},\n", w + 2, key, t->path,
                    t->frame_w, t->frame_h, t->off_x, t->off_y, t->smooth ? "true" : "false");
            if (!ident_valid(t->name)) bprintf(&wb, "! %s is not a valid C identifier\n", t->name);
            for (int o = 0; o < p->tex_count; o++)
                if (o != new_ids[k] && strcmp(p->tex[o].name, t->name) == 0)
                    bprintf(&wb, "! %s is declared twice\n", t->name);
        }
    }

    // ── block 2: tables ──
    static RectText rt[MAX_ITEMS];
    int widths[4];

    int ne = 0, nt = 0, nw = 0, name_w = 0, text_w = 0, wtype_w = 0;
    for (int i = 0; i < l->count; i++) {
        const Item *it = &l->items[i];
        if (it->kind == ITEM_IMAGE) {
            ne++;
            int len = (int)strlen(p->tex[it->tex].name) + 1;
            if (len > name_w) name_w = len;
        } else if (it->kind == ITEM_TEXT) {
            nt++;
            int len = it->text >= 0 ? (int)strlen(p->texts[it->text].name) + 1 : 2;
            if (len > text_w) text_w = len;
        } else {
            nw++;
            int len = (int)strlen(p->widget_types[it->wtype]) + 1;
            if (len > wtype_w) wtype_w = len;
        }
    }

    if (s && !s->is_draft) bprintf(&rb, "// Replaces the tables in %s() (%s)\n", s->name, s->source);

    if (ne) {
        // Clickable buttons need code that nothing here can write. Make that
        // impossible to miss: a banner plus a ready-to-fill skeleton.
        int nc = 0;
        for (int i = 0; i < l->count; i++) if (l->items[i].kind == ITEM_IMAGE && l->items[i].clickable) nc++;
        if (nc) {
            char list[1024];
            export_clickables(p, l, elem_name, list, sizeof(list), false);
            bprintf(&rb, "    // ================================================================\n");
            bprintf(&rb, "    // AVISO: %d BOTON%s CLICKABLE%s: %s\n", nc, nc > 1 ? "ES" : "", nc > 1 ? "S" : "", list);
            bprintf(&rb, "    // HAY QUE PROGRAMAR SU LOGICA. El editor solo pone hover_effect;\n");
            bprintf(&rb, "    // que hace cada clic va a mano, por indice. Si reordenas la tabla,\n");
            bprintf(&rb, "    // los indices cambian y nada te avisa.\n");
            bprintf(&rb, "    //\n");
            bprintf(&rb, "    // if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {\n");
            bool first = true;
            for (int i = 0; i < l->count; i++) {
                if (l->items[i].kind != ITEM_IMAGE || !l->items[i].clickable) continue;
                char label[NAME_LEN];
                export_button_label(p, l, i, label, sizeof(label));
                bprintf(&rb, "    //     %sif (CheckCollisionPointRec(mouse_canvas, %s[%d].area)) {\n",
                        first ? "" : "} else ", elem_name, layout_index_of_kind(l, i));
                bprintf(&rb, "    //         // TODO: %s\n", label[0] ? label : "?");
                first = false;
            }
            bprintf(&rb, "    //     }\n");
            bprintf(&rb, "    // }\n");
            bprintf(&rb, "    // ================================================================\n");
        }

        rect_cols(l, ITEM_IMAGE, rt, widths);
        bprintf(&rb, "    static const CanvasElement %s[] = {\n", elem_name);
        int idx = 0;
        for (int i = 0; i < l->count; i++) {
            const Item *it = &l->items[i];
            if (it->kind != ITEM_IMAGE) continue;
            char tex[NAME_LEN + 2];
            snprintf(tex, sizeof(tex), "%s,", p->tex[it->tex].name);
            bprintf(&rb, "        [%d] = {%-*s ", idx, name_w, tex);
            put_rect(&rb, &rt[i], widths);
            if (it->clickable) bprintf(&rb, ", true, %s", p->tex[it->hover_tex].name);
            bprintf(&rb, "},\n");

            if (it->tex == p->tex_none) bprintf(&wb, "! %s[%d] has no texture (TEX_NONE)\n", elem_name, idx);
            if (it->clickable && it->hover_tex == p->tex_none) bprintf(&wb, "! %s[%d] is clickable but its hover texture is TEX_NONE\n", elem_name, idx);
            idx++;
        }
        bprintf(&rb, "    };\n");
    }

    if (nt) {
        if (ne) bprintf(&rb, "\n");
        rect_cols(l, ITEM_TEXT, rt, widths);
        bprintf(&rb, "    static const CanvasElementText %s[] = {\n", text_name);
        int idx = 0;
        for (int i = 0; i < l->count; i++) {
            const Item *it = &l->items[i];
            if (it->kind != ITEM_TEXT) continue;
            bprintf(&rb, "        [%d] = {", idx);
            put_rect(&rb, &rt[i], widths);
            char tname[NAME_LEN + 2];
            snprintf(tname, sizeof(tname), "%s,", it->text >= 0 ? p->texts[it->text].name : "0");
            bprintf(&rb, ", %-*s %s},\n", text_w, tname, p->styles[it->style].name);
            if (it->text < 0) {
                bprintf(&wb, "! %s[%d] has no text (it will draw %s)\n", text_name, idx, p->text_count ? p->texts[0].name : "the first TextId");
            }
            idx++;
        }
        bprintf(&rb, "    };\n");
    }

    if (nw) {
        if (ne || nt) bprintf(&rb, "\n");
        rect_cols(l, ITEM_WIDGET, rt, widths);
        bprintf(&rb, "    static const CanvasWidget %s[] = {\n", widget_name);
        int idx = 0;
        for (int i = 0; i < l->count; i++) {
            const Item *it = &l->items[i];
            if (it->kind != ITEM_WIDGET) continue;
            char wt[NAME_LEN + 2];
            snprintf(wt, sizeof(wt), "%s,", p->widget_types[it->wtype]);
            bprintf(&rb, "        [%d] = {%-*s ", idx, wtype_w, wt);
            put_rect(&rb, &rt[i], widths);
            bprintf(&rb, ", %s", p->setting_count ? p->settings[it->setting] : "0");
            if (it->option[0]) bprintf(&rb, ", %s", it->option);
            bprintf(&rb, "},\n");
            idx++;
        }
        bprintf(&rb, "    };\n");
    }

    if (!ne && !nt && !nw) bprintf(&rb, "    // (empty canvas)\n");

    for (int i = 0; i < l->count; i++) {
        const Rectangle a = l->items[i].area;
        if (a.x + a.width <= 0 || a.y + a.height <= 0 || a.x >= p->canvas_w || a.y >= p->canvas_h)
            bprintf(&wb, "! %s[%d] is entirely outside the canvas\n",
                    l->items[i].kind == ITEM_IMAGE ? elem_name : l->items[i].kind == ITEM_TEXT ? text_name : widget_name, layout_index_of_kind(l, i));
    }

    *textures_block = bdone(&tb);
    *tables_block = bdone(&rb);
    *warnings = bdone(&wb);
}

bool export_write_draft(const Project *p, const Screen *s, const char *textures_block,
                        const char *tables_block, char *out_path, int out_size) {
    char dir[PATH_LEN * 2];
    snprintf(dir, sizeof(dir), "%s/tools/ui_editor/out", p->root);
    if (!DirectoryExists(dir)) MakeDirectory(dir);
    snprintf(out_path, out_size, "%s/%s.c", dir, s->name);

    FILE *f = fopen(out_path, "w");
    if (!f) return false;
    fprintf(f, "// ui_editor draft: %s\n", s->name);
    fprintf(f, "// Not compiled by anything. Paste the two blocks into the game by hand.\n");
    fprintf(f, "// The editor reads this file back, so this is also your save.\n\n");
    fprintf(f, "// ===== BLOCK 1: textures =====\n%s\n", textures_block);
    fprintf(f, "// ===== BLOCK 2: tables =====\n%s", tables_block);
    fclose(f);
    return true;
}
