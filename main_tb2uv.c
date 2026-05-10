#include "termbox2.h"
#include "tb2uv.h"

static int on_keydown(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_window_t* wndp = (tu_window_t*)notify->data;

    if (TB_MOD_ALT == mod &&
        TB_KEY_F3  == key )
    {
        tu_shutdown();
        return 1; /*processed*/
    }
    else if (TB_MOD_ALT == mod &&
            TB_KEY_F1   == key)
    {
        tu_wnd_setactive(wndp, 50);
        return 1;
    }
    else if (TB_MOD_ALT == mod &&
            TB_KEY_F2   == key)
    {
        tu_wnd_setactive(wndp, 20);
        return 1;
    }
    return 0;   /*return to the global event*/
}
static int on_blur(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_wnditem_t* itemp = (tu_wnditem_t*)notify->data;
    tu_window_t* wndp   = tu_wnditem_getparent(itemp);
    tu_wnditem_t* label = tu_wnd_getfield(wndp, 10);
    char text[FIELD_MAX_TEXT + 1] = "";
    
    tu_wnditem_gettext(itemp, text, FIELD_MAX_TEXT);
    tu_wnditem_settext(label, text);
    
    if (notify->id == 30)
    {
        tu_input_t* inp = (tu_input_t*)itemp;
        int number = tu_inp_getnumber(inp);
        if (number < 100)
        {
            tu_wnditem_settext(label, "NUMBER MUST BE GREATER THAN 100!!");
            tu_wnditem_draw(label);
            return 1;
        }
    }
    tu_wnditem_draw(label);
    return 0;   /*return to the global event*/
}
static int on_focus(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_wnditem_t* itemp = (tu_wnditem_t*)notify->data;
    tu_window_t* wndp   = tu_wnditem_getparent(itemp);
    tu_wnditem_t* label = tu_wnd_getfield(wndp, 11);
    char text[FIELD_MAX_TEXT + 1] = "";
    
    tu_wnditem_gettext(itemp, text, FIELD_MAX_TEXT);
    tu_wnditem_settext(label, text);
    tu_wnditem_draw(label);
    return 0;   /*return to the global event*/
}
static int on_notify(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_wnditem_t* itemp  = (tu_wnditem_t*)notify->data;
    tu_window_t*  wndp   = tu_wnditem_getparent(itemp);
    tu_wnditem_t* label  = tu_wnd_getfield(wndp, 10);
    tu_wnditem_t* label2 = tu_wnd_getfield(wndp, 11);
    tu_wnditem_t* label3 = tu_wnd_getfield(wndp, 12);
    char text[FIELD_MAX_TEXT + 1] = "";

    if (notify->id == 50)
    {
        tu_listbox_t* lbxp = (tu_listbox_t*)itemp;
        int currow = tu_lbx_getcursel(lbxp);
        tu_subitem_t  subitem;
        
        subitem.text = text; /*required*/
        tu_lbx_getitem(lbxp, currow, 0, &subitem);
        switch (notify->code)
        {
            case FIELD_NOTIFY_ITEMCHANGING:
                tu_wnditem_settext(label, subitem.text);
                tu_wnditem_draw(label);
                break;
            case FIELD_NOTIFY_ITEMCHANGED:
                tu_wnditem_settext(label2, subitem.text);
                tu_wnditem_draw(label2);
                break;
            case FIELD_NOTIFY_PRESSEDENTER:
                tu_lbx_getitem(lbxp, currow, 1, &subitem);
                tu_wnditem_settext(label3, subitem.text);
                tu_wnditem_draw(label3);
                break;
        }
    }
    return 0;   /*return to the global event*/
}

static void init_listbox(tu_window_t*    wndp)
{
    tu_field_t      fld;
    tu_header_t     headers[4];
    tu_subitem_t    subitem[4];
    tu_listbox_t*   lbxp    = 0;
    tu_wnditem_t*   itemp   = 0;

    tu_fld_initlistbox(&fld, 50, 31, 4, 30, 5, "THIS IS A LISTBOX", 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    lbxp = (tu_listbox_t*)itemp;

    headers[0].w        = 4;
    headers[0].fgcolor  = 0;
    headers[0].bgcolor  = 0;
    headers[0].alignment= FIELD_RIGHT;
    headers[0].attribs  = 0;
    headers[0].text     = "ID";

    headers[1].w        = 10;
    headers[1].fgcolor  = 0;
    headers[1].bgcolor  = 0;
    headers[1].alignment= 0;
    headers[1].attribs  = 0;
    headers[1].text     = "NAME";

    headers[2].w        = 10;
    headers[2].fgcolor  = 0;
    headers[2].bgcolor  = 0;
    headers[2].alignment= FIELD_RIGHT;
    headers[2].attribs  = 0;
    headers[2].text     = "GRADE";

    headers[3].w        = 5;
    headers[3].fgcolor  = 0;
    headers[3].bgcolor  = 0;
    headers[3].alignment= FIELD_RIGHT;
    headers[3].attribs  = 0;
    headers[3].text     = "AGE";
    
    tu_lbx_addheader(lbxp, headers, 4);
    
    subitem[0].fgcolor  = FIELD_CYAN;
    subitem[0].bgcolor  = 0;
    subitem[0].attribs  = 0;
    subitem[0].data     = 0;
    subitem[0].text     = "1";

    subitem[1].fgcolor  = FIELD_BLUE;
    subitem[1].bgcolor  = 0;
    subitem[1].attribs  = 0;
    subitem[1].data     = 0;
    subitem[1].text     = "Tekari";

    subitem[2].fgcolor  = FIELD_YELLOW;
    subitem[2].bgcolor  = 0;
    subitem[2].attribs  = 0;
    subitem[2].data     = 0;
    subitem[2].text     = "4";

    subitem[3].fgcolor  = FIELD_GREEN;
    subitem[3].bgcolor  = 0;
    subitem[3].attribs  = 0;
    subitem[3].data     = 0;
    subitem[3].text     = "10";
    
    tu_lbx_add(lbxp, subitem, 4);

    subitem[0].text     = "2";
    subitem[1].text     = "Tekaruj";
    subitem[2].text     = "3";
    subitem[3].text     = "5";
    tu_lbx_add(lbxp, subitem, 4);

    subitem[0].text     = "3";
    subitem[1].text     = "Tekarin";
    subitem[2].text     = "2";
    subitem[3].text     = "2";
    tu_lbx_add(lbxp, subitem, 4);

    subitem[0].text     = "4";
    subitem[1].text     = "Tekaroj";
    subitem[2].text     = "1";
    subitem[3].text     = "1";
    tu_lbx_add(lbxp, subitem, 4);

    subitem[0].text     = "5";
    subitem[1].text     = "Tekarun";
    subitem[2].text     = "0";
    subitem[3].text     = "3/12";
    tu_lbx_add(lbxp, subitem, 4);

    subitem[0].text     = "6";
    subitem[1].text     = "Tekarain";
    subitem[2].text     = "0.1";
    subitem[3].text     = "4/12";
    tu_lbx_add(lbxp, subitem, 4);
    
    tu_lbx_sort(lbxp, 1, 1);/*test sorting by name*/
    
    tu_lbx_setcursel(lbxp, 4, 0);
}
static void init_controls()
{
    tu_field_t      fld;
    tu_window_t*    wndp    = tu_wnd_new();
    tu_input_t*     inp     = 0;
    tu_wnditem_t*   itemp   = 0;
    
    tu_fld_initlabel(&fld, 10,  0, 0, 30, "PRESS ALT+F3 TO QUIT", 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setattribs(itemp, FIELD_REVERSE);
    tu_wnditem_setcolor(itemp, FIELD_CYAN, FIELD_BLUE);
    
    tu_fld_initlabel(&fld, 11,  0, 1, 30, "INFORMATION HERE!!", 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    /*tu_wnditem_setattribs(itemp, FIELD_REVERSE);*/
    tu_wnditem_setcolor(itemp, 0, FIELD_BLUE);

    tu_fld_initlabel(&fld, 12,  0, 2, 30, "TRY TEXT ME!!", 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    /*tu_wnditem_setattribs(itemp, FIELD_REVERSE);*/
    tu_wnditem_setcolor(itemp, 0, FIELD_BLUE);

    tu_fld_initinput(&fld,  20, 31, 0, 20, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_MAGENTA, 0);
    tu_wnditem_setflags(itemp, FIELD_INPUT_CAPITAL|FIELD_INPUT_PASSWORD);
    inp = (tu_input_t*)itemp;
    tu_inp_setlimit(inp, 8);

    tu_fld_initinput(&fld,  30, 31, 1, 20, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_GREEN, 0);
    tu_wnditem_setflags(itemp, FIELD_INPUT_NUMBER);
    inp = (tu_input_t*)itemp;
    tu_wnditem_settext(itemp, "9230941");

    tu_fld_initinput(&fld,  40, 31, 2, 20, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_BLUE, 0);
    tu_wnditem_setflags(itemp, FIELD_INPUT_HEXNUMBER);
    tu_wnditem_settext(itemp, "3829addce");
    
    init_listbox(wndp);
    
    tu_wnd_setevent(wndp, FIELD_EV_KEYDOWN, on_keydown);
    tu_wnd_setevent(wndp, FIELD_EV_BLUR, on_blur);
    tu_wnd_setevent(wndp, FIELD_EV_FOCUS, on_focus);
    tu_wnd_setevent(wndp, FIELD_EV_NOTIFY, on_notify);
    /*tu_wnd_removefield(wndp, 30);*//*test remove the field*/

    tu_wnd_refresh(wndp);
    
    tu_setwindow(wndp);
}

int main()
{
    tu_init();
    tu_drawbox(50, 1, 20, 10, '-', '|', '+', 0, 0, 0, 1);
    init_controls();
    tu_run();
    return 0;
}
