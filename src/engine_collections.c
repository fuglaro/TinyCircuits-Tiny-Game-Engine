#include "engine_collections.h"


linked_list engine_camera_nodes_collection;
linked_list engine_physics_nodes_collection;
linked_list engine_gui_nodes_collection;
linked_list engine_deletable_nodes_collection;


linked_list_node *engine_collections_track_camera(engine_node_base_t *camera_node_base){
    return linked_list_add_obj(&engine_camera_nodes_collection, camera_node_base);
}

void engine_collections_untrack_camera(linked_list_node *camera_list_node){
    linked_list_del_list_node(&engine_camera_nodes_collection, camera_list_node);
}


linked_list_node *engine_collections_track_physics(engine_node_base_t *physics_node_base){
    return linked_list_add_obj(&engine_physics_nodes_collection, physics_node_base);
}

void engine_collections_untrack_physics(linked_list_node *physics_list_node){
    linked_list_del_list_node(&engine_physics_nodes_collection, physics_list_node);
}


linked_list_node *engine_collections_track_gui(engine_node_base_t *gui_node_base){
    return linked_list_add_obj(&engine_gui_nodes_collection, gui_node_base);
}

void engine_collections_untrack_gui(linked_list_node *gui_list_node){
    linked_list_del_list_node(&engine_gui_nodes_collection, gui_list_node);
}

linked_list_node *engine_collections_track_deletable(engine_node_base_t *node_base){
    return linked_list_add_obj(&engine_deletable_nodes_collection, node_base);
}

void engine_collections_untrack_deletable(linked_list_node *node_base_node){
    linked_list_del_list_node(&engine_deletable_nodes_collection, node_base_node);
}


linked_list *engine_collections_get_camera_list(){
    return &engine_camera_nodes_collection;
}


linked_list *engine_collections_get_physics_list(){
    return &engine_physics_nodes_collection;
}


linked_list *engine_collections_get_gui_list(){
    return &engine_gui_nodes_collection;
}


linked_list *engine_collections_get_deletable_list(){
    return &engine_deletable_nodes_collection;
}