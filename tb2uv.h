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
typedef struct tu_wnditem   tu_wnditem_t;   /**/
typedef struct tu_label     tu_label_t;     /*input field*/
typedef struct tu_input     tu_input_t;     /*input field*/
typedef struct tu_listbox   tu_listbox_t;   /*listbox field*/
typedef struct tu_header    tu_header_t;    /*listbox header*/
typedef struct tu_subitem   tu_subitem_t;   /*listbox subitem*/
typedef struct tu_window    tu_window_t;    /*container*/
typedef struct tu_notify    tu_notify_t;    /*notified by tu_wnditem_t */

struct tu_field
{
    int     size;
    int     type;
    int     id;
    int     x;
    int     y;
    int     w;
    int     h;
    int     fgcolor;
    int     bgcolor;
    int     enable;
    int     visible;
    int     alignment;
    int     attribs;
    char*   text;
};

struct tu_header
{
    int     w;
    int     fgcolor;
    int     bgcolor;
    int     alignment;
    int     attribs;
    char*   text;
};

struct tu_subitem
{
    int     fgcolor;
    int     bgcolor;
    int     attribs;
    void*   data;
    char*   text;
};

struct tu_notify
{
    int     id;     /*notify from the object id*/
    void*   data;   /*pointer to the tu_wnditem_t */
    int     code;
};
/*initialize*/
int             tu_init();
void            tu_shutdown();
int             tu_run();
tu_window_t*    tu_setwindow(tu_window_t* wnd);
tu_window_t*    tu_getwindow();


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
/*input flags*/
#define FIELD_INPUT_NOECHO                                  (0x00000001)
#define FIELD_INPUT_PASSWORD                                (0x00000002)
#define FIELD_INPUT_NUMBER                                  (0x00000004)
#define FIELD_INPUT_HEXNUMBER                               (0x00000008)
#define FIELD_INPUT_CAPITAL                                 (0x00000010)
/*listbox flags*/
#define FIELD_LISTBOX_HIDEHEADER                            (0X00000001)
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
    FIELD_INPUT,
    FIELD_LISTBOX
};

int             tu_fld_initlabel(tu_field_t* fldp,   int id, int x, int y, int width, const char* text);
int             tu_fld_initinput(tu_field_t* fldp,   int id, int x, int y, int width);
int             tu_fld_initlistbox(tu_field_t* fldp, int id, int x, int y, int width, int height, const char* text);

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
tu_wnditem_t*   tu_wnd_addfield(tu_window_t* wndp, tu_field_t* field);
void            tu_wnd_removefield(tu_window_t* wndp, int id);
void            tu_wnd_clearfield(tu_window_t* wndp);
tu_wnditem_t*   tu_wnd_getfield(tu_window_t* wndp, int id);
tu_wnditem_t*   tu_wnd_getactive(tu_window_t* wndp);
tu_wnditem_t*   tu_wnd_setactive(tu_window_t* wndp, int id);

tu_wnditem_t*   tu_wnd_getfirst(tu_window_t* wndp);
tu_wnditem_t*   tu_wnd_getlast(tu_window_t* wndp);
tu_wnditem_t*   tu_wnd_getnext(tu_window_t* wndp);
tu_wnditem_t*   tu_wnd_getprev(tu_window_t* wndp);
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
enum
{
    FIELD_EV_KEYDOWN,   /*all key down probe*/
    FIELD_EV_BLUR,      /*sent by its child*/
    FIELD_EV_FOCUS,     /*sent by its child*/
    FIELD_EV_NOTIFY     /*sent by its child (e.g. pressed ENTER, item changed)*/
};
enum
{
    FIELD_NOTIFY_PRESSEDENTER,
    FIELD_NOTIFY_ITEMCHANGED
};

void            tu_wnd_setevent(tu_window_t* wndp, int event,
                    int (*on_event)(int mod, int key, int ch, tu_notify_t* notify));

/*wnditem*/
int             tu_wnditem_setcolor(tu_wnditem_t* itemp, int fg, int bg);
int             tu_wnditem_setattribs(tu_wnditem_t* itemp, int attribs);
int             tu_wnditem_settext(tu_wnditem_t* itemp, const char* text);
int             tu_wnditem_gettext(tu_wnditem_t* itemp, char* text, int len);
int             tu_wnditem_isenable(tu_wnditem_t* itemp);
void            tu_wnditem_enable(tu_wnditem_t* itemp, int enable);
int             tu_wnditem_isvisible(tu_wnditem_t* itemp);
void            tu_wnditem_visible(tu_wnditem_t* itemp, int visible);
void            tu_wnditem_draw(tu_wnditem_t* itemp);
tu_window_t*    tu_wnditem_getparent(tu_wnditem_t* itemp);
void            tu_wnditem_setflags(tu_wnditem_t* itemp, unsigned int flags);
unsigned int    tu_wnditem_getflags(tu_wnditem_t* itemp);

/*input*/
void            tu_inp_setlimit(tu_input_t* inp, int limit);
int             tu_inp_setpassword(tu_input_t* inp, const char* password);
int             tu_inp_getpassword(tu_input_t* inp, char* password, int len);
int             tu_inp_setnumber(tu_input_t* inp, int number);
int             tu_inp_getnumber(tu_input_t* inp);
/*listbox*/
void            tu_lbx_reset(tu_listbox_t* lbxp);
int             tu_lbx_addheader(tu_listbox_t* lbxp, tu_header_t* hdrp, int nheaders);
int             tu_lbx_add(tu_listbox_t* lbxp, tu_subitem_t* itemp, int nitems);
int             tu_lbx_remove(tu_listbox_t* lbxp, int row);
int             tu_lbx_get(tu_listbox_t* lbxp, int row, tu_subitem_t* itemp, int nitems);
int             tu_lbx_set(tu_listbox_t* lbxp, int row, tu_subitem_t* itemp, int nitems);
int             tu_lbx_getitem(tu_listbox_t* lbxp, int row, int col, tu_subitem_t* itemp);
int             tu_lbx_setitem(tu_listbox_t* lbxp, int row, int col, tu_subitem_t* itemp);
void            tu_lbx_clear(tu_listbox_t* lbxp);
int             tu_lbx_setcursel(tu_listbox_t* lbxp, int row, int redraw);
int             tu_lbx_getcursel(tu_listbox_t* lbxp);
void            tu_lbx_sort(tu_listbox_t* lbxp, int col);

#ifdef __cplusplus
}
#endif


#endif /*__TERMBOX2_LIBUV__*/
