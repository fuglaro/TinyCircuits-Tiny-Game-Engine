#include "line_2d_node.h"

#include "py/objstr.h"
#include "py/objtype.h"
#include "nodes/node_types.h"
#include "nodes/node_base.h"
#include "debug/debug_print.h"
#include "engine_object_layers.h"
#include "math/vector3.h"
#include "math/rectangle.h"
#include "draw/engine_display_draw.h"
#include "math/engine_math.h"


// Class required functions
STATIC void line_2d_node_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    ENGINE_INFO_PRINTF("print(): Line2DNode");
}


STATIC mp_obj_t line_2d_node_class_tick(mp_obj_t self_in){
    ENGINE_WARNING_PRINTF("Line2DNode: Tick function not overridden");
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(line_2d_node_class_tick_obj, line_2d_node_class_tick);


STATIC mp_obj_t line_2d_node_class_draw(mp_obj_t self_in, mp_obj_t camera_node){
    ENGINE_INFO_PRINTF("Line2DNode: Drawing");
    
    // Decode and store properties about the line and camera nodes
    engine_node_base_t *line_node_base = self_in;   // Do not need `node_base_get` since the draw function is always fed an engine_node_base (unlike the attr functions)
    engine_line_2d_node_class_obj_t *line_2d = line_node_base->node;

    engine_node_base_t *camera_node_base = camera_node;

    vector2_class_obj_t *line_start = line_2d->start;
    vector2_class_obj_t *line_end = line_2d->end;
    float line_thickness = mp_obj_get_float(line_2d->thickness);
    uint16_t line_color = mp_obj_get_float(line_2d->color);
    bool line_outlined = mp_obj_get_int(line_2d->outline);

    // The line is drawn as a rectangle since we have a nice algorithm for doing that:
    float line_length = engine_math_distance_between(line_start->x, line_start->y, line_end->x, line_end->y);
    float line_rotation = engine_math_angle_between(line_start->x, line_start->y, line_end->x, line_end->y) - HALF_PI;
    line_rotation *= -1.0f; // https://stackoverflow.com/a/62486304

    // Grab camera
    vector3_class_obj_t *camera_position = mp_load_attr(camera_node_base->attr_accessor, MP_QSTR_position);
    rectangle_class_obj_t *camera_viewport = mp_load_attr(camera_node_base->attr_accessor, MP_QSTR_viewport);
    float camera_zoom = mp_obj_get_float(mp_load_attr(camera_node_base->attr_accessor, MP_QSTR_zoom));

    // Get camera transformation if it is a child
    float camera_resolved_hierarchy_x = 0.0f;
    float camera_resolved_hierarchy_y = 0.0f;
    float camera_resolved_hierarchy_rotation = 0.0f;
    node_base_get_child_absolute_xy(&camera_resolved_hierarchy_x, &camera_resolved_hierarchy_y, &camera_resolved_hierarchy_rotation, NULL, camera_node);
    camera_resolved_hierarchy_rotation = -camera_resolved_hierarchy_rotation;

    // Get line transformation if it is a child
    float line_resolved_hierarchy_x = 0.0f;
    float line_resolved_hierarchy_y = 0.0f;
    float line_resolved_hierarchy_rotation = 0.0f;
    bool line_is_child_of_camera = false;
    node_base_get_child_absolute_xy(&line_resolved_hierarchy_x, &line_resolved_hierarchy_y, &line_resolved_hierarchy_rotation, &line_is_child_of_camera, self_in);

    // Store the non-rotated x and y for a second
    float line_rotated_x = line_resolved_hierarchy_x-camera_resolved_hierarchy_x;
    float line_rotated_y = line_resolved_hierarchy_y-camera_resolved_hierarchy_y;

    // Scale transformation due to camera zoom
    if(line_is_child_of_camera == false){
        engine_math_scale_point(&line_rotated_x, &line_rotated_y, camera_position->x, camera_position->y, camera_zoom);
    }else{
        camera_zoom = 1.0f;
    }

    // Scale by camera
    line_thickness = line_thickness*camera_zoom;
    line_length = line_length*camera_zoom;

    // Rotate rectangle origin about the camera
    engine_math_rotate_point(&line_rotated_x, &line_rotated_y, 0, 0, camera_resolved_hierarchy_rotation);

    line_rotated_x += camera_viewport->width/2;
    line_rotated_y += camera_viewport->height/2;

    if(line_outlined == false){
        engine_draw_fillrect_scale_rotate_viewport(line_color,
                                                (int32_t)line_rotated_x,
                                                (int32_t)line_rotated_y,
                                                line_thickness, 
                                                line_length,
                                                (int32_t)(1.0f*65536 + 0.5),
                                                (int32_t)(1.0f*65536 + 0.5),
                                                (int16_t)(((line_resolved_hierarchy_rotation+line_rotation+camera_resolved_hierarchy_rotation))*1024 / (float)(2*PI)),
                                                (int32_t)camera_viewport->x,
                                                (int32_t)camera_viewport->y,
                                                (int32_t)camera_viewport->width,
                                                (int32_t)camera_viewport->height);
    }else{
        float line_half_width = line_thickness/2;
        float line_half_height = line_length/2;

        // Calculate the coordinates of the 4 corners of the line, not rotated
        // NOTE: positive y is down
        float tlx = line_rotated_x - line_half_width;
        float tly = line_rotated_y - line_half_height;

        float trx = line_rotated_x + line_half_width;
        float try = line_rotated_y - line_half_height;

        float brx = line_rotated_x + line_half_width;
        float bry = line_rotated_y + line_half_height;

        float blx = line_rotated_x - line_half_width;
        float bly = line_rotated_y + line_half_height;

        // Rotate the points and then draw lines between them
        float angle = line_resolved_hierarchy_rotation + camera_resolved_hierarchy_rotation;
        engine_math_rotate_point(&tlx, &tly, line_rotated_x, line_rotated_y, angle);
        engine_math_rotate_point(&trx, &try, line_rotated_x, line_rotated_y, angle);
        engine_math_rotate_point(&brx, &bry, line_rotated_x, line_rotated_y, angle);
        engine_math_rotate_point(&blx, &bly, line_rotated_x, line_rotated_y, angle);

        engine_draw_line(line_color, tlx, tly, trx, try, camera_node);
        engine_draw_line(line_color, trx, try, brx, bry, camera_node);
        engine_draw_line(line_color, brx, bry, blx, bly, camera_node);
        engine_draw_line(line_color, blx, bly, tlx, tly, camera_node);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(line_2d_node_class_draw_obj, line_2d_node_class_draw);





void line_2d_recalculate_midpoint(engine_line_2d_node_class_obj_t *line){
    vector2_class_obj_t *start = line->start;
    vector2_class_obj_t *position = line->position;
    vector2_class_obj_t *end = line->end;

    engine_math_2d_midpoint(start->x, start->y, end->x, end->y, &position->x, &position->y);
}


void line_2d_translate_endpoints(engine_line_2d_node_class_obj_t *line, float nx, float ny){
    vector2_class_obj_t *start = line->start;
    vector2_class_obj_t *position = line->position;
    vector2_class_obj_t *end = line->end;

    float dx = nx - position->x;
    float dy = ny - position->y;

    start->x += dx;
    end->x += dx;

    start->y += dy;
    end->y += dy;
}


// Return `true` if handled loading the attr from internal structure, `false` otherwise
bool line_2d_load_attr(engine_line_2d_node_class_obj_t *self, qstr attribute, mp_obj_t *destination){
    switch(attribute){
        case MP_QSTR___del__:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_del_obj);
            destination[1] = self;
            return true;
        break;
        case MP_QSTR_add_child:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_add_child_obj);
            destination[1] = self;
            return true;
        break;
        case MP_QSTR_get_child:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_get_child_obj);
            destination[1] = self;
            return true;
        break;
        case MP_QSTR_remove_child:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_remove_child_obj);
            destination[1] = self;
            return true;
        break;
        case MP_QSTR_set_layer:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_set_layer_obj);
            destination[1] = self;
            return true;
        break;
        case MP_QSTR_get_layer:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_get_layer_obj);
            destination[1] = self;
            return true;
        break;
        case MP_QSTR_node_base:
            destination[0] = self;
            return true;
        break;
        case MP_QSTR_start:
            destination[0] = self->start;
            return true;
        break;
        case MP_QSTR_end:
            destination[0] = self->end;
            return true;
        break;
        case MP_QSTR_position:
            destination[0] = self->position;
            return true;
        break;
        case MP_QSTR_thickness:
            destination[0] = self->thickness;
            return true;
        break;
        case MP_QSTR_color:
            destination[0] = self->color;
            return true;
        break;
        case MP_QSTR_outline:
            destination[0] = self->outline;
            return true;
        break;
        default:
            return false; // Fail
    }
}


// Return `true` if handled storing the attr from internal structure, `false` otherwise
bool line_2d_store_attr(engine_line_2d_node_class_obj_t *self, qstr attribute, mp_obj_t *destination){
    switch(attribute){
        case MP_QSTR_start:
            self->start = destination[1];
            line_2d_recalculate_midpoint(self);
            return true;
        break;
        case MP_QSTR_end:
            self->end = destination[1];
            line_2d_recalculate_midpoint(self);
            return true;
        break;
        case MP_QSTR_position:
            // Offset `start` and `end` based on new position
            line_2d_translate_endpoints(self, ((vector2_class_obj_t*)destination[1])->x, ((vector2_class_obj_t*)destination[1])->y);
            self->position = destination[1];
            return true;
        break;
        case MP_QSTR_thickness:
            self->thickness = destination[1];
            return true;
        break;
        case MP_QSTR_color:
            self->color = destination[1];
            return true;
        break;
        case MP_QSTR_outline:
            self->outline = destination[1];
            return true;
        break;
        default:
            return false; // Fail
    }
}


STATIC mp_attr_fun_t line_2d_node_class_attr(mp_obj_t self_in, qstr attribute, mp_obj_t *destination){
    ENGINE_INFO_PRINTF("Accessing Line2DNode attr");

    // Get the node base from either class
    // instance or native instance object
    bool is_obj_instance = false;
    engine_node_base_t *node_base = node_base_get(self_in, &is_obj_instance);

    // Get the underlying structure
    engine_line_2d_node_class_obj_t *self = node_base->node;

    // Used for telling if custom load/store functions handled the attr
    bool attr_handled = false;

    if(destination[0] == MP_OBJ_NULL){          // Load
        attr_handled = line_2d_load_attr(self, attribute, destination);
    }else if(destination[1] != MP_OBJ_NULL){    // Store
        attr_handled = line_2d_store_attr(self, attribute, destination);

        // If handled, mark as successful store
        if(attr_handled) destination[0] = MP_OBJ_NULL;
    }

    // If this is a Python class instance and the attr was NOT
    // handled by the above, defer the attr to the instance attr
    // handler
    if(is_obj_instance && attr_handled == false){
        default_instance_attr_func(self_in, attribute, destination);
    }
}


/*  --- doc ---
    NAME: Line2DNode
    DESC: Draws a line from `start` to `end`. Changing `position` (the midpoint of the line) automaticaly translates `end` and `start`
    PARAM:  [type={ref_link:Vector2}]         [name=start]                      [value={ref_link:Vector2}]
    PARAM:  [type={ref_link:Vector2}]         [name=end]                        [value={ref_link:Vector2}]
    PARAM:  [type=float]                      [name=thickness]                  [value=any]
    PARAM:  [type=int]                        [name=color]                      [value=0 ~ 65535 (16-bit RGB565 0bRRRRRGGGGGGBBBBB)]   
    PARAM:  [type=bool]                       [name=outline]                    [value=True or False]
    ATTR:   [type=function]                   [name={ref_link:add_child}]       [value=function]
    ATTR:   [type=function]                   [name={ref_link:get_child}]       [value=function] 
    ATTR:   [type=function]                   [name={ref_link:remove_child}]    [value=function]
    ATTR:   [type=function]                   [name={ref_link:set_layer}]       [value=function]
    ATTR:   [type=function]                   [name={ref_link:get_layer}]       [value=function]
    ATTR:   [type=function]                   [name={ref_link:remove_child}]    [value=function]
    ATTR:   [type={ref_link:Vector2}]         [name=start]                      [value={ref_link:Vector2}]
    ATTR:   [type={ref_link:Vector2}]         [name=end]                        [value={ref_link:Vector2}]
    ATTR:   [type={ref_link:Vector2}]         [name=position]                   [value={ref_link:Vector2}]
    ATTR:   [type=float]                      [name=thickness]                  [value=any]
    ATTR:   [type=int]                        [name=color]                      [value=0 ~ 65535 (16-bit RGB565 0bRRRRRGGGGGGBBBBB)]   
    ATTR:   [type=bool]                       [name=outline]                    [value=True or False]
    OVRR:   [type=function]                   [name={ref_link:tick}]            [value=function]
    OVRR:   [type=function]                   [name={ref_link:draw}]            [value=function]
*/
mp_obj_t line_2d_node_class_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    ENGINE_INFO_PRINTF("New Line2DNode");

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_child_class,  MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_start,        MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_end,          MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_thickness,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_color,        MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_outline,      MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)];
    enum arg_ids {child_class, start, end, thickness, color, outline};
    bool inherited = false;

    // If there is one positional argument and it isn't the first 
    // expected argument (as is expected when using positional
    // arguments) then define which way to parse the arguments
    if(n_args >= 1 && mp_obj_get_type(args[0]) != &vector2_class_type){
        // Using positional arguments but the type of the first one isn't
        // as expected. Must be the child class
        mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args);
        inherited = true;
    }else{
        // Whether we're using positional arguments or not, prase them this
        // way. It's a requirement that the child class be passed using position.
        // Adjust what and where the arguments are parsed, since not inherited based
        // on the first argument
        mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args)-1, allowed_args+1, parsed_args+1);
        inherited = false;
    }

    if(parsed_args[start].u_obj == MP_OBJ_NULL) parsed_args[start].u_obj = vector2_class_new(&vector2_class_type, 2, 0, (mp_obj_t[]){mp_obj_new_float(0.0f), mp_obj_new_float(-5.0f)});
    if(parsed_args[end].u_obj == MP_OBJ_NULL) parsed_args[end].u_obj = vector2_class_new(&vector2_class_type, 2, 0, (mp_obj_t[]){mp_obj_new_float(0.0f), mp_obj_new_float(5.0f)});
    if(parsed_args[thickness].u_obj == MP_OBJ_NULL) parsed_args[thickness].u_obj = mp_obj_new_float(1.0f);
    if(parsed_args[color].u_obj == MP_OBJ_NULL) parsed_args[color].u_obj = mp_obj_new_int(0xffff);
    if(parsed_args[outline].u_obj == MP_OBJ_NULL) parsed_args[outline].u_obj = mp_obj_new_bool(false);

    engine_line_2d_node_common_data_t *common_data = malloc(sizeof(engine_line_2d_node_common_data_t));

    // All nodes are a engine_node_base_t node. Specific node data is stored in engine_node_base_t->node
    engine_node_base_t *node_base = m_new_obj_with_finaliser(engine_node_base_t);
    node_base_init(node_base, common_data, &engine_line_2d_node_class_type, NODE_TYPE_LINE_2D);

    engine_line_2d_node_class_obj_t *line_2d_node = m_malloc(sizeof(engine_line_2d_node_class_obj_t));
    node_base->node = line_2d_node;
    node_base->attr_accessor = node_base;

    common_data->tick_cb = MP_OBJ_FROM_PTR(&line_2d_node_class_tick_obj);
    common_data->draw_cb = MP_OBJ_FROM_PTR(&line_2d_node_class_draw_obj);

    line_2d_node->start = parsed_args[start].u_obj;
    line_2d_node->end = parsed_args[end].u_obj;
    line_2d_node->position = vector2_class_new(&vector2_class_type, 0, 0, NULL);
    line_2d_node->thickness = parsed_args[thickness].u_obj;
    line_2d_node->color = parsed_args[color].u_obj;
    line_2d_node->outline = parsed_args[outline].u_obj;

    if(inherited == true){
        // Look for function overrides otherwise use the defaults
        mp_obj_t dest[2];
        mp_load_method_maybe(node_base->node, MP_QSTR_tick, dest);
        if(dest[0] == MP_OBJ_NULL && dest[1] == MP_OBJ_NULL){   // Did not find method (set to default)
            common_data->tick_cb = MP_OBJ_FROM_PTR(&line_2d_node_class_tick_obj);
        }else{                                                  // Likely found method (could be attribute)
            common_data->tick_cb = dest[0];
        }

        mp_load_method_maybe(node_base->node, MP_QSTR_draw, dest);
        if(dest[0] == MP_OBJ_NULL && dest[1] == MP_OBJ_NULL){   // Did not find method (set to default)
            common_data->draw_cb = MP_OBJ_FROM_PTR(&line_2d_node_class_draw_obj);
        }else{                                                  // Likely found method (could be attribute)
            common_data->draw_cb = dest[0];
        }

        // Get the Python class instance
        mp_obj_t node_instance = parsed_args[child_class].u_obj;

        // Store one pointer on the instance. Need to be able to get the
        // node base that contains a pointer to the engine specific data we
        // care about
        // mp_store_attr(node_instance, MP_QSTR_node_base, node_base);
        mp_store_attr(node_instance, MP_QSTR_node_base, node_base);

        // Store default Python class instance attr function
        // and override with custom intercept attr function
        // so that certain callbacks/code can run (see py/objtype.c:mp_obj_instance_attr(...))
        default_instance_attr_func = MP_OBJ_TYPE_GET_SLOT((mp_obj_type_t*)((mp_obj_base_t*)node_instance)->type, attr);
        MP_OBJ_TYPE_SET_SLOT((mp_obj_type_t*)((mp_obj_base_t*)node_instance)->type, attr, line_2d_node_class_attr, 5);
    }

    // Calculate midpoint/position based on endpoints 
    // (only positions that can be set in the constructor)
    line_2d_recalculate_midpoint(line_2d_node);

    // When any part of the any of these Vector2s change, make sure to
    // recalculate other components of the line
    vector2_class_obj_t *line_start = line_2d_node->start;
    vector2_class_obj_t *line_position = line_2d_node->position;
    vector2_class_obj_t *line_end = line_2d_node->end;

    line_start->on_changed = &line_2d_recalculate_midpoint;
    line_start->on_change_user_ptr = line_2d_node;

    line_position->on_changing = &line_2d_translate_endpoints;
    line_position->on_change_user_ptr = line_2d_node;

    line_end->on_changed = &line_2d_recalculate_midpoint;
    line_end->on_change_user_ptr = line_2d_node;

    return MP_OBJ_FROM_PTR(node_base);
}


// Class attributes
STATIC const mp_rom_map_elem_t line_2d_node_class_locals_dict_table[] = {

};
STATIC MP_DEFINE_CONST_DICT(line_2d_node_class_locals_dict, line_2d_node_class_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    engine_line_2d_node_class_type,
    MP_QSTR_Line2DNode,
    MP_TYPE_FLAG_NONE,

    make_new, line_2d_node_class_new,
    print, line_2d_node_class_print,
    attr, line_2d_node_class_attr,
    locals_dict, &line_2d_node_class_locals_dict
);