#include "termbox2.h"
#include "tb2uv.h"

#define ID_QUIT         (1000)

static int on_event(int mod, int key, int ch, void* data)
{
    if (TB_MOD_ALT == mod &&
        TB_KEY_F3  == key )
    {
        tu_shutdown();
        return 1; /*processed*/
    }
    return 0;   /*return to the global event*/
};

void init()
{
    tu_window_t*    wndp = tu_wnd_new();
    tu_listbox_t*   lbxp = 0;
    tu_header_t     headers[4];
    tu_field_t      fld;
    tu_wnditem_t*   itemp = 0;
    
    tu_fld_initlabel(&fld, 10,  0, 0, 30, "PRESS ALT+F3 TO QUIT");
    itemp = tu_wnd_addfield(wndp, &fld);
    tu_wnditem_setattribs(itemp, FIELD_REVERSE);

    tu_fld_initinput(&fld,  20, 31, 0, 20, "TYPE ON ME!!");
    tu_wnd_addfield(wndp, &fld);

    tu_fld_initinput(&fld,  30, 31, 1, 20, "THIS IS A SECOND INPUT");
    tu_wnd_addfield(wndp, &fld);

    tu_fld_initinput(&fld,  40, 31, 2, 20, "THIS IS A THIRD INPUT");
    tu_wnd_addfield(wndp, &fld);
    
    tu_fld_initlistbox(&fld, 50, 31, 4, 16, 5);
    lbxp = (tu_listbox_t*)tu_wnd_addfield(wndp, &fld);

    headers[0].w = 4;
    headers[0].fgcolor = 0;
    headers[0].bgcolor = 0;
    headers[0].alignment = FIELD_RIGHT;
    headers[0].attribs   = 0;
    headers[0].text = "ID";

    headers[1].w = 10;
    headers[1].fgcolor = 0;
    headers[1].bgcolor = 0;
    headers[1].alignment = 0;
    headers[1].attribs   = 0;
    headers[1].text = "NAME";

    headers[2].w = 10;
    headers[2].fgcolor = 0;
    headers[2].bgcolor = 0;
    headers[2].alignment = FIELD_RIGHT;
    headers[2].attribs   = 0;
    headers[2].text = "GRADE";

    headers[3].w = 5;
    headers[3].fgcolor = 0;
    headers[3].bgcolor = 0;
    headers[3].alignment = FIELD_RIGHT;
    headers[3].attribs   = 0;
    headers[3].text = "AGE";
    
    tu_lbx_addheader(lbxp, headers, 4);

    tu_wnd_setevent(wndp, on_event);
    tu_wnd_refresh(wndp);

    tu_setwindow(wndp);
}

int main()
{
    tu_init();
    /*tu_setcbreak(1);*/
    
    /*tu_draw_text(0, 0, 40, "PRESS ALT+F3 TO QUIT", 0, 0, FIELD_LEFT, FIELD_REVERSE, 1);   */
    tu_drawbox(50, 1, 20, 10, '-', '|', '+', 0, 0, 0, 1);
    
    init();
    tu_addevent(TB_MOD_ALT, TB_KEY_F3, 0, ID_QUIT);

    tu_run();
    return 0;
}
