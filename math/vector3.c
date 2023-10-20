#include "vector3.h"
#include "utility/debug_print.h"

// Class required functions
STATIC void vector3_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    vector3_class_obj_t *self = self_in;
    ENGINE_INFO_PRINTF("print(): Vector3 [%0.3f, %0.3f, %0.3f]", (double)self->x, (double)self->y, (double)self->z);
}

mp_obj_t vector3_class_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    ENGINE_INFO_PRINTF("New Vector3");
    mp_arg_check_num(n_args, n_kw, 0, 0, true);

    vector3_class_obj_t *self = m_new_obj(vector3_class_obj_t);
    self->base.type = &vector3_class_type;

    self->x = 0.0f;
    self->y = 0.0f;
    self->z = 0.0f;

    return MP_OBJ_FROM_PTR(self);
}


// Class methods

STATIC mp_obj_t vector3_class_test(){
    ENGINE_INFO_PRINTF("Vector3 test");

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(vector3_class_test_obj, vector3_class_test);


// Function called when accessing like print(my_node.position.x) (load 'x')
// my_node.position.x = 0 (store 'x').
// See https://micropython-usermod.readthedocs.io/en/latest/usermods_09.html#properties
// // See https://github.com/micropython/micropython/blob/91a3f183916e1514fbb8dc58ca5b77acc59d4346/extmod/modasyncio.c#L227
STATIC void vector3_class_attr(mp_obj_t self_in, qstr attribute, mp_obj_t *destination){
    vector3_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if(destination[0] == MP_OBJ_NULL){          // Load
        if(attribute == MP_QSTR_x){
            destination[0] = mp_obj_new_float(self->x);
        }else if(attribute == MP_QSTR_y){
            destination[0] = mp_obj_new_float(self->y);
        }else if(attribute == MP_QSTR_z){
            destination[0] = mp_obj_new_float(self->z);
        }
    }else if(destination[1] != MP_OBJ_NULL){    // Store
        if(attribute == MP_QSTR_x){
            self->x = mp_obj_get_float(destination[1]);
        }else if(attribute == MP_QSTR_y){
            self->y = mp_obj_get_float(destination[1]);
        }else if(attribute == MP_QSTR_z){
            self->z = mp_obj_get_float(destination[1]);
        }else{
            return;
        }

        // Success
        destination[0] = MP_OBJ_NULL;
    }
}


// Class attributes
STATIC const mp_rom_map_elem_t vector3_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_test), MP_ROM_PTR(&vector3_class_test_obj) },
};


// Class init
STATIC MP_DEFINE_CONST_DICT(vector3_class_locals_dict, vector3_class_locals_dict_table);

const mp_obj_type_t vector3_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_Vector3,
    .print = vector3_class_print,
    .make_new = vector3_class_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = vector3_class_attr,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .locals_dict = (mp_obj_dict_t*)&vector3_class_locals_dict,
};