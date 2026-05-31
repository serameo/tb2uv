/*
File:       test_menu_json.c
Purpose:    Example: load menu + submenus from menu.json and handle selection.

Layout:
  Menu (from JSON)         Status bar (bottom)
  +--------------------+   shows selected item text and its action ID

Keys inside menu:
  UP / DOWN   navigate (separators are skipped)
  ENTER       select leaf item  -> FIELD_NOTIFY_MENUSELECTED
  RIGHT       open submenu (or same as ENTER on leaf)
  LEFT / ESC  close submenu and return to parent
  ESC at root -> FIELD_NOTIFY_MENUCLOSED
  Ctrl+C / Alt+F3  quit
*/

#include <stdio.h>
#include "termbox2.h"
#include "tb2uv.h"
#include "tu_menu_json.h"

#define ID_STATUS   200

static int on_keydown(int mod, int key, int ch, tu_notify_t* notify)
{
    if (TB_MOD_ALT == mod && TB_KEY_F3 == key)
    {
        tu_shutdown();
        return 1;
    }
    return 0;
}

static int on_notify(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_menu_t*    mnup   = (tu_menu_t*)notify->data;
    tu_window_t*  wndp   = tu_wnditem_getparent((tu_wnditem_t*)mnup);
    tu_wnditem_t* status = tu_wnd_getfield(wndp, ID_STATUS);
    char text[FIELD_MAX_TEXT + 1] = "";
    char status_text[FIELD_MAX_TEXT + 1] = "";

    if (notify->code == FIELD_NOTIFY_MENUSELECTED)
    {
        int sel     = tu_mnu_getcursel(mnup);
        int item_id = tu_mnu_getitemid(mnup, sel);

        tu_mnu_getitemtext(mnup, sel, text, FIELD_MAX_TEXT);

        if (item_id == 99)   /*Exit*/
        {
            tu_shutdown();
            return 1;
        }

        snprintf(status_text, FIELD_MAX_TEXT,
            "Selected: id=%d  \"%.*s\"", item_id,
            FIELD_MAX_TEXT - 32, text);
        tu_wnditem_settext(status, status_text);
        tu_wnditem_draw(status);
        return 1;
    }

    if (notify->code == FIELD_NOTIFY_MENUCLOSED)
    {
        tu_wnditem_settext(status, "Menu closed. Press any arrow key to reopen.");
        tu_wnditem_draw(status);
        return 1;
    }

    return 0;
}

int main()
{
    tu_window_t*  wndp = NULL;
    tu_menu_t*    mnup = NULL;
    tu_field_t    fld;
    tu_wnditem_t* itemp;

    tu_init();
    tu_enablemouse();

    wndp = tu_wnd_new();

    /* title */
    tu_fld_initlabel(&fld, 1, 0, 0, 70, 1,
        "  MENU JSON DEMO  |  ENTER=select  RIGHT=submenu  LEFT/ESC=back  Alt+F3=quit",
        FIELD_LEFT, FIELD_REVERSE, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_WHITE, FIELD_BLUE);

    /* load menu tree from JSON */
    mnup = tu_menu_load_json(wndp, "menu.json");
    if (!mnup)
    {
        tu_shutdown();
        fprintf(stderr, "Failed to load menu.json\n");
        return 1;
    }

    /* status bar */
    tu_fld_initlabel(&fld, ID_STATUS, 0, 22, 70, 1,
        "  Use arrow keys to navigate the menu.",
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
