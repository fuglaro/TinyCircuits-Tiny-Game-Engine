#include "py/obj.h"

#include "vector2.h"
#include "vector3.h"
#include "matrix4x4.h"
#include "rectangle.h"
#include "engine_main.h"


static mp_obj_t engine_math_module_init(){
    engine_main_raise_if_not_initialized();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(engine_math_module_init_obj, engine_math_module_init);
    

/*  --- doc ---
    NAME: engine_math
    ID: engine_math
    DESC: Module for common math operations and objects
    ATTR: [type=object]   [name={ref_link:Vector2}]     [value=object]
    ATTR: [type=object]   [name={ref_link:Vector3}]     [value=object]
    ATTR: [type=object]   [name={ref_link:Rectangle}]   [value=object]
*/
static const mp_rom_map_elem_t engine_math_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_engine_math) },
    { MP_OBJ_NEW_QSTR(MP_QSTR___init__), (mp_obj_t)&engine_math_module_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Vector2), (mp_obj_t)&vector2_class_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Vector3), (mp_obj_t)&vector3_class_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Matrix4x4), (mp_obj_t)&matrix4x4_class_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Rectangle), (mp_obj_t)&rectangle_class_type },
};

// Module init
static MP_DEFINE_CONST_DICT (mp_module_engine_math_globals, engine_math_globals_table);

const mp_obj_module_t engine_math_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_engine_math_globals,
};

MP_REGISTER_MODULE(MP_QSTR_engine_math, engine_math_user_cmodule);