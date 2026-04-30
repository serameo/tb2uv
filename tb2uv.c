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
struct tu__env 
{
    uv_poll_t       poll_handler;
    uv_loop_t*      main_loop;
    tu_window_t*    active_wnd;        /*hold windows*/
};
struct tu__env* g_envp = NULL;    /*global termbox2 libuv environment*/
char g_blank[FIELD_MAX_TEXT + 1] = "";

static struct tu__env*   tu__newinstance()
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
    tu_linklist_t*  movable_iter;   /*to handle movable children*/
    tu_listtrav_t*  itertrav;
    tu_listtrav_t*  active_iter;
    tu_field_t*     active_input;
    int             default_action; /*default id*/
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
    tu_listtrav_t* iter = tu_listtrav_new();
    tu_listtrav_clone(iter, wndp->active_iter);
    
    while (fldp)
    {
        if (dir < 0)
        {
            fldp = (tu_field_t*)tu_listrav_prev(iter);
        }
        else /*if (dir >= 0)*/
        {
            fldp = (tu_field_t*)tu_listrav_next(iter);
        }
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
        
        tu_listtrav_clone(wndp->active_iter, iter);
        
        inp = (tu_input_t*)fldp;
        inp->first_focus = 1;
        tu_input__draw(fldp);
        
        wndp->active_input = fldp;
    }
    
    tu_listtrav_delete(iter);
    return fldp;
}

static void field_draw(int x, int y, int width, const char* text, int fg, int bg, int aligment, int attribs, int redraw);
static void field_format_text(char* dest, int limit, const char* src, int alignment);
int         field_process_event(tu_field_t* fldp, struct tb_event* evp);

static void on_termbox_event(uv_poll_t* handle, int status, int events)
{
    static int y = 1;
    struct tb_event ev;
    int rc = tb_peek_event(&ev, 0);
    struct tu__env* envp = (struct tu__env*)handle->data;
    tu_window_t* wndp = tu_getwindow(envp);

    if (rc == TB_OK)
    {
        if (ev.type == TB_EVENT_KEY)
        {
            if (ev.key == TB_KEY_F3 && ev.mod == TB_MOD_ALT)
            {
                tb_shutdown();
                uv_stop(uv_default_loop());
            }
            else if (ev.key == TB_KEY_ENTER)
            {
                /*process a default action in the active window*/
                if (wndp && wndp->default_action != 0)
                {
                }
                else
                {
                }
            }
            else if (ev.key == TB_KEY_BACK_TAB)
            {
                if (wndp)
                {
                    tu_field_t* nextfldp = tu_wnd__getnextinput(wndp, -1);
                }
            }
            else if (ev.key == TB_KEY_TAB)
            {
                if (wndp)
                {
                    tu_field_t* nextfldp = tu_wnd__getnextinput(wndp, 1);
                }
            }
            else
            {
                if (wndp)
                {
                    tu_field_t* curfldp = tu_wnd__getinput(wndp);
                    field_process_event(curfldp, &ev);
                    tu_fld_draw(curfldp);
                }
            }
        }
    }
}

int     tu_init()
{
    /*termbox2*/
    tb_init();
    /*tb_set_input_mode(TB_INPUT_ALT);*/

    /*libuv*/
    tu__newinstance();  /*first init environment*/
    g_envp->main_loop = uv_default_loop();
    
    uv_poll_init(g_envp->main_loop, &g_envp->poll_handler, STDIN_FILENO);
    g_envp->poll_handler.data = g_envp;
    uv_poll_start(&g_envp->poll_handler, UV_READABLE, on_termbox_event);
    
    /*windows*/
    g_envp->active_wnd = NULL;
    
    /*others*/
    memset(g_blank, ' ', sizeof(g_blank));
    g_blank[FIELD_MAX_TEXT] = 0;
    return 0;
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
    uv_run(g_envp->main_loop, UV_RUN_DEFAULT);
    return 0;
}

tu_window_t*    tu_setwindow(tu_window_t* wnd)
{
    tu_window_t* oldwnd = NULL;
    if (g_envp)
    {
        oldwnd = g_envp->active_wnd;
        g_envp->active_wnd = wnd;
    }
    return oldwnd;
}

tu_window_t*    tu_getwindow()
{
    return (g_envp ? g_envp->active_wnd : NULL);
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

void tu_draw_text(int x, int y, int width, const char* text, int fg, int bg, int alignment, int attribs, int redraw)
{
    field_draw(x, y, width, text, fg, bg, alignment, attribs, redraw);
}

void tu_draw_char(int x, int y, char ch, int fg, int bg, int alignment, int attribs, int redraw)
{
    char sz[2] = { ch, 0 };
    tu_draw_text(x, y, 1, sz, fg, bg, FIELD_LEFT, attribs, redraw);
}

void tu_format(char* dest, int limit, const char* src, int alignment)
{
    field_format_text(dest, limit, src, alignment);
}

void tu_draw_line(int x, int y, int width, char ch, int fg, int bg, int attribs, int redraw)
{
    char line[FIELD_MAX_TEXT + 1];
    memset(line, ch, sizeof(line));
    line[FIELD_MAX_TEXT] = 0;
    tu_draw_text(x, y, width, line, fg, bg, FIELD_LEFT, attribs, redraw);
}

void tu_draw_vline(int x, int y, int height, char ch, int fg, int bg, int attribs, int redraw)
{
    char sz[2] = { ch, 0 };
    int i = 0;
    for (i = 0; i < height; ++i)
    {
        tu_draw_text(x, y + i, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    }
    if (redraw)
    {
        tb_present();
    }
}

void tu_draw_box(int x, int y, int width, int height, char chhorz, char chvert, char chcorner, int fg, int bg, int attribs, int redraw)
{
    char sz[2] = { chcorner, 0 };
    /*upper-left corner*/
    tu_draw_text(x, y, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    /*upper-right corner*/
    tu_draw_text(x + width, y, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    /*lower-left corner*/
    tu_draw_text(x, y + height, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    /*lower-right corner*/
    tu_draw_text(x + width, y + height, 1, sz, fg, bg, FIELD_LEFT, attribs, 0);
    
    /*draw upper line*/
    tu_draw_line(x + 1, y, width - 1, chhorz, fg, bg, attribs, 0);
    /*draw lower line*/
    tu_draw_line(x + 1, y + height, width - 1, chhorz, fg, bg, attribs, 0);
    
    /*draw left line*/
    tu_draw_vline(x, y + 1, height - 1, chvert, fg, bg, attribs, 0);
    /*draw right line*/
    tu_draw_vline(x + width, y + 1, height - 1, chvert, fg, bg, attribs, 0);
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
    tu_draw_text(fldp->x, fldp->y, fldp->w, 
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
        if (ev->ch)
        {
            inp->xpos = 0;
            inp->xcur = 0;
            memset(fldp->text, 0, FIELD_MAX_TEXT);
            fldp->text[FIELD_MAX_TEXT] = 0;

            fldp->text[inp->xcur] = ev->ch;
            ++inp->xcur;
        }
        else if (ev->key == TB_KEY_BACKSPACE ||
                 ev->key == TB_KEY_BACKSPACE2)
        {
            inp->xpos = 0;
            inp->xcur = 0;
            memset(fldp->text, 0, FIELD_MAX_TEXT);
            fldp->text[FIELD_MAX_TEXT] = 0;
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
        wndp->movable_iter  = tu_list_new(tu_wnd__cmp, tu_wnd__dup, tu_wnd__rel);
        wndp->itertrav      = tu_listtrav_new();
        wndp->active_iter   = tu_listtrav_new();
    }
    return wndp;
}

void tu_wnd_delete(tu_window_t* wndp)
{
    if (wndp->itertrav)
    {
        free(wndp->itertrav);
        wndp->itertrav = NULL;
    }
    if (wndp->movable_iter)
    {
        tu_list_delete(wndp->movable_iter);
        wndp->movable_iter = NULL;
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

int tu_wnd_addfield(tu_window_t* wndp, tu_field_t* field)
{
    int rc = jsw_rbinsert(wndp->children, field);
    if (1 == rc) /*insert successfully*/
    {
        tu_field_t* newfield = jsw_rbfind(wndp->children, field);
        tu_list_pushback(wndp->movable_iter, newfield, 0);
        /*active_input*/
        if ( NULL == wndp->active_input )
        {
            if (newfield->enable && newfield->visible)
            {
                tu_field_t* fldp = tu_listrav_first(wndp->active_iter, wndp->movable_iter);
                tu_input_t* inp = (tu_input_t*)newfield;
                inp->first_focus = 1;
                wndp->active_input = newfield;
                
                /*init the active_iter*/
                while (fldp && fldp != newfield)
                {
                    fldp = (tu_field_t*)tu_listrav_next(wndp->active_iter);
                }
            }
        }
    }
    return rc;
}

tu_field_t*     tu_wnd_getfirst(tu_window_t* wndp)
{
    tu_field_t* fldp = (tu_field_t*)tu_listrav_first(wndp->itertrav, wndp->movable_iter);
    return fldp;
}

tu_field_t*     tu_wnd_getlast(tu_window_t* wndp)
{
    tu_field_t* fldp = (tu_field_t*)tu_listrav_last(wndp->itertrav, wndp->movable_iter);
    return fldp;
}

tu_field_t*     tu_wnd_getnext(tu_window_t* wndp)
{
    tu_field_t* fldp = (tu_field_t*)tu_listrav_next(wndp->itertrav);
    return fldp;
}

tu_field_t*     tu_wnd_getprev(tu_window_t* wndp)
{
    tu_field_t* fldp = (tu_field_t*)tu_listrav_prev(wndp->itertrav);
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

