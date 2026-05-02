/*
File:       tb2uv.h
Purpose:    create a termbox2 and libuv for a simple Texu UI
Author:     Seree R.
Date:       27-APR-2026
*/
#ifndef __TERMBOX2_LIBUV__
#define __TERMBOX2_LIBUV__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tu_field     tu_field_t;     /*common field: label*/
typedef struct tu_input     tu_input_t;     /*input field*/
typedef struct tu_window    tu_window_t;    /*container*/

/*initialize*/
int             tu_init();
void            tu_shutdown();
void            tu_setcbreak(int cbreak);
int             tu_run();
tu_window_t*    tu_setwindow(tu_window_t* wnd);
tu_window_t*    tu_getwindow();
int             tu_addevent(int mod, int key, int ch, int id);
void            tu_removeevent(int mod, int key, int ch);

#define FIELD_MAX_TEXT                                      (128)
/*field->alignment (only one)*/
#define FIELD_LEFT                                          (0)
#define FIELD_CENTER                                        (1)
#define FIELD_RIGHT                                         (2)
/*field->attribs (one or more)*/
#define FIELD_BOLD                                          (0x0100)
#define FIELD_UNDERLINE                                     (0x0200)
#define FIELD_REVERSE                                       (0x0400)
#define FIELD_ITALIC                                        (0x0800)
#define FIELD_BLINK                                         (0x1000)
#define FIELD_HI_BLACK                                      (0x2000)
#define FIELD_BRIGHT                                        (0x4000)
#define FIELD_DIM                                           (0x8000)

#define FIELD_INPUT_NOECHO                                  (0x00000001)
/*field->fgcolor, bgcolor, fgdis, bgdis*/
#define FIELD_DEFAULT                                       (0x0000)
#define FIELD_BLACK                                         (0x0001)
#define FIELD_RED                                           (0x0002)
#define FIELD_GREEN                                         (0x0003)
#define FIELD_YELLOW                                        (0x0004)
#define FIELD_BLUE                                          (0x0005)
#define FIELD_MAGENTA                                       (0x0006)
#define FIELD_CYAN                                          (0x0007)
#define FIELD_WHITE                                         (0x0008)

/*simple draw*/
void            tu_format(char* dest, int limit, const char* src, int alignment);
void            tu_drawchar(int x, int y, char ch, int fg, int bg, int alignment, int attribs, int redraw);
void            tu_drawtext(int x, int y, int width, const char* text, int fg, int bg, int alignment, int attribs, int redraw);
void            tu_drawline(int x, int y, int width, char ch, int fg, int bg, int attribs, int redraw);
void            tu_drawvline(int x, int y, int height, char ch, int fg, int bg, int attribs, int redraw);
void            tu_drawbox(int x, int y, int width, int height, char chhorz, char chvert, char chcorner, int fg, int bg, int attribs, int redraw);
void            tu_fillbox(int x, int y, int width, int height, char ch, int fg, int bg, int redraw);
/*field*/
/*field type*/
enum
{
    FIELD_LABEL,
    FIELD_INPUT
};

tu_field_t*     tu_fld_new(int type);
void            tu_fld_delete(tu_field_t* fldp);
int             tu_fld_initlabel(tu_field_t* fldp, int id, int x, int y, int width, const char* text);
int             tu_fld_initinput(tu_field_t* fldp, int id, int x, int y, int width, const char* text);

int             tu_fld_setcolor(tu_field_t* fldp, int fg, int bg);
int             tu_fld_setattribs(tu_field_t* fldp, int attribs);
int             tu_fld_settext(tu_field_t* fldp, const char* text);
int             tu_fld_gettext(tu_field_t* fldp, char* text);
int             tu_fld_isenable(tu_field_t* fldp);
void            tu_fld_enable(tu_field_t* fldp, int enable);
int             tu_fld_isvisible(tu_field_t* fldp);
void            tu_fld_visible(tu_field_t* fldp, int visible);

int             tu_fld_draw(tu_field_t* fldp);
/*windows*/
tu_window_t*    tu_wnd_new();
void            tu_wnd_delete(tu_window_t* wndp);
/**
  <summary>
  Insert a copy of the user-specified
  data into a red black tree
  <summary>
  <param name="wndp">The window pointer to insert into</param>
  <param name="field">The field value to insert</param>
  <returns>
  1 if the value was inserted successfully,
  0 if the insertion failed for any reason
  </returns>
*/
int             tu_wnd_addfield(tu_window_t* wndp, tu_field_t* field);
void            tu_wnd_removefield(tu_window_t* wndp, int id);
tu_field_t*     tu_wnd_getfield(tu_window_t* wndp, int id);
tu_field_t*     tu_wnd_getactive(tu_window_t* wndp);
tu_field_t*     tu_wnd_setactive(tu_window_t* wndp, int id);


tu_field_t*     tu_wnd_getfirst(tu_window_t* wndp);
tu_field_t*     tu_wnd_getlast(tu_window_t* wndp);
tu_field_t*     tu_wnd_getnext(tu_window_t* wndp);
tu_field_t*     tu_wnd_getprev(tu_window_t* wndp);
void            tu_wnd_refresh(tu_window_t* wndp);
/*
int (*on_event)(int mod, int key, int ch, int id, void* data);
parameters:
    mod     - TB_MOD_xxx (see termbox2.h)
    key     - TB_KEY_xxx (see termbox2.h)
    ch      - unicode char
    id      - user defined
    data    - system will always send to the current active window pointer
returns:
    0 - to continue the global event
    otherwise - skip the global event and wait for the next event
*/
void            tu_wnd_setevent(tu_window_t* wndp, int (*on_event)(int mod, int key, int ch, void* data));

#ifdef __cplusplus
}
#endif


#endif /*__TERMBOX2_LIBUV__*/
