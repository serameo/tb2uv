#include <stdio.h>
#include <unistd.h>

#include <uv.h>
#define TB_IMPL
extern int errno;
#include "termbox2.h"
#include "jsw_rbtree.h"
#include "cJSON.h"
#include "tu_list.h"
#include "tb2uv.h"

/*to hold all environment data*/
struct tu__event
{
    struct tb_event ev; /*termbox2: event*/
    int             id; /*user defined*/
};
struct tu__env 
{
    uv_poll_t       poll_handler;       /*libuv: input stream*/
    uv_loop_t*      main_loop;          /*libuv: loop*/
    tu_window_t*    active_wnd;         /*hold windows*/
    jsw_rbtree_t*   events;             /*hold the registered events*/
};
static int   tu_env__cmp ( const void *p1, const void *p2 )
{
    struct tu__event* ev1 = (struct tu__event*)p1;
    struct tu__event* ev2 = (struct tu__event*)p2;
    return  (ev1->ev.key == ev2->ev.key) && 
            (ev1->ev.ch  == ev2->ev.ch) && 
            (ev1->ev.mod == ev2->ev.mod);
}
static void *tu_env__dup ( void *p1 )
{
    struct tu__event* ev1 = (struct tu__event*)p1;
    struct tu__event* evp = (struct tu__event*)calloc(1, sizeof(struct tu__event));
    if (evp)
    {
        evp->ev.type    = TB_EVENT_KEY;
        evp->ev.mod     = ev1->ev.mod;
        evp->ev.key     = ev1->ev.key;
        evp->ev.ch      = ev1->ev.ch;
        evp->id         = ev1->id;
    }
    return evp;
}
static void  tu_env__rel ( void *p )
{
    free(p);
}

struct tu__env* g_envp = NULL;    /*global termbox2 libuv environment*/
char g_blank[FIELD_MAX_TEXT + 1] = "";

static struct tu__env*   tu__getinstance()
{
    if (g_envp)
    {
        return g_envp;
    }
    g_envp = (struct tu__env*)calloc(1, sizeof(struct tu__env));
    return g_envp;
}

struct tu_wnditem
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
    char    text[FIELD_MAX_TEXT + 1];
    tu_listnode_t*    node;
};
typedef struct tu_wnditem tu_wnditem_t;

struct tu_input
{
    struct tu_wnditem item;      /*based object*/
    /*color*/
    int     fgdis;
    int     bgdis;
    /*focus*/
    int     first_focus;
    /*cursor*/
    int  xcur;
    int  ycur;
    /*position*/
    int  xpos;
    int  ypos;
    /*flags*/
    unsigned int flags;
};

struct tu_listheader
{
    int     w;
    int     fgcolor;
    int     bgcolor;
    int     alignment;
    int     attribs;
    char    text[FIELD_MAX_TEXT + 1];
};
typedef struct tu_listheader tu_listheader_t;

struct tu_listsubitem
{
    int     fgcolor;
    int     bgcolor;
    int     fgsel;
    int     bgsel;
    int     attribs;
    void*   data;
    char    text[FIELD_MAX_TEXT + 1];
    tu_listnode_t*    node;
};
typedef struct tu_listsubitem tu_listsubitem_t;

struct tu_listbox
{
    struct tu_wnditem     item;      /*based object*/
    /*header*/
    tu_listheader_t*    hdrp;
    int                 nheaders;
    /*rows*/
    tu_linklist_t*      rowp;       /*to point the number of rows*/
    int                 nrows;
    /*index*/
    int                 currow;     /*TB_KEY_UP | TB_KEY_DOWN | TB_KEY_PAGEUP | TB_KEY_PAGEDOWN*/
    int                 curcol;     /*changed when pressed TB_KEY_LEFT or TB_KEY_RIGHT*/
    /*flags*/
    unsigned int        flags;
};
static int   tu_lbx__cmp ( const void *p1, const void *p2 )
{
    return 0;
}
static void *tu_lbx__dup ( void *p1 )
{
    return 0;
}
static void  tu_lbx__rel ( void *p )
{
}


struct tu_window
{
    jsw_rbtree_t*       children;       /*hold all tu_fields*/
    jsw_rbtrav_t*       childtrav;
    tu_linklist_t*      fields;         /*to handle movable children*/
    tu_listnode_t*      nodes;
    tu_wnditem_t*      active_input;
    int                 (*on_event)(int mod, int key, int ch, void* data);
};
static int   tu_wnd__cmp ( const void *p1, const void *p2 )
{
    tu_wnditem_t* f1 = (tu_wnditem_t*)p1;
    tu_wnditem_t* f2 = (tu_wnditem_t*)p2;
    return (f1->id - f2->id);
}
static void *tu_wnd__dup ( void *p1 )
{
    tu_wnditem_t* f1   = (tu_wnditem_t*)p1;
    tu_wnditem_t* itemp = (tu_wnditem_t*)calloc(1, f1->size);
    if (itemp)
    {
        memcpy(itemp, f1, f1->size);
    }
    return itemp;
}
static void  tu_wnd__rel ( void *p )
{
    free(p);
}
static tu_wnditem_t* tu_wnd__getinput(tu_window_t* wndp)
{
    return (wndp ? wndp->active_input : NULL);
}

int tu_input__draw(tu_wnditem_t* itemp);

static tu_wnditem_t* tu_wnd__getnextinput(tu_window_t* wndp, int dir)
{
    tu_wnditem_t* itemp = (wndp ? wndp->active_input : NULL);
    /*tu_listtrav_t* iter = tu_listtrav_new();
    tu_listtrav_clone(iter, wndp->active_iter);*/
    tu_listnode_t*  node = 0;
    
    while (itemp)
    {
        node = itemp->node;
        if (dir < 0)
        {
            node = tu_list_prev(node);
        }
        else /*if (dir >= 0)*/
        {
            node = tu_list_next(node);
        }
        if (NULL == node)
        {
            break;
        }
        itemp = (tu_wnditem_t*)tu_list_data(node);
        if (itemp && itemp->type == FIELD_INPUT && itemp->enable && itemp->visible)
        {
            break;
        }
    }
    /*moved successfully*/
    if (itemp && itemp != wndp->active_input)
    {
        tu_input_t* inp = (tu_input_t*)wndp->active_input;
        inp->first_focus = 0;
        tu_input__draw(wndp->active_input);
        
        /*tu_listtrav_clone(wndp->active_iter, iter);*/
        
        inp = (tu_input_t*)itemp;
        inp->first_focus = 1;
        tu_input__draw(itemp);
        
        wndp->active_input = itemp;
    }
    
    /*tu_listtrav_delete(iter);*/
    return itemp;
}

static void field_draw(int x, int y, int width, const char* text, int fg, int bg, int aligment, int attribs, int redraw);
static void field_format_text(char* dest, int limit, const char* src, int alignment);
int         field_process_event(tu_wnditem_t* itemp, struct tb_event* evp);


void tu_wnditem_draw(tu_wnditem_t* itemp)
{
    if (itemp->visible == 0)
    {
        return;
    }
    if (itemp->type == FIELD_INPUT)
    {
        tu_input__draw(itemp);
        return;
    }
    tu_drawtext(itemp->x, itemp->y, itemp->w, 
        itemp->text, 
        itemp->fgcolor, itemp->bgcolor, 
        itemp->alignment, itemp->attribs, 1);
}

static void on_termbox_event(uv_poll_t* handle, int status, int events)
{
    struct tb_event ev;
    struct tu__env* envp = (struct tu__env*)handle->data;
    tu_window_t* wndp = tu_getwindow(envp);
    int rc = tb_peek_event(&ev, 0);

    if (rc == TB_OK && wndp)
    {
        if (ev.type == TB_EVENT_KEY)
        {
            /*check the active window processing the key if it is registered the event*/
            rc = 0;
            if (wndp->on_event)
            {
                rc = wndp->on_event(ev.mod, ev.key, ev.ch, wndp);
            }
            if (rc != 0)
            {
                return; /*alreay processed by wndp*/
            }
            /*the global event*/
            if (ev.key == TB_KEY_CTRL_C)
            {
                tu_shutdown();
            }
            else if (ev.key == TB_KEY_ENTER)
            {
                /*process a default action in the active window*/
            }
            else if (ev.key == TB_KEY_BACK_TAB)
            {
                tu_wnditem_t* nextitemp = tu_wnd__getnextinput(wndp, -1);
            }
            else if (ev.key == TB_KEY_TAB)
            {
                tu_wnditem_t* nextitemp = tu_wnd__getnextinput(wndp, 1);
            }
            else
            {
                tu_wnditem_t* curitemp = tu_wnd__getinput(wndp);
                if (curitemp)
                {
                    field_process_event(curitemp, &ev);
                    tu_wnditem_draw(curitemp);
                }
            }
        }
    }
    else if (TB_OK == rc)
    {
        /*the global event*/
        if (ev.key == TB_KEY_CTRL_C)
        {
            tu_shutdown();
        }
    }
}

int     tu_init()
{
    struct tu__env* envp = tu__getinstance();
    /*termbox2*/
    tb_init();
    /*tb_set_input_mode(TB_INPUT_ALT);*/
    envp->events = jsw_rbnew(tu_env__cmp, tu_env__dup, tu_env__rel);

    /*libuv*/
    /*tu__getinstance(); */ /*first init environment*/
    envp->main_loop = uv_default_loop();
    
    uv_poll_init(envp->main_loop, &envp->poll_handler, STDIN_FILENO);
    envp->poll_handler.data = envp;
    uv_poll_start(&envp->poll_handler, UV_READABLE, on_termbox_event);
    
    /*windows*/
    envp->active_wnd = NULL;
    
    /*others*/
    memset(g_blank, ' ', sizeof(g_blank));
    g_blank[FIELD_MAX_TEXT] = 0;
    return 0;
}

void tu_shutdown()
{
    struct tu__env* envp = tu__getinstance();
    /*free*/
    tu_window_t* wndp = envp->active_wnd;
    if (wndp)
    {
        tu_wnd_delete(wndp);
    }

    /*libs*/
    tb_shutdown();
    uv_stop(uv_default_loop());
}

void    tu_setcbreak(int cbreak)
{
    int fd = 0;
    int szfd = 0;
    struct termios tios;
    memset(&tios, 0, sizeof(tios));
    tcgetattr(fd, &tios);

    tb_get_fds(&fd, &szfd);
    if (cbreak)
    {
        tios.c_lflag |= (ISIG);
    }
    else
    {
        tios.c_lflag &= ~(ISIG);
    }
    tcsetattr(fd, TCSAFLUSH, &tios);
}

int     tu_run()
{
    struct tu__env* envp = tu__getinstance();
    uv_run(envp->main_loop, UV_RUN_DEFAULT);
    return 0;
}

tu_window_t*    tu_setwindow(tu_window_t* wndp)
{
    struct tu__env* envp = tu__getinstance();
    tu_window_t* oldwndp = NULL;
    oldwndp = envp->active_wnd;
    envp->active_wnd = wndp;
    return oldwndp;
}

tu_window_t*    tu_getwindow()
{
    struct tu__env* envp = tu__getinstance();
    return envp->active_wnd;
}

int tu_addevent(int mod, int key, int ch, int id)
{
    int rc = 0;
    struct tu__env* envp = tu__getinstance();
    struct tu__event ev;

    memset(&ev, 0, sizeof(ev));
    ev.ev.mod = mod;
    ev.ev.key = key;
    ev.ev.ch  = ch;
    ev.id     = id;
    rc = jsw_rbinsert(envp->events, &ev);
    return rc;
}
void    tu_removeevent(int mod, int key, int ch)
{
    struct tu__env* envp = tu__getinstance();
    struct tu__event ev;

    memset(&ev, 0, sizeof(ev));
    ev.ev.mod = mod;
    ev.ev.key = key;
    ev.ev.ch  = ch;
    jsw_rberase(envp->events, &ev);
}

static void field_format_text(char* dest, int limit, const char* src, int alignment)
{
    int len = strlen(src);
    int mid = 0;

    memset(dest, ' ', limit);
    dest[limit] = 0;

    if (alignment == FIELD_RIGHT)
    {
        if (len >= limit)
        {
            mid = (len - limit);
            strncpy(dest, &src[mid], limit);
        }
        else
        {
            mid = (limit - len);
            strncpy(&dest[mid], src, len);
        }
    }
    else if (alignment == FIELD_CENTER)
    {
        if (len >= limit)
        {
            strncpy(dest, src, limit);
        }
        else
        {
            mid = (limit - len)/2;
            strncpy(&dest[mid], src, len);
        }
    }
    else
    {
        len = (limit < len ? limit : len);
        strncpy(dest, src, len);
    }
}

static void field_draw(int x, int y, int width, const char* text, int fg, int bg, int alignment, int attribs, int redraw)
{
    char buffer[FIELD_MAX_TEXT + 1];
    
    /*add attributes*/
    if (attribs & FIELD_BOLD)
    {
        fg |= TB_BOLD;
    }
    if (attribs & FIELD_UNDERLINE)
    {
        fg |= TB_UNDERLINE;
    }
    if (attribs & FIELD_REVERSE)
    {
        fg |= TB_REVERSE;
    }
    if (attribs & FIELD_ITALIC)
    {
        fg |= TB_BOLD;
    }
    if (attribs & FIELD_BLINK)
    {
        fg |= TB_BLINK;
    }

    /*draw*/
    field_format_text(buffer, width, text, alignment);
    tb_print(x, y, fg, bg, buffer);

    if (redraw)
    {
        tb_present();
    }
}

static void field_filltext(int x, int y, int width, int height, const char* text, int fg, int bg, int alignment, int attribs, int redraw)
{
}

void tu_drawtext(int x, int y, int width, const char* text, int fg, int bg, int alignment, int attribs, int redraw)
{
    field_draw(x, y, width, text, fg, bg, alignment, attribs, redraw);
}

void tu_drawchar(int x, int y, char ch, int fg, int bg, int alignment, int attribs, int redraw)
{
    char sz[2] = { ch, 0 };
    tu_drawtext(x, y, 1, sz, fg, bg, FIELD_LEFT, attribs, redraw);
}

void tu_format(char* dest, int limit, const char* src, int alignment)
{
    field_format_text(dest, limit, src, alignment);
}

void tu_drawline(int x, int y, int width, char ch, int fg, int bg, int attribs, int redraw)
{
    char line[FIELD_MAX_TEXT + 1];
    memset(line, ch, sizeof(line));
    line[FIELD_MAX_TEXT] = 0;
    tu_drawtext(x, y, width, line, fg, bg, FIELD_LEFT, attribs, redraw);
}

void tu_drawvline(int x, int y, int height, char ch, int fg, int bg, int attribs, int redraw)
{
    char sz[2] = { ch, 0 };
    int i = 0;
    for (i = 0; i < height; ++i)
    {
        tu_drawtext(x, y + i, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    }
    if (redraw)
    {
        tb_present();
    }
}

void tu_drawbox(int x, int y, int width, int height, char chhorz, char chvert, char chcorner, int fg, int bg, int attribs, int redraw)
{
    char sz[2] = { chcorner, 0 };
    /*upper-left corner*/
    tu_drawtext(x, y, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    /*upper-right corner*/
    tu_drawtext(x + width, y, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    /*lower-left corner*/
    tu_drawtext(x, y + height, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    /*lower-right corner*/
    tu_drawtext(x + width, y + height, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    
    /*draw upper line*/
    tu_drawline(x + 1, y, width - 1, chhorz, fg, bg, attribs, 0);
    /*draw lower line*/
    tu_drawline(x + 1, y + height, width - 1, chhorz, fg, bg, attribs, 0);
    
    /*draw left line*/
    tu_drawvline(x, y + 1, height - 1, chvert, fg, bg, attribs, 0);
    /*draw right line*/
    tu_drawvline(x + width, y + 1, height - 1, chvert, fg, bg, attribs, 0);
    if (redraw)
    {
        tb_present();
    }
}

void tu_fillbox(int x, int y, int width, int height, char ch, int fg, int bg, int redraw)
{
    int i = 0;
    memset(g_blank, ch, sizeof(g_blank));
    g_blank[FIELD_MAX_TEXT] = 0;
    for (i = 0; i < height; ++i)
    {
        tu_drawtext(x, y + i, width, g_blank, fg, bg, 0, 0, 0);
    }
    if (redraw)
    {
        tb_present();
    }
}

#define INIT_FIELD_MEMBER(field_ptr, field_size, id, typ, x, y, w, txt)  \
do {                                            \
    memset((field_ptr), 0, sizeof(struct tu_field));    \
    field_ptr->size         = (field_size);     \
    field_ptr->id           = (id);             \
    field_ptr->type         = (typ);            \
    field_ptr->x            = (x);              \
    field_ptr->y            = (y);              \
    field_ptr->w            = (w);              \
    field_ptr->h            = (1);              \
    field_ptr->fgcolor      = (0);              \
    field_ptr->bgcolor      = (0);              \
    field_ptr->enable       = (1);              \
    field_ptr->visible      = (1);              \
    field_ptr->alignment    = (0);              \
    field_ptr->attribs      = (0);              \
    field_ptr->text         = (char*)txt;       \
} while (0)
/*factory*/

int tu_fld_initlabel(tu_field_t* fldp, int id, int x, int y, int w, const char* text)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_wnditem), id, FIELD_LABEL, x, y, w, text);
    fldp->enable = 0; /*always disable*/
    return 0;
}

int tu_fld_initinput(tu_field_t* fldp, int id, int x, int y, int w, const char* text)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_input), id, FIELD_INPUT, x, y, w, text);
    return 0;
}

int tu_fld_initlistbox(tu_field_t* fldp, int id, int x, int y, int w, int height)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_listbox), id, FIELD_LISTBOX, x, y, w, "");
    fldp->h = height; 
    return 0;
}

int tu_wnditem_setcolor(tu_wnditem_t* itemp, int fg, int bg)
{
    itemp->fgcolor = fg;
    itemp->bgcolor = bg;
    return 0;
}

int tu_wnditem_setattribs(tu_wnditem_t* itemp, int attribs)
{
    itemp->attribs = attribs;
    return 0;
}

int tu_wnditem_settext(tu_wnditem_t* itemp, const char* text)
{
    strncpy(itemp->text, text, FIELD_MAX_TEXT);
    return 0;
}

int tu_wnditem_gettext(tu_wnditem_t* itemp, char* text)
{
    strncpy(text, itemp->text, FIELD_MAX_TEXT);
    return 0;
}

int tu_wnditem_isenable(tu_wnditem_t* itemp)
{
    return itemp->enable;
}

void tu_wnditem_enable(tu_wnditem_t* itemp, int enable)
{
    itemp->enable = enable;
}

int tu_wnditem_isvisible(tu_wnditem_t* itemp)
{
    return itemp->visible;
}

void tu_wnditem_visible(tu_wnditem_t* itemp, int visible)
{
    itemp->visible = visible;
}

int tu_input__draw(tu_wnditem_t* itemp)
{
    tu_input_t* input = (tu_input_t*)itemp;
    int  x = itemp->x;
    int  y = itemp->y;
    int  width = itemp->w;
    int  fg = (itemp->enable ? itemp->fgcolor : input->fgdis);
    int  bg = (itemp->enable ? itemp->bgcolor : input->bgdis);
    char text[FIELD_MAX_TEXT + 1] = "";
    int  len = 0;
    unsigned int flags = input->flags;

    if (input->first_focus)
    {
        fg |= FIELD_REVERSE;
        bg |= FIELD_REVERSE;
    }

    if (flags & FIELD_INPUT_NOECHO)
    {
        text[0] = 0; /*no drawing*/
    }
    else
    {
        memcpy(text, &itemp->text[input->xpos], width);
    }
    field_draw(itemp->x, itemp->y, itemp->w, 
        text,
        fg, bg, 
        itemp->alignment, itemp->attribs, 0);
    /*cursor*/
    len = strlen(text);
    x = x + len;
    tb_set_cursor(x, y);

    tb_present();
}

int tu_fld_draw(tu_field_t* fldp)
{
    if (fldp->visible == 0)
    {
        return 0;
    }
    /*if (fldp->type == FIELD_INPUT)
    {
        tu_input__draw(fldp);
        return 0;
    }*/
    tu_drawtext(fldp->x, fldp->y, fldp->w, 
        fldp->text, 
        fldp->fgcolor, fldp->bgcolor, 
        fldp->alignment, fldp->attribs, 1);
    return 0;
}

int tu_input__process_event(tu_wnditem_t* itemp, struct tb_event* ev)
{
    int len   = 0;
    tu_input_t* inp = (tu_input_t*)itemp;
    int width = itemp->w;

    if (!itemp->enable || !itemp->visible)
    {
        return 0;
    }
    if (inp->first_focus)
    {
        inp->xpos = 0;
        inp->xcur = 0;
        memset(itemp->text, 0, FIELD_MAX_TEXT);
        itemp->text[FIELD_MAX_TEXT] = 0;

        if (ev->ch)
        {
            itemp->text[inp->xcur] = ev->ch;
            ++inp->xcur;
        }
        inp->first_focus = 0;
        return 0;
    }
    if (ev->key)
    {
        switch (ev->key)
        {
            case TB_KEY_BACKSPACE: 
            case TB_KEY_BACKSPACE2:
            {
                len = strlen(itemp->text);
                if (len > width)
                {
                    --inp->xpos;
                }
                else
                {
                    inp->xpos = 0;
                }
                if (inp->xcur > 0)
                {
                    --inp->xcur;
                    itemp->text[inp->xcur] = 0;
                }
                break;
            }
        }
    }
    else if (ev->ch)
    {
        if (inp->xcur < FIELD_MAX_TEXT)
        {
            itemp->text[inp->xcur] = ev->ch;
            ++inp->xcur;
            len = strlen(itemp->text);
            if (len >= width)
            {
                inp->xpos = len - width;
            }
        }
    }
    return 1;
}

int field_process_event(tu_wnditem_t* itemp, struct tb_event* evp)
{
    switch (itemp->type)
    {
        case FIELD_INPUT:
            return tu_input__process_event(itemp, evp);
    }
    return 0;
}

tu_window_t*    tu_wnd_new()
{
    tu_window_t* wndp = (tu_window_t*)calloc(1, sizeof(tu_window_t));
    if (wndp)
    {
        wndp->children = jsw_rbnew(tu_wnd__cmp, tu_wnd__dup, tu_wnd__rel);
        if (NULL == wndp->children)
        {
            free(wndp);
            return NULL;
        }
        wndp->childtrav     = jsw_rbtnew();
        wndp->fields        = tu_list_new(tu_wnd__cmp, tu_wnd__dup, tu_wnd__rel);
        /*wndp->itertrav      = tu_listtrav_new();
        wndp->active_iter   = tu_listtrav_new();*/
    }
    return wndp;
}

void tu_wnd_delete(tu_window_t* wndp)
{
    tu_wnd_clearfield(wndp);
    if (wndp->fields)
    {
        tu_list_delete(wndp->fields);
        wndp->fields = NULL;
    }
    if (wndp->childtrav)
    {
        jsw_rbtdelete(wndp->childtrav);
        wndp->childtrav = NULL;
    }
    if (wndp->children)
    {
        jsw_rbdelete(wndp->children);
        wndp->children = NULL;
    }
    free(wndp);
}

void tu_wnd_clearfield(tu_window_t* wndp)
{
    tu_wnditem_t* itemp = 0;
    tu_listnode_t* node = tu_list_first(wndp->fields);
    while (node)
    {
        itemp = (tu_wnditem_t*)tu_list_data(node);
        if (itemp->type == FIELD_LISTBOX)
        {
            tu_lbx_reset((tu_listbox_t*)itemp);
        }
        jsw_rberase(wndp->children, itemp);
        tu_list_remove(wndp->fields, node, 0);
        
        node = tu_list_first(wndp->fields);
    }
}

void tu_wnd_removefield(tu_window_t* wndp, int id)
{
    tu_wnditem_t* itemp = tu_wnd_getfield(wndp, id);
    
    if (itemp)
    {
        if (itemp == wndp->active_input)
        {
            tu_wnditem_t* nextp = tu_wnd__getnextinput(wndp, -1);
            if (itemp == nextp)
            {
                /*no prev*/
                nextp = tu_wnd__getnextinput(wndp, 1);
            }
            if (itemp == nextp)
            {
                /*no next: only one input active*/
                wndp->active_input = NULL;
            }
        }
        tu_list_erase(wndp->fields, itemp, 0);
        
        if (itemp->type == FIELD_LISTBOX)
        {
            tu_lbx_reset((tu_listbox_t*)itemp);
        }
        jsw_rberase(wndp->children, itemp);
    }
}

tu_wnditem_t* tu_wnd_getfield(tu_window_t* wndp, int id)
{
    tu_wnditem_t item;
    item.id = id;
    tu_wnditem_t* itemp = jsw_rbfind(wndp->children, &item);
    return itemp;
}

#if 0
struct tu_input
{
    struct tu_wnditem item;      /*based object*/
    /*color*/
    int     fgdis;
    int     bgdis;
    /*focus*/
    int     first_focus;
    /*cursor*/
    int  xcur;
    int  ycur;
    /*position*/
    int  xpos;
    int  ypos;
    /*flags*/
    unsigned int flags;
};

struct tu_listbox
{
    struct tu_wnditem     item;      /*based object*/
    /*header*/
    tu_listheader_t*    hdrp;
    int                 nheaders;
    /*rows*/
    tu_linklist_t*      rowp;       /*to point the number of rows*/
    int                 nrows;
    /*index*/
    int                 currow;     /*TB_KEY_UP | TB_KEY_DOWN | TB_KEY_PAGEUP | TB_KEY_PAGEDOWN*/
    int                 curcol;     /*changed when pressed TB_KEY_LEFT or TB_KEY_RIGHT*/
    /*flags*/
    unsigned int        flags;
};
#endif
void tu_wnditem__init(tu_wnditem_t* itemp)
{
    switch (itemp->type)
    {
        case FIELD_INPUT:
        {
            tu_input_t* inp = (tu_input_t*)itemp;
            inp->fgdis  = 0;
            inp->bgdis  = 0;
            inp->first_focus = 0;
            inp->xcur   = 0;
            inp->ycur   = 0;
            inp->xpos   = 0;
            inp->ypos   = 0;
            inp->flags  = 0;
            break;
        }
        case FIELD_LISTBOX:
        {
            tu_listbox_t* lbxp = (tu_listbox_t*)itemp;
            lbxp->hdrp      = NULL;
            lbxp->nheaders  = 0;
            lbxp->rowp      = NULL; /*to point the number of rows*/
            lbxp->nrows     = 0;
            lbxp->currow    = -1;   /*TB_KEY_UP | TB_KEY_DOWN | TB_KEY_PAGEUP | TB_KEY_PAGEDOWN*/
            lbxp->curcol    = 0;    /*changed when pressed TB_KEY_LEFT or TB_KEY_RIGHT*/
            lbxp->flags     = 0;
            break;
        }
    }
}

tu_wnditem_t*  tu_wnd_addfield(tu_window_t* wndp, tu_field_t* fldp)
{
    tu_wnditem_t*   newitemp = 0;

    tu_wnditem_t    item;
    int             rc = 0;
    memset(&item, 0, sizeof(struct tu_wnditem));
    memcpy(&item, fldp, sizeof(struct tu_field));
    strcpy(item.text, fldp->text);
    rc = jsw_rbinsert(wndp->children, &item);
    if (1 == rc) /*insert successfully*/
    {
        newitemp = (tu_wnditem_t*)jsw_rbfind(wndp->children, &item);
        tu_list_pushback(wndp->fields, newitemp, 0);
        newitemp->node = tu_list_last(wndp->fields);
        
        tu_wnditem__init(newitemp);
        /*active_input*/
        if ( NULL == wndp->active_input )
        {
            if (newitemp->enable && newitemp->visible)
            {
                if (newitemp->type == FIELD_INPUT)
                {
                    tu_input_t* inp = (tu_input_t*)newitemp;
                    inp->first_focus = 1;
                }
                wndp->active_input = newitemp;
            }
        }
    }
    return newitemp;
}

tu_wnditem_t* tu_wnd_getactive(tu_window_t* wndp)
{
    return (wndp->active_input);
}

tu_wnditem_t* tu_wnd_setactive(tu_window_t* wndp, int id)
{
    tu_wnditem_t* activep = tu_wnd_getactive(wndp);
    tu_wnditem_t* newitemp = tu_wnd_getfield(wndp, id);
    if (NULL == newitemp)
    {
        if (newitemp->enable && newitemp->visible)
        {
            tu_input_t* inp = 0;
            if (activep->type == FIELD_INPUT)
            {
                inp = (tu_input_t*)wndp->active_input;
                inp->first_focus = 0;
                tu_wnditem_draw(wndp->active_input);
            }
            if (newitemp->type == FIELD_INPUT)
            {
                inp = (tu_input_t*)wndp->active_input;
                inp->first_focus = 1;
                tu_wnditem_draw(newitemp);
            }
            wndp->active_input = newitemp;
        }
    }
    return activep;
}

tu_wnditem_t*     tu_wnd_getfirst(tu_window_t* wndp)
{
    tu_wnditem_t* itemp = 0;
    wndp->nodes = tu_list_first(wndp->fields);
    itemp = (tu_wnditem_t*)tu_list_data(wndp->nodes);
    return itemp;
}

tu_wnditem_t*     tu_wnd_getlast(tu_window_t* wndp)
{
    tu_wnditem_t* itemp = 0;
    wndp->nodes = tu_list_last(wndp->fields);
    itemp = (tu_wnditem_t*)tu_list_data(wndp->nodes);
    return itemp;
}

tu_wnditem_t*     tu_wnd_getnext(tu_window_t* wndp)
{
    tu_wnditem_t* itemp = 0;
    wndp->nodes = tu_list_next(wndp->nodes);
    itemp = (tu_wnditem_t*)tu_list_data(wndp->nodes);
    return itemp;
}

tu_wnditem_t*     tu_wnd_getprev(tu_window_t* wndp)
{
    tu_wnditem_t* itemp = 0;
    wndp->nodes = tu_list_prev(wndp->nodes);
    itemp = (tu_wnditem_t*)tu_list_data(wndp->nodes);
    return itemp;
}

void tu_wnd_refresh(tu_window_t* wndp)
{
    tu_wnditem_t* itemp = tu_wnd_getfirst(wndp);
    while (itemp)
    {
        tu_wnditem_draw(itemp);
        itemp = tu_wnd_getnext(wndp);
    }
}

void tu_wnd_setevent(tu_window_t* wndp, int (*on_event)(int mod, int key, int ch, void* data))
{
    wndp->on_event = on_event;
}

/*listbox*/
int tu_lbx_addheader(tu_listbox_t* lbxp, tu_header_t* hdrp, int nheaders)
{
    int i = 0;
    tu_listheader_t* headers = 0;
    if (lbxp->hdrp)
    {
        return 1;
    }
    headers = (tu_listheader_t*)calloc(nheaders, sizeof(struct tu_listheader));
    if (NULL == headers)
    {
        return -1;
    }

    if (NULL == lbxp->rowp)
    {
        lbxp->rowp = tu_list_new(tu_lbx__cmp, tu_lbx__dup, tu_lbx__rel); /*no cmp, dup, rel*/
    }
    for (i = 0; i < nheaders; ++i)
    {
        headers[i].w            = hdrp[i].w;
        headers[i].fgcolor      = hdrp[i].fgcolor;
        headers[i].bgcolor      = hdrp[i].bgcolor;
        headers[i].alignment    = hdrp[i].alignment;
        headers[i].attribs      = hdrp[i].attribs;
        strcpy(headers[i].text, hdrp[i].text);
    }
    lbxp->hdrp = headers;
    lbxp->nheaders = nheaders;
    return 0;
}

int tu_lbx_add(tu_listbox_t* lbxp, tu_subitem_t* itemp, int nitems)
{
    int i   = 0;
    int rc  = 0;
    int idx = lbxp->nrows;
    tu_listsubitem_t* subitemp = 0;
    /*required the header*/
    if (NULL == lbxp->hdrp)
    {
        return -1;
    }
    /*allocate the new items*/
    subitemp = (tu_listsubitem_t*)calloc(nitems, sizeof(struct tu_listsubitem));
    if (NULL == subitemp)
    {
        return -2;
    }
    /*set the item into the list*/
    rc = tu_list_pushback(lbxp->rowp, subitemp, 0); /*not cloned*/
    if (rc != 0)
    {
        free(subitemp);
        return -3;
    }
    subitemp->node = tu_list_last(lbxp->rowp);
    /*copy data*/
    for (i = 0; i < nitems; ++i)
    {
        subitemp[i].fgcolor      = itemp[i].fgcolor;
        subitemp[i].bgcolor      = itemp[i].bgcolor;
        subitemp[i].fgsel        = itemp[i].fgsel;
        subitemp[i].bgsel        = itemp[i].bgsel;
        subitemp[i].data         = itemp[i].data;
        strcpy(subitemp[i].text, itemp[i].text);
    }
    /*increment the row number*/
    ++lbxp->nrows;
    return idx;
}

tu_listsubitem_t*   tu_lbx__get(tu_listbox_t* lbxp, int idx)
{
    int i = 0;
    tu_listnode_t* node = tu_list_first(lbxp->rowp);
    tu_listsubitem_t* subitemp = 0;
    if (idx < 0 || idx >= lbxp->nrows)
    {
        return NULL;
    }
    while (node && i != idx)
    {
        node = tu_list_next(node);
        ++i;
    }
    if (node && i == idx)
    {
        subitemp = (tu_listsubitem_t*)tu_list_data(node);
    }
    return subitemp; 
}

int tu_lbx_remove(tu_listbox_t* lbxp, int idx)
{
    tu_listsubitem_t* subitemp = tu_lbx__get(lbxp, idx);
    if (subitemp)
    {
        tu_list_remove(lbxp->rowp, subitemp->node, 0);
        free(subitemp);
        
        --lbxp->nrows;
        if (lbxp->nrows <= 0)
        {
            lbxp->nrows = 0;
            lbxp->currow = -1;
        }
    }
    return 0;
}

int   tu_lbx_get(tu_listbox_t* lbxp, int idx, tu_subitem_t* itemp, int nitems)
{
    int i = 0;
    tu_listsubitem_t* subitemp = 0;
    
    if ((nitems <= 0) ||
        (nitems > 0 && nitems > lbxp->nheaders))
    {
        nitems = lbxp->nheaders;
    }
    subitemp = tu_lbx__get(lbxp, idx);
    if (subitemp)
    {
        /*copy data*/
        for (i = 0; i < nitems; ++i)
        {
            itemp[i].fgcolor    = subitemp[i].fgcolor;
            itemp[i].bgcolor    = subitemp[i].bgcolor;
            itemp[i].fgsel      = subitemp[i].fgsel;
            itemp[i].bgsel      = subitemp[i].bgsel;
            itemp[i].data       = subitemp[i].data;
            strcpy(itemp[i].text, subitemp[i].text);
        }
    }
    return 0;
}

int tu_lbx_set(tu_listbox_t* lbxp, int idx, tu_subitem_t* itemp, int nitems)
{
    int i = 0;
    tu_listsubitem_t* subitemp = 0;
    
    if ((nitems <= 0) ||
        (nitems > 0 && nitems > lbxp->nheaders))
    {
        nitems = lbxp->nheaders;
    }
    subitemp = tu_lbx__get(lbxp, idx);
    if (subitemp)
    {
        /*copy data*/
        for (i = 0; i < nitems; ++i)
        {
            subitemp[i].fgcolor      = itemp[i].fgcolor;
            subitemp[i].bgcolor      = itemp[i].bgcolor;
            subitemp[i].fgsel        = itemp[i].fgsel;
            subitemp[i].bgsel        = itemp[i].bgsel;
            subitemp[i].data         = itemp[i].data;
            strcpy(subitemp[i].text, itemp[i].text);
        }
    }
    return 0;
}

int tu_lbx_setitem(tu_listbox_t* lbxp, int idx, int col, tu_subitem_t* itemp)
{
    tu_listsubitem_t* subitemp = 0;
    if (col < 0 || col >= lbxp->nheaders)
    {
        return -1;
    }
    subitemp = tu_lbx__get(lbxp, idx);
    if (subitemp)
    {
        subitemp[col].fgcolor   = itemp->fgcolor;
        subitemp[col].bgcolor   = itemp->bgcolor;
        subitemp[col].fgsel     = itemp->fgsel;
        subitemp[col].bgsel     = itemp->bgsel;
        subitemp[col].data      = itemp->data;
        strcpy(subitemp->text, itemp->text);
    }
    return 0;
}

int tu_lbx_getitem(tu_listbox_t* lbxp, int idx, int col, tu_subitem_t* itemp)
{
    tu_listsubitem_t* subitemp = 0;
    if (col < 0 || col >= lbxp->nheaders)
    {
        return -1;
    }
    subitemp = tu_lbx__get(lbxp, idx);
    if (subitemp)
    {
        itemp->fgcolor = subitemp[col].fgcolor;
        itemp->bgcolor = subitemp[col].bgcolor;
        itemp->fgsel   = subitemp[col].fgsel;
        itemp->bgsel   = subitemp[col].bgsel;
        itemp->data    = subitemp[col].data;
        strcpy(itemp->text, subitemp->text);
    }
    return 0;
}

void tu_lbx_clear(tu_listbox_t* lbxp)
{
    tu_listsubitem_t* subitemp = 0;
    tu_listnode_t* node = 0;
    if (!lbxp->rowp)
    {
        return;
    }
    node = tu_list_first(lbxp->rowp);
    while (node)
    {
        subitemp = (tu_listsubitem_t*)tu_list_data(node);
        tu_list_remove(lbxp->rowp, node, 0);
        free(subitemp);
        node = tu_list_first(lbxp->rowp);
    }
    lbxp->nrows     = 0;
    lbxp->currow    = -1;
    lbxp->curcol    = -1;
}

void tu_lbx_reset(tu_listbox_t* lbxp)
{
    tu_lbx_clear(lbxp);
    if (lbxp->rowp)
    {
        tu_list_delete(lbxp->rowp);
    }
    if (lbxp->hdrp)
    {
        free(lbxp->hdrp);
    }
    memset(lbxp, 0, sizeof(struct tu_listbox));
    lbxp->currow    = -1;
    lbxp->curcol    = -1;
}
