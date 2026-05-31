/*
File:       tu_menubar_json.h
Purpose:    Load a tu_menubar_t widget tree (bar + all dropdown menus) from JSON.
*/
#ifndef __TU_MENUBAR_JSON_H__
#define __TU_MENUBAR_JSON_H__

#include "tb2uv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
tu_menubar_load_json()
    Parse a JSON file and create a fully wired menubar + dropdown tree inside wndp.
    Returns the tu_menubar_t* on success, NULL on failure.

    The menubar goes on the default (always visible) layer.
    Each dropdown menu gets its own hidden layer; it becomes visible when activated.

    JSON format:
    {
      "id":   <int>,     required: menubar widget ID
      "x":    <int>,     required: x position
      "y":    <int>,     required: y position
      "w":    <int>,     required: bar display width
      "fg":   <int>,     optional: foreground color (FIELD_* constant)
      "bg":   <int>,     optional: background color (FIELD_* constant)
      "entries": [
        {
          "text": <string>,   title shown in the bar (e.g. " File ")
          "menu": {           dropdown definition
            "id":   <int>,    required: widget ID (must be unique)
            "w":    <int>,    required: dropdown width (includes border)
            "h":    <int>,    required: dropdown height (includes border)
            "fg":   <int>,    optional
            "bg":   <int>,    optional
            "items": [
              {
                "text":      <string>,
                "id":        <int>,      optional: action ID
                "separator": true,       optional: non-selectable divider
                "submenu":   { ... }     optional: nested submenu (recursive)
              }, ...
            ]
          }
        }, ...
      ]
    }
*/
tu_menubar_t* tu_menubar_load_json(tu_window_t* wndp, const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /*__TU_MENUBAR_JSON_H__*/
