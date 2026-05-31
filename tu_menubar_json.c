/*
File:       tu_menubar_json.c
Purpose:    Load tu_menubar_t + dropdown trees from a JSON file (uses cJSON).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "tb2uv.h"
#include "tu_menubar_json.h"

/*forward declarations for mutual recursion*/
static tu_menu_t* mbar_load_submenu(tu_window_t* wndp, cJSON* j_menu, int parent_id);
static void       mbar_load_items(tu_window_t* wndp, tu_menu_t* mnup, cJSON* j_items);

/*--------------------------------------------------------------------------*/

static void mbar_load_items(tu_window_t* wndp, tu_menu_t* mnup, cJSON* j_items)
{
    cJSON* j_item = NULL;
    int    menu_id = tu_wnditem_getid((tu_wnditem_t*)mnup);

    if (!j_items || !cJSON_IsArray(j_items))
    {
        return;
    }
    cJSON_ArrayForEach(j_item, j_items)
    {
        cJSON* j_text = cJSON_GetObjectItem(j_item, "text");
        cJSON* j_id   = cJSON_GetObjectItem(j_item, "id");
        cJSON* j_sep  = cJSON_GetObjectItem(j_item, "separator");
        cJSON* j_subm = cJSON_GetObjectItem(j_item, "submenu");

        const char* text    = (j_text ? j_text->valuestring : "");
        int         item_id = (j_id   ? (int)j_id->valuedouble : 0);
        int         is_sep  = (j_sep  ? cJSON_IsTrue(j_sep) : 0);

        int idx = tu_mnu_additem(mnup, text, NULL);
        if (idx < 0)
        {
            break; /*menu full*/
        }

        tu_mnu_setitemid(mnup, idx, item_id);

        if (is_sep)
        {
            tu_mnu_setitemflags(mnup, idx, FIELD_MENU_ITEM_SEPARATOR);
        }

        if (j_subm)
        {
            tu_menu_t* sub = mbar_load_submenu(wndp, j_subm, menu_id);
            if (sub)
            {
                tu_mnu_setitemsubmenu(mnup, idx,
                    tu_wnditem_getid((tu_wnditem_t*)sub));
            }
        }
    }
}

static tu_menu_t* mbar_load_submenu(tu_window_t* wndp, cJSON* j_menu, int parent_id)
{
    cJSON* j_id    = cJSON_GetObjectItem(j_menu, "id");
    cJSON* j_w     = cJSON_GetObjectItem(j_menu, "w");
    cJSON* j_h     = cJSON_GetObjectItem(j_menu, "h");
    cJSON* j_fg    = cJSON_GetObjectItem(j_menu, "fg");
    cJSON* j_bg    = cJSON_GetObjectItem(j_menu, "bg");
    cJSON* j_items = cJSON_GetObjectItem(j_menu, "items");

    int id = (j_id ? (int)j_id->valuedouble : 0);
    int w  = (j_w  ? (int)j_w->valuedouble  : 20);
    int h  = (j_h  ? (int)j_h->valuedouble  : 5);
    int fg = (j_fg ? (int)j_fg->valuedouble  : FIELD_DEFAULT);
    int bg = (j_bg ? (int)j_bg->valuedouble  : FIELD_DEFAULT);

    tu_field_t    fld;
    tu_wnditem_t* itemp;
    tu_menu_t*    mnup;
    tu_layer_t*   layp;

    if (id == 0)
    {
        return NULL;
    }

    layp = tu_wnd_newlayer(wndp);
    tu_lay_show(layp, 0);

    tu_fld_initmenu(&fld, id, 0, 0, w, h, FIELD_LEFT, 0, NULL);
    itemp = tu_wnd_addfieldlayer(wndp, &fld, layp);
    if (!itemp)
    {
        return NULL;
    }

    tu_wnditem_setcolor(itemp, fg, bg);
    mnup = (tu_menu_t*)itemp;
    tu_mnu_setparent(mnup, parent_id);

    mbar_load_items(wndp, mnup, j_items);
    return mnup;
}

/*--------------------------------------------------------------------------*/

tu_menubar_t* tu_menubar_load_json(tu_window_t* wndp, const char* filepath)
{
    FILE*  fp   = NULL;
    long   size = 0;
    char*  buf  = NULL;
    cJSON* root = NULL;
    cJSON* j_id, *j_x, *j_y, *j_w, *j_fg, *j_bg, *j_entries;
    int    id, x, y, w, fg, bg;
    tu_field_t    fld;
    tu_wnditem_t* itemp;
    tu_menubar_t* mbarp;
    cJSON* j_entry;

    fp = fopen(filepath, "r");
    if (!fp)
    {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buf = (char*)malloc(size + 1);
    if (!buf)
    {
        fclose(fp);
        return NULL;
    }
    fread(buf, 1, size, fp);
    buf[size] = 0;
    fclose(fp);

    root = cJSON_Parse(buf);
    free(buf);
    if (!root)
    {
        return NULL;
    }

    j_id      = cJSON_GetObjectItem(root, "id");
    j_x       = cJSON_GetObjectItem(root, "x");
    j_y       = cJSON_GetObjectItem(root, "y");
    j_w       = cJSON_GetObjectItem(root, "w");
    j_fg      = cJSON_GetObjectItem(root, "fg");
    j_bg      = cJSON_GetObjectItem(root, "bg");
    j_entries = cJSON_GetObjectItem(root, "entries");

    id = (j_id ? (int)j_id->valuedouble : 0);
    x  = (j_x  ? (int)j_x->valuedouble  : 0);
    y  = (j_y  ? (int)j_y->valuedouble  : 0);
    w  = (j_w  ? (int)j_w->valuedouble  : 80);
    fg = (j_fg ? (int)j_fg->valuedouble  : FIELD_DEFAULT);
    bg = (j_bg ? (int)j_bg->valuedouble  : FIELD_DEFAULT);

    if (id == 0)
    {
        cJSON_Delete(root);
        return NULL;
    }

    /*menubar itself goes on the default (always visible) layer*/
    tu_fld_initmenubar(&fld, id, x, y, w, FIELD_LEFT, 0, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    if (!itemp)
    {
        cJSON_Delete(root);
        return NULL;
    }

    tu_wnditem_setcolor(itemp, fg, bg);
    mbarp = (tu_menubar_t*)itemp;

    /*load each entry: create a hidden menu, add it to the bar*/
    if (j_entries && cJSON_IsArray(j_entries))
    {
        cJSON_ArrayForEach(j_entry, j_entries)
        {
            cJSON* j_text = cJSON_GetObjectItem(j_entry, "text");
            cJSON* j_menu = cJSON_GetObjectItem(j_entry, "menu");

            const char* entry_text = (j_text ? j_text->valuestring : "");
            int         bar_id     = tu_wnditem_getid(itemp);

            if (j_menu)
            {
                tu_menu_t* mnup = mbar_load_submenu(wndp, j_menu, bar_id);
                if (mnup)
                {
                    int menu_id = tu_wnditem_getid((tu_wnditem_t*)mnup);
                    tu_mbar_addentry(mbarp, entry_text, menu_id);
                }
            }
            else
            {
                tu_mbar_addentry(mbarp, entry_text, 0);
            }
        }
    }

    cJSON_Delete(root);
    return mbarp;
}
