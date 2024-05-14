#ifndef ENGINE_GUI_H
#define ENGINE_GUI_H

#include "utility/linked_list.h"
#include "nodes/node_base.h"
#include "py/obj.h"

void engine_gui_reset();

void engine_gui_focus_node(engine_node_base_t *gui_node);
bool engine_gui_get_focus();
void engine_gui_toggle_focus();
engine_node_base_t *engine_gui_get_focused();

void engine_gui_tick();

#endif  // ENGINE_GUI_H