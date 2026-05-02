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

static void init()
{
    tu_window_t* wndp = tu_wnd_new();
    tu_field_t*  fldp = tu_fld_new(FIELD_LABEL);
    tu_field_t*  inp  = tu_fld_new(FIELD_INPUT);
    
    tu_fld_initlabel(fldp, 10,  0, 0, 30, "PRESS ALT+F3 TO QUIT");
    tu_fld_setattribs(fldp, FIELD_REVERSE);
    tu_wnd_addfield(wndp, fldp);

    tu_fld_initinput(inp,  20, 31, 0, 20, "TYPE ON ME!!");
    tu_wnd_addfield(wndp, inp);

    tu_fld_initinput(inp,  30, 31, 1, 20, "THIS IS A SECOND INPUT");
    tu_wnd_addfield(wndp, inp);

    tu_fld_initinput(inp,  40, 31, 2, 20, "THIS IS A THIRD INPUT");    
    tu_wnd_addfield(wndp, inp);
    
    tu_wnd_refresh(wndp);
    tu_wnd_setevent(wndp, on_event);

    tu_setwindow(wndp);

    tu_fld_delete(fldp);
    tu_fld_delete(inp);
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
    tu_shutdown();
    return 0;
}
