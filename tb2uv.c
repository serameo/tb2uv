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
    struct tu__event* evp = (struct tu__event*)calloc(1, sizeof(struct tb_event));
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
    char    text[FIELD_MAX_TEXT + 1];
    tu_listnode_t*    node;
};

struct tu_input
{
    struct tu_field     field;
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

struct tu_window
{
    jsw_rbtree_t*   children;       /*hold all tu_fields*/
    jsw_rbtrav_t*   childtrav;
    tu_linklist_t*  fields;         /*to handle movable children*/
    tu_listnode_t*  nodes;
    /*tu_listtrav_t*  itertrav;
    tu_listtrav_t*  active_iter;*/
    tu_field_t*     active_input;
    int             default_action; /*default id*/
    int             (*on_event)(int mod, int key, int ch, void* data);
};
static int   tu_wnd__cmp ( const void *p1, const void *p2 )
{
    tu_field_t* f1 = (tu_field_t*)p1;
    tu_field_t* f2 = (tu_field_t*)p2;
    return (f1->id - f2->id);
}
static void *tu_wnd__dup ( void *p1 )
{
    tu_field_t* f1   = (tu_field_t*)p1;
    tu_field_t* fldp = (tu_field_t*)calloc(1, f1->size);
    if (fldp)
    {
        memcpy(fldp, f1, f1->size);
    }
    return fldp;
}
static void  tu_wnd__rel ( void *p )
{
    free(p);
}
static tu_field_t* tu_wnd__getinput(tu_window_t* wndp)
{
    return (wndp ? wndp->active_input : NULL);
}

int tu_input__draw(tu_field_t* fldp);

static tu_field_t* tu_wnd__getnextinput(tu_window_t* wndp, int dir)
{
    tu_field_t* fldp = (wndp ? wndp->active_input : NULL);
    /*tu_listtrav_t* iter = tu_listtrav_new();
    tu_listtrav_clone(iter, wndp->active_iter);*/
    tu_listnode_t*  node = 0;
    
    while (fldp)
    {
        node = fldp->node;
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
        fldp = (tu_field_t*)tu_list_data(node);
        if (fldp && fldp->type == FIELD_INPUT && fldp->enable && fldp->visible)
        {
            break;
        }
    }
    /*moved successfully*/
    if (fldp && fldp != wndp->active_input)
    {
        tu_input_t* inp = (tu_input_t*)wndp->active_input;
        inp->first_focus = 0;
        tu_input__draw(wndp->active_input);
        
        /*tu_listtrav_clone(wndp->active_iter, iter);*/
        
        inp = (tu_input_t*)fldp;
        inp->first_focus = 1;
        tu_input__draw(fldp);
        
        wndp->active_input = fldp;
    }
    
    /*tu_listtrav_delete(iter);*/
    return fldp;
}

static void field_draw(int x, int y, int width, const char* text, int fg, int bg, int aligment, int attribs, int redraw);
static void field_format_text(char* dest, int limit, const char* src, int alignment);
int         field_process_event(tu_field_t* fldp, struct tb_event* evp);

static void on_termbox_event(uv_poll_t* handle, int status, int events)
{
    struct tb_event ev;
    int rc = tb_peek_event(&ev, 0);
    struct tu__env* envp = (struct tu__env*)handle->data;
    tu_window_t* wndp = tu_getwindow(envp);

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
                if (wndp->default_action != 0)
                {
                }
                else
                {
                }
            }
            else if (ev.key == TB_KEY_BACK_TAB)
            {
                tu_field_t* nextfldp = tu_wnd__getnextinput(wndp, -1);
            }
            else if (ev.key == TB_KEY_TAB)
            {
                tu_field_t* nextfldp = tu_wnd__getnextinput(wndp, 1);
            }
            else
            {
                tu_field_t* curfldp = tu_wnd__getinput(wndp);
                field_process_event(curfldp, &ev);
                tu_fld_draw(curfldp);
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

tu_window_t*    tu_setwindow(tu_window_t* wnd)
{
    struct tu__env* envp = tu__getinstance();
    tu_window_t* oldwnd = NULL;
    oldwnd = envp->active_wnd;
    envp->active_wnd = wnd;
    return oldwnd;
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
do {                                                        \
    memset((field_ptr), 0, (field_size));                   \
    field_ptr->size         = (field_size);                 \
    field_ptr->id           = (id);                         \
    field_ptr->type         = (typ);                        \
    field_ptr->x            = (x);                          \
    field_ptr->y            = (y);                          \
    field_ptr->w            = (w);                          \
    field_ptr->h            = (1);                          \
    field_ptr->fgcolor      = (0);                          \
    field_ptr->bgcolor      = (0);                          \
    field_ptr->enable       = (1);                          \
    field_ptr->visible      = (1);                          \
    field_ptr->alignment    = (0);                          \
    field_ptr->attribs      = (0);                          \
    strncpy(field_ptr->text, txt, FIELD_MAX_TEXT);          \
} while (0)

tu_field_t* tu_fld_new(int type)
{
    tu_field_t* fldp = 0;
    switch (type)
    {
        case FIELD_LABEL:
            fldp = (tu_field_t*)calloc(1, sizeof(struct tu_field));
            break;
        case FIELD_INPUT:
            fldp = (tu_field_t*)calloc(1, sizeof(struct tu_input));
            break;
        default:
            break;
    }
    return fldp;
}

void tu_fld_delete(tu_field_t* fldp)
{
    free(fldp);
}

int tu_fld_initlabel(tu_field_t* fldp, int id, int x, int y, int w, const char* text)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_field), id, FIELD_LABEL, x, y, w, text);
    fldp->enable = 0; /*always disable*/
    return 0;
}

int tu_fld_initinput(tu_field_t* fldp, int id, int x, int y, int w, const char* text)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_input), id, FIELD_INPUT, x, y, w, text);
    return 0;
}

int tu_fld_setcolor(tu_field_t* fldp, int fg, int bg)
{
    fldp->fgcolor = fg;
    fldp->bgcolor = bg;
    return 0;
}

int tu_fld_setattribs(tu_field_t* fldp, int attribs)
{
    fldp->attribs = attribs;
    return 0;
}

int tu_fld_settext(tu_field_t* fldp, const char* text)
{
    strncpy(fldp->text, text, FIELD_MAX_TEXT);
    return 0;
}

int tu_fld_gettext(tu_field_t* fldp, char* text)
{
    strncpy(text, fldp->text, FIELD_MAX_TEXT);
    return 0;
}

int tu_fld_isenable(tu_field_t* fldp)
{
    return fldp->enable;
}

void tu_fld_enable(tu_field_t* fldp, int enable)
{
    fldp->enable = enable;
}

int tu_fld_isvisible(tu_field_t* fldp)
{
    return fldp->visible;
}

void tu_fld_visible(tu_field_t* fldp, int visible)
{
    fldp->visible = visible;
}

int tu_input__draw(tu_field_t* fldp)
{
    tu_input_t* input = (tu_input_t*)fldp;
    int  x = fldp->x;
    int  y = fldp->y;
    int  width = fldp->w;
    int  fg = (fldp->enable ? fldp->fgcolor : input->fgdis);
    int  bg = (fldp->enable ? fldp->bgcolor : input->bgdis);
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
        memcpy(text, &fldp->text[input->xpos], width);
    }
    field_draw(fldp->x, fldp->y, fldp->w, 
        text,
        fg, bg, 
        fldp->alignment, fldp->attribs, 0);
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
    if (fldp->type == FIELD_INPUT)
    {
        tu_input__draw(fldp);
        return 0;
    }
    tu_drawtext(fldp->x, fldp->y, fldp->w, 
        fldp->text, 
        fldp->fgcolor, fldp->bgcolor, 
        fldp->alignment, fldp->attribs, 1);
    return 0;
}

int tu_input__process_event(tu_field_t* fldp, struct tb_event* ev)
{
    int len   = 0;
    tu_input_t* inp = (tu_input_t*)fldp;
    int width = fldp->w;

    if (!fldp->enable || !fldp->visible)
    {
        return 0;
    }
    if (inp->first_focus)
    {
        inp->xpos = 0;
        inp->xcur = 0;
        memset(fldp->text, 0, FIELD_MAX_TEXT);
        fldp->text[FIELD_MAX_TEXT] = 0;

        if (ev->ch)
        {
            fldp->text[inp->xcur] = ev->ch;
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
                len = strlen(fldp->text);
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
                    fldp->text[inp->xcur] = 0;
                }
                break;
            }
        }
    }
    else if (ev->ch)
    {
        if (inp->xcur < FIELD_MAX_TEXT)
        {
            fldp->text[inp->xcur] = ev->ch;
            ++inp->xcur;
            len = strlen(fldp->text);
            if (len >= width)
            {
                inp->xpos = len - width;
            }
        }
    }
    return 1;
}

int field_process_event(tu_field_t* fldp, struct tb_event* evp)
{
    switch (fldp->type)
    {
        case FIELD_INPUT:
            return tu_input__process_event(fldp, evp);
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
    /*if (wndp->itertrav)
    {
        free(wndp->itertrav);
        wndp->itertrav = NULL;
    }*/
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

void tu_wnd_removefield(tu_window_t* wndp, int id)
{
    tu_field_t* fldp = tu_wnd_getfield(wndp, id);
    
    if (fldp)
    {
        if (fldp == wndp->active_input)
        {
            tu_field_t* nextp = tu_wnd__getnextinput(wndp, -1);
            if (fldp == nextp)
            {
                /*no prev*/
                nextp = tu_wnd__getnextinput(wndp, 1);
            }
            if (fldp == nextp)
            {
                /*no next: only one input active*/
                wndp->active_input = NULL;
            }
        }
        tu_list_erase(wndp->fields, fldp, 0);
        jsw_rberase(wndp->children, fldp);
    }
}

tu_field_t* tu_wnd_getfield(tu_window_t* wndp, int id)
{
    tu_field_t fld;
    fld.id = id;
    tu_field_t* fldp = jsw_rbfind(wndp->children, &fld);
    return fldp;
}

int tu_wnd_addfield(tu_window_t* wndp, tu_field_t* field)
{
    int rc = jsw_rbinsert(wndp->children, field);
    if (1 == rc) /*insert successfully*/
    {
        tu_field_t* newfield = jsw_rbfind(wndp->children, field);
        tu_listnode_t*  node = 0;
        tu_list_pushback(wndp->fields, newfield, 0);
        node = tu_list_last(wndp->fields);
        newfield->node = node;
        /*active_input*/
        if ( NULL == wndp->active_input )
        {
            if (newfield->enable && newfield->visible)
            {
                tu_input_t* inp = (tu_input_t*)newfield;
                inp->first_focus = 1;
                wndp->active_input = newfield;
            }
        }
    }
    return rc;
}

tu_field_t* tu_wnd_getactive(tu_window_t* wndp)
{
    return (wndp->active_input);
}

tu_field_t* tu_wnd_setactive(tu_window_t* wndp, int id)
{
    tu_field_t* activep = tu_wnd_getactive(wndp);
    tu_field_t* newfldp = tu_wnd_getfield(wndp, id);
    if (NULL == newfldp)
    {
        if (newfldp->enable && newfldp->visible)
        {
            tu_input_t* inp = (tu_input_t*)wndp->active_input;
            inp->first_focus = 0;
            tu_fld_draw(wndp->active_input);
            
            wndp->active_input = newfldp;
            inp = (tu_input_t*)wndp->active_input;
            inp->first_focus = 1;
            tu_fld_draw(newfldp);
        }
    }
    return activep;
}

tu_field_t*     tu_wnd_getfirst(tu_window_t* wndp)
{
    wndp->nodes = tu_list_first(wndp->fields);
    tu_field_t* fldp = (tu_field_t*)tu_list_data(wndp->nodes);
    return fldp;
}

tu_field_t*     tu_wnd_getlast(tu_window_t* wndp)
{
    wndp->nodes = tu_list_last(wndp->fields);
    tu_field_t* fldp = (tu_field_t*)tu_list_data(wndp->nodes);
    return fldp;
}

tu_field_t*     tu_wnd_getnext(tu_window_t* wndp)
{
    wndp->nodes = tu_list_next(wndp->nodes);
    tu_field_t* fldp = (tu_field_t*)tu_list_data(wndp->nodes);
    return fldp;
}

tu_field_t*     tu_wnd_getprev(tu_window_t* wndp)
{
    wndp->nodes = tu_list_prev(wndp->nodes);
    tu_field_t* fldp = (tu_field_t*)tu_list_data(wndp->nodes);
    return fldp;
}

void            tu_wnd_refresh(tu_window_t* wndp)
{
    tu_field_t* fldp = tu_wnd_getfirst(wndp);
    while (fldp)
    {
        if (tu_fld_isvisible(fldp))
        {
            tu_fld_draw(fldp);
        }
        fldp = tu_wnd_getnext(wndp);
    }
}

void tu_wnd_setevent(tu_window_t* wndp, int (*on_event)(int mod, int key, int ch, void* data))
{
    wndp->on_event = on_event;
}

