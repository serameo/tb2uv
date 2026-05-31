/*
File:       test_menubar_json.c
Purpose:    Example: load a full menubar from menubar.json and handle selection.

Layout:
  Row 0  — title / hint strip
  Row 1  — menubar (File  Edit  View  Help)
  Row 2+ — content area (empty in this demo)
  Row 22 — status bar

Keys:
  F10          — activate / re-activate the menu bar
  LEFT / RIGHT — move between top-level entries (also works while a
                 dropdown is open: closes old, opens new)
  DOWN / ENTER — open the highlighted entry's dropdown
  UP / DOWN    — navigate items inside the open dropdown
  RIGHT        — open submenu (on an item that has one)
  LEFT         — go back to parent (or switch menu if at top-level entry)
  ENTER        — select item  ->  FIELD_NOTIFY_MENUSELECTED
  ESC          — close dropdown (return to bar) / deactivate bar
  Alt+F3       — quit
*/

#include <stdio.h>
#include "termbox2.h"
#include "tb2uv.h"
#include "tu_menubar_json.h"

#define ID_STATUS   200

static tu_menubar_t* g_mbarp = NULL;

static int on_keydown(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_window_t* wndp = (tu_window_t*)notify->data;

    if (TB_MOD_ALT == mod && TB_KEY_F3 == key)
    {
        tu_shutdown();
        return 1;
    }
    if (TB_KEY_F10 == key && g_mbarp)
    {
        tu_mbar_setactive(g_mbarp, wndp);
        return 1;
    }
    return 0;
}

static int on_notify(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_menu_t*    mnup   = (tu_menu_t*)notify->data;
    tu_window_t*  wndp   = tu_wnditem_getparent((tu_wnditem_t*)mnup);
    tu_wnditem_t* status = tu_wnd_getfield(wndp, ID_STATUS);
    char item_text[FIELD_MAX_TEXT + 1] = "";
    char status_buf[FIELD_MAX_TEXT + 1] = "";

    if (notify->code != FIELD_NOTIFY_MENUSELECTED)
    {
        return 0;
    }

    int sel     = tu_mnu_getcursel(mnup);
    int item_id = tu_mnu_getitemid(mnup, sel);

    tu_mnu_getitemtext(mnup, sel, item_text, FIELD_MAX_TEXT);

    if (item_id == 99) /*Exit*/
    {
        tu_shutdown();
        return 1;
    }

    snprintf(status_buf, FIELD_MAX_TEXT,
        "  Selected id=%-3d  \"%.*s\"", item_id, 50, item_text);
    tu_wnditem_settext(status, status_buf);
    tu_wnditem_draw(status);
    return 1;
}

int main()
{
    tu_window_t*  wndp = NULL;
    tu_field_t    fld;
    tu_wnditem_t* itemp;

    tu_init();
    tu_enablemouse();

    wndp = tu_wnd_new();

    /*title row*/
    tu_fld_initlabel(&fld, 1, 0, 0, 80, 1,
        "  MENUBAR JSON DEMO  |  F10=open bar  LEFT/RIGHT=switch  ENTER=select  Alt+F3=quit",
        FIELD_LEFT, FIELD_REVERSE, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_WHITE, FIELD_BLUE);

    /*load menubar tree from JSON (placed at row 1)*/
    g_mbarp = tu_menubar_load_json(wndp, "menubar.json");
    if (!g_mbarp)
    {
        tu_shutdown();
        fprintf(stderr, "Failed to load menubar.json\n");
        return 1;
    }

    /*content area placeholder*/
    tu_fld_initlabel(&fld, 2, 0, 3, 80, 18,
        "  (content area — press F10 to open the menu bar)",
        FIELD_LEFT, 0, NULL);
    tu_wnd_addfield(wndp, &fld);

    /*status bar*/
    tu_fld_initlabel(&fld, ID_STATUS, 0, 22, 80, 1,
        "  Press F10 to activate the menu bar.",
        FIELD_LEFT, 0, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_YELLOW, FIELD_DEFAULT);

    tu_wnd_setevent(wndp, FIELD_EV_KEYDOWN, on_keydown);
    tu_wnd_setevent(wndp, FIELD_EV_NOTIFY,  on_notify);

    tu_wnd_refresh(wndp);
    tu_setwindow(wndp);
    tu_run();
    return 0;
}
