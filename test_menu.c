/*
File:       test_menu.c
Purpose:    Example demonstrating the tu_menu_t (FIELD_MENU) widget.

Layout:
  +-- File Menu (col 2, row 1) ---+   Status label (col 2, row 10)
  | items: New, Open, Save, ...   |   shows which item was selected or "closed"
  +--------------------------------+

Keys:
  UP / DOWN   navigate items
  ENTER       select highlighted item  -> fires FIELD_NOTIFY_MENUSELECTED
  ESC         close / dismiss menu     -> fires FIELD_NOTIFY_MENUCLOSED
  Ctrl+C / Alt+F3   quit
*/

#include "termbox2.h"
#include "tb2uv.h"

#define ID_MENU_FILE    10
#define ID_STATUS       20

/* menu item indices */
#define MENU_NEW        0
#define MENU_OPEN       1
#define MENU_SAVE       2
#define MENU_SAVE_AS    3
#define MENU_SEP        4   /* visual separator */
#define MENU_EXIT       5

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
    tu_window_t*  wndp    = (tu_window_t*)notify->data;
    tu_wnditem_t* status  = NULL;
    char          text[FIELD_MAX_TEXT + 1] = "";

    if (notify->id == ID_MENU_FILE)
    {
        tu_menu_t*    mnup   = (tu_menu_t*)notify->data;
        wndp   = tu_wnditem_getparent((tu_wnditem_t*)mnup);
        status = tu_wnd_getfield(wndp, ID_STATUS);

        if (notify->code == FIELD_NOTIFY_MENUSELECTED)
        {
            int sel = tu_mnu_getcursel(mnup);
            tu_mnu_getitemtext(mnup, sel, text, FIELD_MAX_TEXT);

            if (sel == MENU_EXIT)
            {
                tu_shutdown();
                return 1;
            }
            if (sel == MENU_SEP)
            {
                /* separator: do nothing */
                return 1;
            }
            /* show selected item name in the status label */
            tu_wnditem_settext(status, text);
            tu_wnditem_draw(status);
            return 1;
        }
        else if (notify->code == FIELD_NOTIFY_MENUCLOSED)
        {
            tu_wnditem_settext(status, "[ menu closed - press ESC again to re-open ]");
            tu_wnditem_draw(status);
            return 1;
        }
    }
    return 0;
}

static void init_controls()
{
    tu_field_t    fld;
    tu_window_t*  wndp  = tu_wnd_new();
    tu_wnditem_t* itemp = NULL;
    tu_menu_t*    mnup  = NULL;

    /* --- title label --- */
    tu_fld_initlabel(&fld, 1, 0, 0, 60, 1,
        "  MENU DEMO  |  UP/DOWN=navigate  ENTER=select  ESC=close  Alt+F3=quit",
        FIELD_LEFT, FIELD_REVERSE, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_WHITE, FIELD_BLUE);

    /* --- File menu: position (2,2), size 22 wide x 9 tall (7 inner rows) --- */
    tu_fld_initmenu(&fld, ID_MENU_FILE, 2, 2, 22, 9, FIELD_LEFT, 0, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_WHITE, FIELD_BLUE);
    mnup = (tu_menu_t*)itemp;

    tu_mnu_additem(mnup, " New          Ctrl+N", NULL);
    tu_mnu_additem(mnup, " Open...      Ctrl+O", NULL);
    tu_mnu_additem(mnup, " Save         Ctrl+S", NULL);
    tu_mnu_additem(mnup, " Save As...         ", NULL);
    tu_mnu_additem(mnup, "--------------------", NULL);   /* visual separator */
    tu_mnu_additem(mnup, " Exit         Alt+F3", NULL);

    /* --- status label: shows feedback below the menu --- */
    tu_fld_initlabel(&fld, ID_STATUS, 2, 12, 58, 1,
        "Use arrow keys to navigate, ENTER to select.", FIELD_LEFT, 0, NULL);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_YELLOW, FIELD_DEFAULT);

    /* --- register events --- */
    tu_wnd_setevent(wndp, FIELD_EV_KEYDOWN, on_keydown);
    tu_wnd_setevent(wndp, FIELD_EV_NOTIFY,  on_notify);

    tu_wnd_refresh(wndp);
    tu_setwindow(wndp);
}

int main()
{
    tu_init();
    tu_enablemouse();
    init_controls();
    tu_run();
    return 0;
}
