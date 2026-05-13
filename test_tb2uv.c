#include "termbox2.h"
#include "tb2uv.h"

#define ID_LISTBOX1     100
#define ID_EDIT1        101
#define ID_INPUT1       102
#define ID_INPUT2       103
#define ID_INPUT3       104
#define ID_LABEL1       105
#define ID_LABEL2       106
#define ID_LABEL3       107
#define ID_LABEL4       108

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
        tu_wnd_setactive(wndp, ID_LISTBOX1);
        return 1;
    }
    else if (TB_MOD_ALT == mod &&
            TB_KEY_F2   == key)
    {
        tu_wnd_setactive(wndp, ID_EDIT1);
        return 1;
    }
    return 0;   /*return to the global event*/
}
static int on_blur(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_wnditem_t* itemp = (tu_wnditem_t*)notify->data;
    tu_window_t* wndp   = tu_wnditem_getparent(itemp);
    tu_wnditem_t* label = tu_wnd_getfield(wndp, ID_LABEL1);
    char text[FIELD_MAX_TEXT + 1] = "";
    
    tu_wnditem_gettext(itemp, text, FIELD_MAX_TEXT);
    tu_wnditem_settext(label, text);
    
    if (notify->id == ID_INPUT2)
    {
        tu_input_t* inp = (tu_input_t*)itemp;
        int number = tu_inp_getnumber(inp);
        if (number <= 100)
        {
            tu_wnditem_settext(label, "MUST BE GREATER THAN 100!!");
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
    tu_wnditem_t* label = tu_wnd_getfield(wndp, ID_LABEL2);
    char text[FIELD_MAX_TEXT + 1] = "";
    
    tu_wnditem_gettext(itemp, text, FIELD_MAX_TEXT);
    tu_wnditem_settext(label, text);
    tu_wnditem_draw(label);
    return 0;   /*return to the global event*/
}
static int on_notify(int mod, int key, int ch, tu_notify_t* notify)
{
    tu_wnditem_t* itemp  = (tu_wnditem_t*)notify->data;
    tu_window_t*  wndp   = (notify->id != 0 ?   tu_wnditem_getparent(itemp) : 
                                                (tu_window_t*)notify->data);
    tu_wnditem_t* label  = tu_wnd_getfield(wndp, ID_LABEL1);
    tu_wnditem_t* label2 = tu_wnd_getfield(wndp, ID_LABEL2);
    tu_wnditem_t* label3 = tu_wnd_getfield(wndp, ID_LABEL3);
    char text[FIELD_MAX_TEXT + 1] = "";

    if (notify->id == ID_LISTBOX1)
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
            case FIELD_NOTIFY_MOUSELEFTCLICKED:
            case FIELD_NOTIFY_MOUSERIGHTCLICKED:
            case FIELD_NOTIFY_MOUSEMIDDLECLICKED:
                sprintf(text, "ID:%d got clicked (%d)", notify->id, notify->code);
                tu_wnditem_settext(label3, text);
                tu_wnditem_draw(label3);
                break;
        }
        return 0;
    }
    if (notify->id == ID_INPUT1 ||
        notify->id == ID_INPUT2 ||
        notify->id == ID_INPUT3)
    {
        tu_wnditem_gettext(itemp, text, FIELD_MAX_TEXT);
        tu_wnditem_settext(label3, text);
        tu_wnditem_draw(label3);
        return 0;
    }

    if ((notify->code == FIELD_NOTIFY_MOUSELEFTCLICKED) ||
        (notify->code == FIELD_NOTIFY_MOUSERIGHTCLICKED) ||
        (notify->code == FIELD_NOTIFY_MOUSEMIDDLECLICKED))
    {
        if (notify->id == 0)
        {
            /*data = wndp*/
            wndp = (tu_window_t*)notify->data;
            sprintf(text, "window got clicked (%d)", notify->code);
            tu_wnditem_settext(label3, text);
            tu_wnditem_draw(label3);
            return 0;
        }
        sprintf(text, "ID:%d got clicked (%d)", notify->id, notify->code);
        tu_wnditem_settext(label3, text);
        tu_wnditem_draw(label3);
        return 0;
    }
    return 0;   /*return to the global event*/
}

static void init_listbox(tu_window_t* wndp, int id)
{
    tu_field_t      fld;
    tu_header_t     headers[4];
    tu_subitem_t    subitem[4];
    tu_listbox_t*   lbxp    = 0;
    tu_wnditem_t*   itemp   = 0;
    tu_layer_t*     layp    = 0;

    layp = tu_wnd_newlayer(wndp);
    /*tu_lay_visible(layp, 0);*//*test: hide layer*/
    tu_fld_initlistbox(&fld, id, 31, 4, 30, 5, "THIS IS A LISTBOX", 0, 0, 0);
    itemp = tu_wnd_addfieldlayer(wndp, &fld, layp);
    lbxp = (tu_listbox_t*)itemp;
    tu_wnditem_setflags(itemp, FIELD_LISTBOX_SORTABLE|FIELD_LISTBOX_HIDEHEADER);

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

    tu_fld_initedit(&fld, ID_EDIT1, 71, 0, 30, 10, "", 0, 0, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, 0, FIELD_BLUE);

    tu_fld_initlabel(&fld, ID_LABEL1,  0, 0, 30, 1, "PRESS ALT+F3 TO QUIT", 0, FIELD_REVERSE, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    /*tu_wnditem_setattribs(itemp, FIELD_REVERSE);*/
    tu_wnditem_setcolor(itemp, FIELD_CYAN, FIELD_BLUE);
    
    tu_fld_initlabel(&fld, ID_LABEL2,  0, 1, 30, 1, "INFORMATION HERE!!", 0, 0, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    /*tu_wnditem_setattribs(itemp, FIELD_REVERSE);*/
    tu_wnditem_setcolor(itemp, 0, FIELD_BLUE);

    tu_fld_initlabel(&fld, ID_LABEL3,  0, 2, 30, 1, "TRY TEXT ME!!", 0, 0, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    /*tu_wnditem_setattribs(itemp, FIELD_REVERSE);*/
    tu_wnditem_setcolor(itemp, 0, FIELD_GREEN);

    /*tu_drawbox(0, 10, 20, 10, '-', '|', '+', 0, 0, 0, 0);*/
    tu_fld_initlabel(&fld, ID_LABEL4,  1, 11, 19, 9, 
        "the quick brown fox is jumping over the lazy dog."
        "THE QUICK BROWN FOX IS JUMPING OVER THE LAZY DOG.",
        FIELD_CENTER, FIELD_UNDERLINE, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    /*tu_wnditem_setattribs(itemp, FIELD_UNDERLINE);*/
    tu_wnditem_setcolor(itemp, 0, FIELD_RED);
    tu_wnditem_setflags(itemp, FIELD_LABEL_WRAPTEXT);
    /*tu_wnditem_setalignment(itemp, FIELD_RIGHT);*/
    /*tu_wnditem_setalignment(itemp, FIELD_CENTER);*/

    tu_fld_initinput(&fld,  ID_INPUT1, 31, 0, 20, 1, "", 0, 0, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_MAGENTA, 0);
    tu_wnditem_setflags(itemp, FIELD_INPUT_PASSWORD);
    inp = (tu_input_t*)itemp;
    tu_inp_setlimit(inp, 8);

    tu_fld_initinput(&fld,  ID_INPUT2, 31, 1, 20, 1, ">=100", 0, 0, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_GREEN, 0);
    tu_wnditem_setflags(itemp, FIELD_INPUT_NUMBER);
    inp = (tu_input_t*)itemp;
    /*tu_wnditem_settext(itemp, "9230941");*/

    tu_fld_initinput(&fld,  ID_INPUT3, 31, 2, 20, 1, "HEXANUMBER", 0, 0, 0);
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setcolor(itemp, FIELD_BLUE, 0);
    tu_wnditem_setflags(itemp, FIELD_INPUT_CAPITAL|FIELD_INPUT_HEXNUMBER);
    /*tu_wnditem_settext(itemp, "3829addce");*/
    
    init_listbox(wndp, ID_LISTBOX1);
    
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
    tu_enablemouse();
    tu_drawbox(0, 10, 20, 10, '-', '|', '+', 0, 0, 0, 0);
    init_controls();
    tu_run();
    return 0;
}
