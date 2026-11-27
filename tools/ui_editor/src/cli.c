// No-window modes. Handy for checking the parser against the real game.
//
//   ui_editor ROOT --list            screens and textures it found
//   ui_editor ROOT --build SCREEN    print both blocks for a screen (writes nothing)
//   ui_editor ROOT --selftest        for every game screen: export, re-read the
//                                    export, check it's identical to the source

#include "cli.h"
#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Project P;
static Layout  A, B;

static int find_screen(const char *name) {
    for (int i = 0; i < P.screen_count; i++) if (strcmp(P.screens[i].name, name) == 0 && !P.screens[i].is_draft) return i;
    for (int i = 0; i < P.screen_count; i++) if (strcmp(P.screens[i].name, name) == 0) return i;
    return -1;
}

static bool same_item(const Item *a, const Item *b) {
    if (a->kind != b->kind) return false;
    if (a->area.x != b->area.x || a->area.y != b->area.y || a->area.width != b->area.width || a->area.height != b->area.height) return false;
    if (a->kind == ITEM_IMAGE) {
        if (a->tex != b->tex || a->clickable != b->clickable) return false;
        if (a->clickable && a->hover_tex != b->hover_tex) return false;
        return true;
    }
    return a->text == b->text;
}

// Writes the export into a temp file shaped like a draft and reads it back.
static bool roundtrip(int s) {
    char *tb, *rb, *wb;
    export_blocks(&P, &P.screens[s], &A, &tb, &rb, &wb);

    char tmp_rel[PATH_LEN] = "tools/ui_editor/build/selftest_tmp.c";
    char tmp[PATH_LEN * 2];
    snprintf(tmp, sizeof(tmp), "%s/%s", P.root, tmp_rel);
    FILE *f = fopen(tmp, "w");
    if (!f) { printf("  can't write %s\n", tmp); return false; }
    fprintf(f, "%s\n%s", tb, rb);
    fclose(f);
    free(tb); free(rb); free(wb);

    Screen fake = P.screens[s];
    fake.is_draft = true;
    snprintf(fake.source, PATH_LEN, "%s", tmp_rel);
    int slot = P.screen_count;
    P.screens[slot] = fake;
    P.screen_count++;
    bool ok = layout_load(&P, slot, &B);
    P.screen_count--;
    remove(tmp);
    if (!ok) { printf("  re-reading the export failed\n"); return false; }

    if (A.count != B.count) { printf("  item count %d -> %d\n", A.count, B.count); return false; }
    for (int i = 0; i < A.count; i++)
        if (!same_item(&A.items[i], &B.items[i])) { printf("  item %d differs\n", i); return false; }
    return true;
}

int cli_run(const char *root, int argc, char **argv) {
    if (argc < 1) return -1;
    const char *mode = argv[0];
    if (strcmp(mode, "--list") && strcmp(mode, "--build") && strcmp(mode, "--selftest")) return -1;

    SetTraceLogLevel(LOG_WARNING);
    if (!project_load(&P, root)) return 1;

    if (strcmp(mode, "--list") == 0) {
        printf("canvas %gx%g, label height %g, languages:", P.canvas_w, P.canvas_h, P.text_h);
        for (int i = 0; i < P.lang_count; i++) printf(" %s", P.langs[i]);
        printf("\n\n%d textures:\n", P.tex_count);
        for (int i = 0; i < P.tex_count; i++) {
            TexEntry *t = &P.tex[i];
            printf("  %-36s %4dx%-4d %s %s%s\n", t->name, t->frame_w, t->frame_h, t->smooth ? "smooth" : "pixel ",
                   t->path[0] ? t->path : "(no def)", t->is_new ? "  [draft]" : "");
        }
        printf("\n%d screens:\n", P.screen_count);
        for (int i = 0; i < P.screen_count; i++) {
            Screen *s = &P.screens[i];
            printf("  %-24s %-34s %s%s%s\n", s->name, s->source, s->elem_table, s->text_table[0] ? " + " : "", s->text_table);
        }
        return 0;
    }

    if (strcmp(mode, "--build") == 0) {
        if (argc < 2) { fprintf(stderr, "--build needs a screen name\n"); return 1; }
        int s = find_screen(argv[1]);
        if (s < 0) { fprintf(stderr, "no screen called %s (try --list)\n", argv[1]); return 1; }
        if (!layout_load(&P, s, &A)) return 1;
        char *tb, *rb, *wb;
        export_blocks(&P, &P.screens[s], &A, &tb, &rb, &wb);
        printf("// ===== BLOCK 1: textures =====\n%s\n// ===== BLOCK 2: tables =====\n%s", tb, rb);
        if (wb[0]) fprintf(stderr, "\nwarnings:\n%s", wb);
        free(tb); free(rb); free(wb);
        return 0;
    }

    // --selftest
    int failed = 0, tested = 0;
    for (int s = 0; s < P.screen_count; s++) {
        tested++;
        printf("%-24s %-6s ", P.screens[s].name, P.screens[s].is_draft ? "draft" : "game");
        if (!layout_load(&P, s, &A)) { printf("FAIL (can't read)\n"); failed++; continue; }
        printf("%2d items ... ", A.count);
        if (roundtrip(s)) printf("ok\n");
        else { printf("FAIL\n"); failed++; }
    }
    if (P.warn_count) {
        printf("\nwarnings while parsing:\n");
        for (int i = 0; i < P.warn_count; i++) printf("  %s\n", P.warn[i]);
    }
    printf("\n%d/%d screens round-trip exactly\n", tested - failed, tested);
    return failed ? 1 : 0;
}
