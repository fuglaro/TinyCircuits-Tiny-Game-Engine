#ifndef PHYSICS_NODE_BASE_H
#define PHYSICS_NODE_BASE_H

#include "py/obj.h"
#include "node_base.h"

typedef struct{
    mp_obj_t position;                      // Vector2: 2d xy position of this node
    mp_obj_t velocity;                      // Vector2 (Absolute velocity)
    mp_obj_t acceleration;                  // Vector2
    mp_obj_t rotation;                      // float (Current rotation angle)
    mp_obj_t mass;                          // How heavy the node is
    mp_obj_t bounciness;                    // Restitution or elasticity

    mp_obj_t dynamic;                       // Flag indicating if node is dynamic and moving around due to physics or static
    mp_obj_t solid;                         // May want collision callbacks to happen without impeding objects, set to false

    mp_obj_t gravity_scale;                 // Vector2 allowing scaling affects of gravity. Set to 0,0 for no gravity

    uint8_t physics_id;

    float inverse_mass;

    void *unique_data;                      // Unique data about the collider (radius, width, height, etc.)

    mp_obj_t tick_cb;
    mp_obj_t collision_cb;
    linked_list_node *physics_list_node;    // All physics 2d nodes get added to a list that is easy to traverse
}engine_physics_node_base_t;

float physics_node_base_calculate_inverse_mass(engine_physics_node_base_t *physics_node_base);

// Return `true` if handled loading the attr from internal structure, `false` otherwise
bool physics_node_base_load_attr(engine_node_base_t *self_node_base, qstr attribute, mp_obj_t *destination);

// Return `true` if handled storing the attr from internal structure, `false` otherwise
bool physics_node_base_store_attr(engine_node_base_t *self_node_base, qstr attribute, mp_obj_t *destination);


#endif  // PHYSICS_NODE_BASE_H