#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
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
    unsigned int flags;
    tu_listnode_t*      node;
    tu_window_t*        parent;
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
    int     xcur;
    int     ycur;
    /*position*/
    int     xpos;
    int     ypos;
    /*limit*/
    int     limit;
    char    password[FIELD_MAX_TEXT + 1];
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
    int     attribs;
    void*   data;
    char    text[FIELD_MAX_TEXT + 1];
    /*internally used*/
    tu_listnode_t*    node;
    int     sorting;
};
typedef struct tu_listsubitem tu_listsubitem_t;

/*sorting*/
struct tu_row
{
    tu_listsubitem_t   subitems[10];
};
typedef struct tu_row       tu_row_t;       /*listbox row: to compare*/

struct tu_listbox
{
    struct tu_wnditem     item;      /*based object*/
    /*header*/
    tu_listheader_t*    hdrp;
    int                 nheaders;
    int                 rightcol;   /*to show the max right column*/
    /*rows*/
    tu_linklist_t*      rowp;       /*to point the number of rows*/
    int                 nrows;
    /*index*/
    int                 currow;     /*TB_KEY_UP | TB_KEY_DOWN | TB_KEY_PAGEUP | TB_KEY_PAGEDOWN*/
    int                 curcol;     /*changed when pressed TB_KEY_LEFT or TB_KEY_RIGHT*/
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
    tu_wnditem_t*       activep;
    /*events*/
    int                 (*on_keydown)(int mod, int key, int ch, tu_notify_t* notify);    /*data = self   (tu_window_t*)  */
    int                 (*on_blur)   (int mod, int key, int ch, tu_notify_t* notify);    /*data = child  (tu_wnditem_t*) */
    int                 (*on_focus)  (int mod, int key, int ch, tu_notify_t* notify);    /*data = child  (tu_wnditem_t*) */
    int                 (*on_notify) (int mod, int key, int ch, tu_notify_t* notify);    /*data = child  (tu_wnditem_t*) */
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
    return (wndp ? wndp->activep : NULL);
}

int  tu_inp__settext(tu_input_t* inp, const char* text);
void tu_inp__draw(tu_input_t* inp);
void tu_lbx__draw(tu_listbox_t* lbxp);

static tu_wnditem_t* tu_wnd__getnextinput(tu_window_t* wndp, int dir)
{
    tu_wnditem_t* itemp = (wndp ? wndp->activep : NULL);
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
        if (itemp && /*itemp->type == FIELD_INPUT && */
            itemp->enable && itemp->visible)
        {
            break;
        }
    }
    /*moved successfully*/
    if (node && itemp && itemp != wndp->activep)
    {
        tu_input_t* inp = 0;

        if (wndp->activep->type == FIELD_INPUT)
        {
            inp = (tu_input_t*)wndp->activep;
            inp->first_focus = 0;
            tu_inp__draw(inp);
        }
        
        if (itemp->type == FIELD_INPUT)
        {
            inp = (tu_input_t*)itemp;
            inp->first_focus = 1;
            tu_inp__draw(inp);
        }
        
        wndp->activep = itemp;
    }
    
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
        tu_inp__draw((tu_input_t*)itemp);
        return;
    }
    else if (itemp->type == FIELD_LISTBOX)
    {
        tu_lbx__draw((tu_listbox_t*)itemp);
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
    tu_wnditem_t* nextp = 0;
    tu_wnditem_t* curp = 0;
    tu_notify_t   notify = { 0, 0, 0 };

    if (rc == TB_OK && wndp)
    {
        if (ev.type == TB_EVENT_KEY)
        {
            /*check the active window processing the key if it is registered the event*/
            rc = 0;
            if (wndp->on_keydown)
            {
                notify.data = wndp;
                rc = wndp->on_keydown(ev.mod, ev.key, ev.ch, &notify);
            }
            if (rc != 0)
            {
                return; /*alreay processed by wndp*/
            }
            /*the global event*/
            curp = tu_wnd__getinput(wndp);
            if (ev.key == TB_KEY_CTRL_C)
            {
                tu_shutdown();
            }
            else if (ev.key == TB_KEY_BACK_TAB)
            {
                nextp = tu_wnd__getnextinput(wndp, -1);
            }
            else if (ev.key == TB_KEY_TAB)
            {
                nextp = tu_wnd__getnextinput(wndp, 1);
            }
            else
            {
                if (curp)
                {
                    field_process_event(curp, &ev);
                    tu_wnditem_draw(curp);
                }
            }
            /*check if the window implement the events*/
            do
            {
                if (nextp && curp && nextp != curp)
                {
                    if (wndp->on_blur)
                    {
                        notify.id = curp->id;
                        notify.data = curp;
                        rc = wndp->on_blur(ev.mod, ev.key, ev.ch, &notify);
                        if (rc != 0)
                        {
                            /*not allowed to move*/
                            tu_wnd_setactive(wndp, curp->id);
                            break;
                        }
                    }
                    if (wndp->on_focus)
                    {
                        notify.id = nextp->id;
                        notify.data = nextp;
                        rc = wndp->on_focus(ev.mod, ev.key, ev.ch, &notify);
                    }
                }
            } while (0);
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

    envp->events = jsw_rbnew(tu_env__cmp, tu_env__dup, tu_env__rel);

    /*libuv*/
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


int tu_fld_initlabel(tu_field_t* fldp, int id, int x, int y, int w, const char* text)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_wnditem), id, FIELD_LABEL, x, y, w, text);
    fldp->enable = 0; /*always disable*/
    return 0;
}

int tu_fld_initinput(tu_field_t* fldp, int id, int x, int y, int w)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_input), id, FIELD_INPUT, x, y, w, "");
    return 0;
}

int tu_fld_initlistbox(tu_field_t* fldp, int id, int x, int y, int w, int height, const char* text)
{
    INIT_FIELD_MEMBER(fldp, sizeof(struct tu_listbox), id, FIELD_LISTBOX, x, y, w, text);
    fldp->h = height; 
    return 0;
}

void tu_wnditem_setflags(tu_wnditem_t* itemp, unsigned int flags)
{
    itemp->flags = flags;
}

unsigned int tu_wnditem_getflags(tu_wnditem_t* itemp)
{
    return itemp->flags;
}

tu_window_t*    tu_wnditem_getparent(tu_wnditem_t* itemp)
{
    return (itemp ? itemp->parent : NULL);
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
    if (itemp->type == FIELD_INPUT)
    {
        tu_inp__settext((tu_input_t*)itemp, text);
    }
    strncpy(itemp->text, text, FIELD_MAX_TEXT);
    return 0;
}

int tu_wnditem_gettext(tu_wnditem_t* itemp, char* text, int len)
{
    if (len > FIELD_MAX_TEXT)
    {
        len = strlen(itemp->text);
    }
    strncpy(text, itemp->text, len);
    return 0;
}

int tu_wnditem_isenable(tu_wnditem_t* itemp)
{
    return itemp->enable;
}

void tu_wnditem_enable(tu_wnditem_t* itemp, int enable)
{
    if (enable != 0 && itemp->type == FIELD_LABEL)
    {
        /*label is always disable*/
        return;
    }
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

int  tu_inp__settext(tu_input_t* inp, const char* text)
{
    tu_wnditem_t* itemp = &inp->item;
    unsigned int flags = itemp->flags;
    int len = strlen(text);
    char buffer[FIELD_MAX_TEXT + 1];

    memset(buffer, 0, sizeof(buffer));
    if (flags & FIELD_INPUT_PASSWORD)
    {
        memset(buffer, '*', len);
        strncpy(inp->password, text, len);
    }
    else if (flags & (FIELD_INPUT_NUMBER | FIELD_INPUT_HEXNUMBER))
    {
        char* endptr;
        unsigned long long int number;
        int base = 10;
        if (flags & FIELD_INPUT_HEXNUMBER)
        {
            base = 16;
        }
        number = strtoull (text, &endptr, base);
        sprintf(buffer, "%llu", number);
    }
    if (flags & FIELD_INPUT_CAPITAL)
    {
        int i = 0;
        len = strlen(buffer);
        for (i = 0; i < len; ++i)
        {
            buffer[i] = toupper(buffer[i]);
        }
    }
    strcpy(itemp->text, buffer);
    return 0;
}

void tu_inp__draw(tu_input_t* inp)
{
    tu_wnditem_t* itemp = &inp->item;
    int  x = itemp->x;
    int  y = itemp->y;
    int  width = itemp->w;
    int  fg = (itemp->enable ? itemp->fgcolor : inp->fgdis);
    int  bg = (itemp->enable ? itemp->bgcolor : inp->bgdis);
    char text[FIELD_MAX_TEXT + 1] = "";
    int  len = 0;
    unsigned int flags = itemp->flags;
    int  limit = inp->limit;
    int  number = atoi(itemp->text);
    char buffer[FIELD_MAX_TEXT + 1] = "";

    if (inp->first_focus)
    {
        fg |= FIELD_REVERSE;
        bg |= FIELD_REVERSE;
    }
    if (limit > 0 && limit <= FIELD_MAX_TEXT)
    {
        width = (width < limit ? width : limit);
    }

    memset(text, 0, FIELD_MAX_TEXT);
    text[FIELD_MAX_TEXT] = 0;
    if (flags & FIELD_INPUT_NOECHO)
    {
        text[0] = 0; /*no drawing*/
    }
    else
    {
        if (flags & FIELD_INPUT_PASSWORD)
        {
            memset(text, '*', width);
        }
        /*else if (flags & FIELD_INPUT_NUMBER)
        {
            sprintf(buffer, "%d", number);
            memcpy(text, &buffer[inp->xpos], width);
        }
        else if (flags & FIELD_INPUT_HEXNUMBER)
        {
            char* endp;
            number = strtol(itemp->text, &endp, 16);
            if (number != 0)
            {
                if (flags & FIELD_INPUT_CAPITAL)
                {
                    sprintf(buffer, "%X", number);
                }
                else
                {
                    sprintf(buffer, "%x", number);
                }
                memcpy(text, &buffer[inp->xpos], width);
            }
        }*/
        else
        {
            memcpy(text, &itemp->text[inp->xpos], width);
        }
    }
    field_draw(itemp->x, itemp->y, width, 
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
    unsigned int flags = itemp->flags;

    if (!itemp->enable || !itemp->visible)
    {
        return 0;
    }
    if (inp->first_focus)
    {
        inp->first_focus = 0;
        inp->xpos = 0;
        inp->xcur = 0;
        memset(itemp->text, 0, FIELD_MAX_TEXT);
        itemp->text[FIELD_MAX_TEXT] = 0;
#if 0
        if (ev->ch)
        {
            char ch = ev->ch;
            inp->xpos = 0;
            inp->xcur = 0;
            memset(itemp->text, 0, FIELD_MAX_TEXT);
            itemp->text[FIELD_MAX_TEXT] = 0;
            if (flags & FIELD_INPUT_CAPITAL)
            {
                if (ch >= 'a' && ch <= 'z')
                {
                    ch = (ch - 'a') + 'A';
                }
            }
            if (flags & FIELD_INPUT_NUMBER)
            {
                if (ch < '0' || ch > '9')
                {
                    /*replace*/
                    return 1;
                }
            }
            if (flags & FIELD_INPUT_HEXNUMBER)
            {
                if ((ch >= '0' && ch <= '9') ||
                    (ch >= 'a' && ch <= 'f') ||
                    (ch >= 'A' && ch <= 'F'))
                {
                }
                else
                {
                    /*replace*/
                    return 1;
                }
            }
            if (flags & FIELD_INPUT_PASSWORD)
            {
                memset(inp->password, 0, FIELD_MAX_TEXT);
                inp->password[FIELD_MAX_TEXT] = 0;
                inp->password[inp->xcur] = ev->ch;
                
                ch = '*';
            }
            itemp->text[inp->xcur] = ch;
            ++inp->xcur;
        }
        else
        {
            if (flags & (FIELD_INPUT_NUMBER | FIELD_INPUT_HEXNUMBER))
            {
                int number = atoi(itemp->text);
                if (number == 0)
                {
                    /*always replace*/
                    inp->xpos = 0;
                    inp->xcur = 0;
                    memset(itemp->text, 0, FIELD_MAX_TEXT);
                    itemp->text[FIELD_MAX_TEXT] = 0;
                }
                else
                {
                    len = strlen(itemp->text);
                    if (len > width)
                    {
                        inp->xpos = len - width;
                    }
                    inp->xcur = len;
                }
            }
            else
            {
                len = strlen(itemp->text);
                if (len > width)
                {
                    inp->xpos = len - width;
                }
                inp->xcur = len;
            }
        }
        if (ev->key)
        {
            tu_window_t* wndp = tu_wnditem_getparent(itemp);
            if (wndp->on_notify && ev->key == TB_KEY_ENTER)
            {
                tu_notify_t notify = { itemp->id, itemp, FIELD_NOTIFY_PRESSEDENTER };
                wndp->on_notify(ev->mod, ev->key, ev->ch, &notify);
            }
        }
        return 0;
#endif
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
            case TB_KEY_ENTER:
            {
                tu_window_t* wndp = tu_wnditem_getparent(itemp);
                if (wndp->on_notify && ev->key == TB_KEY_ENTER)
                {
                    tu_notify_t notify = { itemp->id, itemp, FIELD_NOTIFY_PRESSEDENTER };
                    wndp->on_notify(ev->mod, ev->key, ev->ch, &notify);
                }
                break;
            }
        }
    }
    else if (ev->ch)
    {
        len = strlen(itemp->text);
        if (inp->limit > 0 && inp->limit <= FIELD_MAX_TEXT)
        {
            if (len >= inp->limit)
            {
                return 1;
            }
        }
        if (inp->xcur < FIELD_MAX_TEXT)
        {
            char ch = ev->ch;
            if (flags & FIELD_INPUT_PASSWORD)
            {
                ch = '*';
            }
            else if (flags & FIELD_INPUT_NUMBER)
            {
                if (ch < '0' || ch > '9')
                {
                    return 1;
                }
            }
            else if (flags & FIELD_INPUT_HEXNUMBER)
            {
                if ((ch >= '0' && ch <= '9') ||
                    (ch >= 'a' && ch <= 'f') ||
                    (ch >= 'A' && ch <= 'F'))
                {
                    /*return 1;*/
                }
                else
                {
                    return 1;
                }
            }
            if (flags & FIELD_INPUT_CAPITAL)
            {
                if (ch >= 'a' && ch <= 'z')
                {
                    ch = (ch - 'a') + 'A';
                }
            }
            
            if (flags & FIELD_INPUT_PASSWORD)
            {
                inp->password[inp->xcur] = ev->ch;
            }
            itemp->text[inp->xcur] = ch;
            ++inp->xcur;
            /*len = strlen(itemp->text);*/
            ++len;
            if (len >= width)
            {
                inp->xpos = len - width;
            }
        }
    }
    return 1;
}

int tu_lbx__process_event(tu_wnditem_t* itemp, struct tb_event* ev);

int field_process_event(tu_wnditem_t* itemp, struct tb_event* evp)
{
    switch (itemp->type)
    {
        case FIELD_INPUT:
            return tu_input__process_event(itemp, evp);
        case FIELD_LISTBOX:
            return tu_lbx__process_event(itemp, evp);
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
        if (itemp == wndp->activep)
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
                wndp->activep = NULL;
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
            break;
        }
    }
}

tu_wnditem_t*  tu_wnd_addfield(tu_window_t* wndp, tu_field_t* fldp)
{
    tu_wnditem_t*   newp = 0;

    tu_wnditem_t    item;
    int             rc = 0;
    memset(&item, 0, sizeof(struct tu_wnditem));
    memcpy(&item, fldp, sizeof(struct tu_field));
    strcpy(item.text, fldp->text);
    rc = jsw_rbinsert(wndp->children, &item);
    if (1 == rc) /*insert successfully*/
    {
        newp = (tu_wnditem_t*)jsw_rbfind(wndp->children, &item);
        tu_list_pushback(wndp->fields, newp, 0);
        newp->node  = tu_list_last(wndp->fields);
        newp->parent = wndp;
        
        tu_wnditem__init(newp);
        /*activep*/
        if ( NULL == wndp->activep )
        {
            if (newp->enable && newp->visible)
            {
                if (newp->type == FIELD_INPUT)
                {
                    tu_input_t* inp = (tu_input_t*)newp;
                    inp->first_focus = 1;
                }
                wndp->activep = newp;
            }
        }
    }
    return newp;
}

tu_wnditem_t* tu_wnd_getactive(tu_window_t* wndp)
{
    return (wndp->activep);
}

tu_wnditem_t* tu_wnd_setactive(tu_window_t* wndp, int id)
{
    tu_wnditem_t* activep = tu_wnd_getactive(wndp);
    tu_wnditem_t* newp = tu_wnd_getfield(wndp, id);
    if (NULL == newp)
    {
        if (newp->enable && newp->visible)
        {
            tu_input_t* inp = 0;
            if (activep->type == FIELD_INPUT)
            {
                inp = (tu_input_t*)wndp->activep;
                inp->first_focus = 0;
                tu_wnditem_draw(wndp->activep);
            }
            if (newp->type == FIELD_INPUT)
            {
                inp = (tu_input_t*)wndp->activep;
                inp->first_focus = 1;
                tu_wnditem_draw(newp);
            }
            wndp->activep = newp;
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
    itemp = tu_wnd__getinput(wndp);
    if (itemp)
    {
        tu_wnditem_draw(itemp);
    }
}

void tu_wnd_setevent(tu_window_t* wndp, int event, 
    int (*on_event)(int mod, int key, int ch, tu_notify_t* nofity))
{
    switch (event)
    {
        case FIELD_EV_KEYDOWN:
            wndp->on_keydown = on_event;
            break;
        case FIELD_EV_BLUR:
            wndp->on_blur = on_event;
            break;
        case FIELD_EV_FOCUS:
            wndp->on_focus = on_event;
            break;
        case FIELD_EV_NOTIFY:
            wndp->on_notify = on_event;
            break;
    }
}

/*input*/
void tu_inp_setlimit(tu_input_t* inp, int limit)
{
    inp->limit = limit;
}

int tu_inp_setpassword(tu_input_t* inp, const char* password)
{
    strncpy(inp->password, password, FIELD_MAX_TEXT);
    return 0;
}

int tu_inp_getpassword(tu_input_t* inp, char* password, int len)
{
    if (len > FIELD_MAX_TEXT)
    {
        len = strlen(inp->password);
    }
    strncpy(password, inp->password, len);
    return 0;
}

int tu_inp_setnumber(tu_input_t* inp, int number)
{
    return sprintf(inp->item.text, "%d", number);
}

int tu_inp_getnumber(tu_input_t* inp)
{
    return atoi(inp->item.text);
}

/*listbox*/
int tu_lbx_addheader(tu_listbox_t* lbxp, tu_header_t* hdrp, int nheaders)
{
    int i = 0;
    tu_wnditem_t* itemp = &lbxp->item;
    int w = itemp->w;
    int cw = 0;
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
    
    /*find the min right column*/
    for (i = nheaders - 1; i >= 0; --i)
    {
        if (cw + hdrp[i].w < w)
        {
            lbxp->rightcol = i;
        }
        else
        {
            break;
        }
        cw += hdrp[i].w;
    }
    return 0;
}

int tu_lbx_add(tu_listbox_t* lbxp, tu_subitem_t* itemp, int nitems)
{
    int i   = 0;
    int rc  = 0;
    int row = lbxp->nrows;
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
        subitemp[i].data         = itemp[i].data;
        strcpy(subitemp[i].text, itemp[i].text);
    }
    /*increment the row number*/
    ++lbxp->nrows;
    return row;
}

tu_listsubitem_t*   tu_lbx__get(tu_listbox_t* lbxp, int row)
{
    int i = 0;
    tu_listnode_t* node = tu_list_first(lbxp->rowp);
    tu_listsubitem_t* subitemp = 0;
    if (row < 0 || row >= lbxp->nrows)
    {
        return NULL;
    }
    while (node && i != row)
    {
        node = tu_list_next(node);
        ++i;
    }
    if (node && i == row)
    {
        subitemp = (tu_listsubitem_t*)tu_list_data(node);
    }
    return subitemp; 
}

#if 0
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
    int     attribs;
    void*   data;
    char    text[FIELD_MAX_TEXT + 1];
    tu_listnode_t*    node;
};
typedef struct tu_listsubitem tu_listsubitem_t;
#endif

int tu_lbx__maxcol(tu_listbox_t* lbxp)
{
    tu_wnditem_t* itemp = &lbxp->item;
    int i = 0;
    int w = itemp->w;
    int cw = 0;
    tu_listheader_t* hdrp = 0;

    for (i = lbxp->curcol; i < lbxp->nheaders; ++i)
    {
        if (cw > w)
        {
            return (i + 1);
        }
        hdrp = &lbxp->hdrp[i];
        cw += hdrp->w;
    }
    return (lbxp->nheaders);
}

int tu_lbx__itemspage(tu_listbox_t* lbxp)
{
    tu_wnditem_t* itemp = &lbxp->item;
    int height = itemp->h;
    if (FIELD_LISTBOX_HIDEHEADER & itemp->flags)
    {
        height = height;
    }
    else
    {
        height -= 1;
    }
    return height;
}

int tu_lbx__getpage(tu_listbox_t* lbxp, int row)
{
    int items = tu_lbx__itemspage(lbxp);
    int pages = (items > 0 ? lbxp->nrows / items : 0);
    if (items <= 0)
    {
        /*height too small*/
        return -1;
    }
    if (row < 0)
    {
        /*out-of-bound*/
        return 0;
    }
    else if (row >= lbxp->nrows)
    {
        return pages;
    }
    return (row / items);
}
/*
tu_lbx__getfirstvisible()
    return: the first visible row of the page
*/
int tu_lbx__getfirstvisible(tu_listbox_t* lbxp)
{
    int items = tu_lbx__itemspage(lbxp);
    int curpage = 0;
    
    if (items <= 0)
    {
        return -1;
    }
    curpage = tu_lbx__getpage(lbxp, lbxp->currow);
    if (curpage < 0)
    {
        return curpage;
    }
    return (curpage * items);
}

int tu_lbx__getlastvisible(tu_listbox_t* lbxp)
{
    int items = tu_lbx__itemspage(lbxp);
    int row = tu_lbx__getfirstvisible(lbxp);
    if (row < 0)
    {
        return row;
    }
    return (row + items);
}

int tu_lbx__process_event(tu_wnditem_t* itemp, struct tb_event* ev)
{
    tu_listbox_t* lbxp = (tu_listbox_t*)itemp;
    int items = tu_lbx__itemspage(lbxp);
    int currow = lbxp->currow;
    int curcol = lbxp->curcol;

    if (ev->key)
    {
        switch (ev->key)
        {
            case TB_KEY_ARROW_UP:
                --currow;
                break;
            case TB_KEY_ARROW_DOWN:
                ++currow;
                break;
            case TB_KEY_PGUP:
                currow -= items;
                break;
            case TB_KEY_PGDN:
                currow += items;
                break;
            case TB_KEY_ARROW_LEFT:
                --curcol;
                break;
            case TB_KEY_ARROW_RIGHT:
                ++curcol;
                break;
        }
        if (currow < 0)
        {
            currow = 0;
        }
        else if (currow >= lbxp->nrows)
        {
            currow = lbxp->nrows - 1;
        }
        if (curcol < 0)
        {
            curcol = 0;
        }
        else if (curcol > lbxp->rightcol)/*lbxp->nheaders)*/
        {
            curcol = lbxp->rightcol;/*lbxp->nheaders - 1;*/
        }

        lbxp->curcol = curcol;
        lbxp->currow = currow;
        
        tu_lbx__draw(lbxp);
    }
    return 1;
}

void tu_lbx__drawheader(tu_listbox_t* lbxp)
{
    tu_wnditem_t* itemp = &lbxp->item;
    int i = 0;
    int x = itemp->x;
    int y = itemp->y;
    int w = itemp->w;
    int h = itemp->h;
    int cw = 0;
    tu_listheader_t* hdrp = 0;
    
    /*fill blank to cleanup the line*/
    tu_fillbox(x, y, w, 1, ' ', 0, 0, 0);
    
    /*fill each column header*/
    for (i = lbxp->curcol; i < lbxp->nheaders; ++i)
    {
        hdrp = &lbxp->hdrp[i];
        if (cw + hdrp->w > w)
        {
            tu_drawtext(x, y, (w - cw), 
                hdrp->text, 
                hdrp->fgcolor, hdrp->bgcolor, 
                hdrp->alignment, 
                hdrp->attribs | FIELD_UNDERLINE, 0);
            break;
        }
        
        tu_drawtext(x, y, hdrp->w, 
            hdrp->text, 
            hdrp->fgcolor, hdrp->bgcolor, 
            hdrp->alignment, 
            hdrp->attribs | FIELD_UNDERLINE, 0);

        cw += hdrp->w;
        x  += hdrp->w;
    }
}

void tu_lbx__drawrows(tu_listbox_t* lbxp, int y)
{
    tu_wnditem_t* itemp = &lbxp->item;
    int i = 0;  /*column*/
    int x = itemp->x;
    /*int y = itemp->y;*/
    int w = itemp->w;
    int h = itemp->h;
    int cw = 0;
    tu_listheader_t* hdrp = 0;
    tu_listsubitem_t* subitemp = 0;
    int startrow = tu_lbx__getfirstvisible(lbxp);
    int endrow   = tu_lbx__getlastvisible(lbxp);
    int nitems   = tu_lbx__itemspage(lbxp);
    int j = 0;  /*row*/
    int currow   = lbxp->currow;

    /*fill each row*/
    for (j = 0; j < nitems; ++j)
    {
        x  = itemp->x;
        cw = 0;
        /*fill blank to cleanup the line*/
        tu_fillbox(x, y + j, w, 1, ' ', 0, 0, 0);
        if (startrow + j >= lbxp->nrows)
        {
            continue; /*just filled the blank*/
        }
        subitemp = tu_lbx__get(lbxp, startrow + j);
        /*fill each column header*/
        for (i = lbxp->curcol; i < lbxp->nheaders; ++i)
        {
            hdrp = &lbxp->hdrp[i];
            if (cw + hdrp->w > w)
            {
                /*the last column left*/
                if (currow == (startrow + j))
                {
                    tu_drawtext(x, y + j, (w - cw), 
                        subitemp[i].text, 
                        subitemp[i].fgcolor, subitemp[i].bgcolor, 
                        hdrp->alignment, 
                        hdrp->attribs | FIELD_REVERSE, 0);
                    tb_set_cursor(x + (w - cw), y + j);
                }
                else
                {
                    tu_drawtext(x, y + j, (w - cw), 
                        subitemp[i].text, 
                        subitemp[i].fgcolor, subitemp[i].bgcolor, 
                        hdrp->alignment, 
                        hdrp->attribs, 0);
                }
                break;
            }
            /*draw item*/
            if (currow == (startrow + j))
            {
                tu_drawtext(x, y + j, hdrp->w, 
                    subitemp[i].text, 
                    subitemp[i].fgcolor, subitemp[i].bgcolor, 
                    hdrp->alignment, 
                    hdrp->attribs | FIELD_REVERSE, 0);
                tb_set_cursor(x + hdrp->w, y + j);
            }
            else
            {
                tu_drawtext(x, y + j, hdrp->w, 
                    subitemp[i].text, 
                    subitemp[i].fgcolor, subitemp[i].bgcolor, 
                    hdrp->alignment, 
                    hdrp->attribs, 0);
            }

            cw += hdrp->w;
            x  += hdrp->w;
        } /*column*/
    }   /*row*/
}

void tu_lbx__draw(tu_listbox_t* lbxp)
{
    tu_wnditem_t* itemp = &lbxp->item;
    int  y = itemp->y;
    unsigned int flags = itemp->flags;
    
    if (FIELD_LISTBOX_HIDEHEADER & flags)
    {
        tu_lbx__drawrows(lbxp, y);
    }
    else
    {
        tu_lbx__drawheader(lbxp);
        tu_lbx__drawrows(lbxp, y + 1);
    }
    
    tb_present();
}

int tu_lbx_remove(tu_listbox_t* lbxp, int row)
{
    tu_listsubitem_t* subitemp = tu_lbx__get(lbxp, row);
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

int   tu_lbx_get(tu_listbox_t* lbxp, int row, tu_subitem_t* itemp, int nitems)
{
    int i = 0;
    tu_listsubitem_t* subitemp = 0;
    
    if ((nitems <= 0) ||
        (nitems > 0 && nitems > lbxp->nheaders))
    {
        nitems = lbxp->nheaders;
    }
    subitemp = tu_lbx__get(lbxp, row);
    if (subitemp)
    {
        /*copy data*/
        for (i = 0; i < nitems; ++i)
        {
            itemp[i].fgcolor    = subitemp[i].fgcolor;
            itemp[i].bgcolor    = subitemp[i].bgcolor;
            itemp[i].data       = subitemp[i].data;
            strcpy(itemp[i].text, subitemp[i].text);
        }
    }
    return 0;
}

int tu_lbx_set(tu_listbox_t* lbxp, int row, tu_subitem_t* itemp, int nitems)
{
    int i = 0;
    tu_listsubitem_t* subitemp = 0;
    
    if ((nitems <= 0) ||
        (nitems > 0 && nitems > lbxp->nheaders))
    {
        nitems = lbxp->nheaders;
    }
    subitemp = tu_lbx__get(lbxp, row);
    if (subitemp)
    {
        /*copy data*/
        for (i = 0; i < nitems; ++i)
        {
            subitemp[i].fgcolor      = itemp[i].fgcolor;
            subitemp[i].bgcolor      = itemp[i].bgcolor;
            subitemp[i].data         = itemp[i].data;
            strcpy(subitemp[i].text, itemp[i].text);
        }
    }
    return 0;
}

int tu_lbx_setcursel(tu_listbox_t* lbxp, int row, int redraw)
{
    if (lbxp->nrows > 0)
    {
        if (row < 0)
        {
            row = 0;
        }
        else if (row >= lbxp->nrows)
        {
            row = lbxp->nrows - 1;
        }
        lbxp->currow = row;
        if (redraw)
        {
            tu_lbx__draw(lbxp);
        }
    }
}

int tu_lbx_getcursel(tu_listbox_t* lbxp)
{
    return lbxp->currow;
}

static int tu_lbx__cmpcol(const void* p1, const void* p2)
{
    const tu_row_t* r1p = (const tu_row_t*)p1;
    const tu_row_t* r2p = (const tu_row_t*)p2;
    int col = r1p->subitems[0].sorting;
    
    const tu_listsubitem_t* sub1p = &r1p->subitems[col];
    const tu_listsubitem_t* sub2p = &r2p->subitems[col];
    return strcmp(sub1p->text, sub2p->text);
}

void tu_lbx_sort(tu_listbox_t* lbxp, int col)
{
    /*create a temp array*/
    int rowsize = sizeof(struct tu_listsubitem) * 10;/*lbxp->nheaders;*/
    int rownode = sizeof(struct tu_listsubitem) * lbxp->nheaders;
    int tabsize = rowsize * lbxp->nrows;
    tu_listnode_t* nodep = tu_list_first(lbxp->rowp);
    tu_row_t* tabp = 0;
    int i = 0;
    int j = 0;
    tu_listsubitem_t* srcp = 0;
    tu_listsubitem_t* dstp = 0;
    tu_listnode_t* dstnodep = 0;

    if (lbxp->nrows < 2)
    {
        /*zero or one row*/
        return;
    }
    else if (lbxp->nheaders >= 10)
    {
        /*cannot sort the table if the column is too much*/
        return;
    }
    if (col < 0 || col >= lbxp->nheaders)
    {
        col = 0;
    }
    /*copy to the temp table*/
    tabp = (tu_row_t*)calloc(lbxp->nrows, sizeof(struct tu_row));
    while (nodep)
    {
        memcpy(tabp[i].subitems, (tu_listsubitem_t*)tu_list_data(nodep), rownode);
        tabp[i].subitems[0].sorting = col;
        nodep = tu_list_next(nodep);
        ++i;
    }
    /*sort it now*/
    qsort(tabp, lbxp->nrows, rowsize, tu_lbx__cmpcol);
    
    /*copy back from the table to the linklist*/
    nodep = tu_list_first(lbxp->rowp);
    for (i = 0; i < lbxp->nrows; ++i)
    {
        for (j = 0; j < lbxp->nheaders; ++j)
        {
            srcp = tabp[i].subitems;
            dstp = (tu_listsubitem_t*)tu_list_data(nodep);
            dstnodep = dstp->node;
            
            memcpy(dstp, srcp, rownode);
            dstp->node = dstnodep;
        }
        nodep = tu_list_next(nodep);
    }
    /*free it*/
    free(tabp);
}

int tu_lbx_setitem(tu_listbox_t* lbxp, int row, int col, tu_subitem_t* itemp)
{
    tu_listsubitem_t* subitemp = 0;
    if (col < 0 || col >= lbxp->nheaders)
    {
        return -1;
    }
    subitemp = tu_lbx__get(lbxp, row);
    if (subitemp)
    {
        subitemp[col].fgcolor   = itemp->fgcolor;
        subitemp[col].bgcolor   = itemp->bgcolor;
        subitemp[col].data      = itemp->data;
        strcpy(subitemp->text, itemp->text);
    }
    return 0;
}

int tu_lbx_getitem(tu_listbox_t* lbxp, int row, int col, tu_subitem_t* itemp)
{
    tu_listsubitem_t* subitemp = 0;
    if (col < 0 || col >= lbxp->nheaders)
    {
        return -1;
    }
    subitemp = tu_lbx__get(lbxp, row);
    if (subitemp)
    {
        itemp->fgcolor = subitemp[col].fgcolor;
        itemp->bgcolor = subitemp[col].bgcolor;
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
    lbxp->curcol    = 0;
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
    lbxp->curcol    = 0;
}
