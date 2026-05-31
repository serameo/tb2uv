/*
File:       tu_menu_json.h
Purpose:    Load tu_menu_t widget trees from a JSON file (uses cJSON)
Author:     Seree R.
Date:       29-MAY-2026
*/
#ifndef __TU_MENU_JSON_H__
#define __TU_MENU_JSON_H__

#include "tb2uv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
tu_menu_load_json()
    Parse a JSON file and create a full menu/submenu widget tree inside wndp.
    Returns the root tu_menu_t* on success, NULL on failure.

    Root menu is placed on the default layer (always visible).
    Each submenu gets its own hidden layer; it becomes visible when activated.

    JSON format:
    {
      "id":    <int>,        required: widget ID (must be unique in the window)
      "x":     <int>,        required for root
      "y":     <int>,        required for root
      "w":     <int>,        required: total width  (includes border)
      "h":     <int>,        required: total height (includes border)
      "fg":    <int>,        optional: FIELD_* foreground color
      "bg":    <int>,        optional: FIELD_* background color
      "items": [             required
        {
          "text":      <string>,   required: display text
          "id":        <int>,      optional: action id (tu_mnu_getitemid)
          "separator": true,       optional: marks row as non-selectable divider
          "submenu":   { ... }     optional: nested menu (same schema; no x/y needed)
        },
        ...
      ]
    }

    Submenus are positioned automatically when opened (right of parent, clamped
    to terminal dimensions).  You do not need to specify x/y for submenus.
*/
tu_menu_t* tu_menu_load_json(tu_window_t* wndp, const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /*__TU_MENU_JSON_H__*/
