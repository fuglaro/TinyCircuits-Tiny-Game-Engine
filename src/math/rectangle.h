#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "py/obj.h"

typedef struct{
    mp_obj_base_t base;
    float x;
    float y;
    float width;
    float height;
}rectangle_class_obj_t;

extern const mp_obj_type_t rectangle_class_type;

mp_obj_t rectangle_class_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

#endif  // RECTANGLE_H