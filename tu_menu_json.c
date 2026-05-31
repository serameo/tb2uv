/*
File:       tu_menu_json.c
Purpose:    Load tu_menu_t widget trees from a JSON file (uses cJSON)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "tb2uv.h"
#include "tu_menu_json.h"

/*forward declarations for mutual recursion*/
static tu_menu_t* load_submenu(tu_window_t* wndp, cJSON* j_menu, int parent_id);
static void       load_menu_items(tu_window_t* wndp, tu_menu_t* mnup, cJSON* j_items);

/*--------------------------------------------------------------------------*/

static void load_menu_items(tu_window_t* wndp, tu_menu_t* mnup, cJSON* j_items)
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
            tu_menu_t* sub = load_submenu(wndp, j_subm, menu_id);
            if (sub)
            {
                tu_mnu_setitemsubmenu(mnup, idx,
                    tu_wnditem_getid((tu_wnditem_t*)sub));
            }
        }
    }
}

static tu_menu_t* load_submenu(tu_window_t* wndp, cJSON* j_menu, int parent_id)
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
        return NULL; /*id required for submenus*/
    }

    /*submenus live on their own hidden layers*/
    layp = tu_wnd_newlayer(wndp);
    tu_lay_show(layp, 0);

    /*position 0,0 — will be moved dynamically when opened*/
    tu_fld_initmenu(&fld, id, 0, 0, w, h, FIELD_LEFT, 0, NULL);
    itemp = tu_wnd_addfieldlayer(wndp, &fld, layp);
    if (!itemp)
    {
        return NULL;
    }

    tu_wnditem_setcolor(itemp, fg, bg);
    mnup = (tu_menu_t*)itemp;
    tu_mnu_setparent(mnup, parent_id);

    load_menu_items(wndp, mnup, j_items);
    return mnup;
}

/*--------------------------------------------------------------------------*/

tu_menu_t* tu_menu_load_json(tu_window_t* wndp, const char* filepath)
{
    FILE*  fp   = NULL;
    long   size = 0;
    char*  buf  = NULL;
    cJSON* root = NULL;
    cJSON* j_id, *j_x, *j_y, *j_w, *j_h, *j_fg, *j_bg, *j_items;
    int    id, x, y, w, h, fg, bg;
    tu_field_t    fld;
    tu_wnditem_t* itemp;
    tu_menu_t*    mnup;

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

    j_id    = cJSON_GetObjectItem(root, "id");
    j_x     = cJSON_GetObjectItem(root, "x");
    j_y     = cJSON_GetObjectItem(root, "y");
    j_w     = cJSON_GetObjectItem(root, "w");
    j_h     = cJSON_GetObjectItem(root, "h");
    j_fg    = cJSON_GetObjectItem(root, "fg");
    j_bg    = cJSON_GetObjectItem(root, "bg");
    j_items = cJSON_GetObjectItem(root, "items");

    id = (j_id ? (int)j_id->valuedouble : 0);
    x  = (j_x  ? (int)j_x->valuedouble  : 0);
    y  = (j_y  ? (int)j_y->valuedouble  : 0);
    w  = (j_w  ? (int)j_w->valuedouble  : 20);
    h  = (j_h  ? (int)j_h->valuedouble  : 5);
    fg = (j_fg ? (int)j_fg->valuedouble  : FIELD_DEFAULT);
    bg = (j_bg ? (int)j_bg->valuedouble  : FIELD_DEFAULT);

    if (id == 0)
    {
        cJSON_Delete(root);
        return NULL;
    }

    /*root menu goes on the default layer (always visible)*/
    tu_fld_initmenu(&fld, id, x, y, w, h, FIELD_LEFT, 0, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    if (!itemp)
    {
        cJSON_Delete(root);
        return NULL;
    }

    tu_wnditem_setcolor(itemp, fg, bg);
    mnup = (tu_menu_t*)itemp;
    /*parent_id stays 0 (root)*/

    load_menu_items(wndp, mnup, j_items);

    cJSON_Delete(root);
    return mnup;
}
