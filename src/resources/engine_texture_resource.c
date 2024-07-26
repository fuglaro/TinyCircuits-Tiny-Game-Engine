#include "engine_texture_resource.h"
#include "debug/debug_print.h"
#include "draw/engine_color.h"
#include "resources/engine_resource_manager.h"
#include <stdlib.h>
#include <math.h>

#define RED_565_MASK  0b1111100000000000
#define RED_1555_MASK 0b0111110000000000

#define GREEN_565_MASK  0b0000011111100000
#define GREEN_1555_MASK 0b0000001111100000

#define BLUE_565_MASK  0b0000000000011111
#define BLUE_1555_MASK 0b0000000000011111

#define ALPHA_1555_MASK 0b1000000000000000
#define ALPHA_8888_MASK 0b11111111000000000000000000000000

enum bitmap_compression{
	BI_RGB,
	BI_RLE8,
	BI_RLE4,
	BI_BITFIELDS,
	BI_JPEG,
	BI_PNG,
	BI_ALPHABITFIELDS,
	BI_CMYK,
	BI_CMYKRLE8,
	BI_CMYKRLE4,
};


enum bitmap_formats{
    FMT_1BPP_PAL,
    FMT_4BPP_PAL,
    FMT_8BPP_PAL,

    FMT_16BPP_RGB565,
    FMT_16BPP_XRGB_1555,
    FMT_16BPP_ARGB_1555,

    FMT_24BPP_RGB888,

    FMT_32BPP_XRGB8888,
    FMT_32BPP_ARGB8888
};


// Make sure all structs are minimally packed
// so that file reads can go directly into these
// https://stackoverflow.com/a/7351714
#pragma pack(push)
#pragma pack(1)

typedef struct bmfh_t{                          // Bitmap header
    uint16_t bf_type;
    uint32_t bf_size;                           // Size of entire file
    uint16_t bf_reserved_1;
    uint16_t bf_reserved_2;
    uint32_t bf_off_bits;                       // Offset from 0 to start of pixel/index data
}bmfh_t;


typedef struct bmih_v1_t{                  
    uint32_t bi_size;                           // Size of this information section
	int32_t bi_width;                           // Width of bitmap in pixels
	int32_t bi_height;                          // Height of bitmap in pixels
	uint16_t bi_planes;
	uint16_t bi_bit_count;                      // Bit-depth of bitmap (1, 2, 4, 8, 16, 24, 32)
	uint32_t bi_compression;                    // Type of compression bitmap uses
	uint32_t bi_size_image;                     // Size of the bitmap image section but no always filled out by exporters
	int32_t bi_x_pels_per_meter;
	int32_t bi_y_pels_per_meter;
	uint32_t bi_clr_used;                       // Number of colors used if indexed image
	uint32_t bi_clr_important;
}bmih_v1_t;


typedef struct bmih_v2_t{                  
    uint32_t bi_red_mask;                       // Mask bits for red channel in pixel data (only useful for >= 16bpp formats)
	uint32_t bi_green_mask;                     // Mask bits for green channel in pixel data (only useful for >= 16bpp formats)
	uint32_t bi_blue_mask;                      // Mask bits for blue channel in pixel data (only useful for >= 16bpp formats)
}bmih_v2_t;


typedef struct bmih_v3_t{                  
    uint32_t bi_alpha_mask;                     // Mask bits for alpha channel in pixel data (only useful for >= 16bpp formats)
}bmih_v3_t;

#pragma pack(pop)


void bitmap_get_header_and_info(bmfh_t *header, bmih_v1_t *info_v1, bmih_v2_t *info_v2, bmih_v3_t *info_v3){
    // Read header: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader
    engine_file_read(0, header, 14);

    // Read info up to version we care about depending on amount of
    // data in the information section
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
    uint32_t read_info_len = 0;
    read_info_len += engine_file_read(0, info_v1, sizeof(bmih_v1_t));

    if(info_v1->bi_size > read_info_len){
        read_info_len += engine_file_read(0, info_v2, sizeof(bmih_v2_t));
    }

    if(info_v1->bi_size > read_info_len){
        read_info_len += engine_file_read(0, info_v3, sizeof(bmih_v3_t));
    }

    // Check that this is a bitmap and is using correct uncompressed formats
    if(header->bf_type != 19778){
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("TextureResource: BMP header ID field incorrect! Not a BMP file or file doesn't exist!"));
    }

    if(info_v1->bi_compression != BI_RGB && info_v1->bi_compression != BI_BITFIELDS){
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("TextureResource: Bitmap uses compression, only raw RGB is supported!"));
    }
}


void bitmap_get_and_fill_color_table(uint16_t *color_table, uint32_t color_table_offset, uint16_t color_count){
    // Seek to color table and create a temporary
    // buffer for the 24-bit color bytes
    engine_file_seek(0, color_table_offset, MP_SEEK_SET);
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-rgbquad
    // Color table in file is in bgr format (not rgb)
    uint8_t bgr[4];

    ENGINE_PRINTF("\t colors: \t\t\t");
    for(uint16_t color_index=0; color_index<color_count; color_index++){
        // Read the 4 bytes (last is reserved) from
        // color table and convert to 16 bit RGB565
        engine_file_read(0, bgr, 4);
        uint16_t rgb565 = engine_color_16_from_24_bit_rgb(bgr[2], bgr[1], bgr[0]);

        // Color table is always in RAM, write directly to it
        color_table[color_index] = rgb565;

        // Print the original color and converted RGB565
        ENGINE_PRINTF("%d,%d,%d->%d,%d,%d  ", bgr[2], bgr[1], bgr[0], (rgb565 >> 11) & 0b00011111, (rgb565 >> 5) & 0b00111111, (rgb565 >> 0) & 0b00011111);
    }
    ENGINE_PRINTF("\n");
}


void bitmap_handle_1bpp(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_1BPP_PAL\n");
    texture->format = FMT_1BPP_PAL;
}


void bitmap_handle_4bpp(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_4BPP_PAL\n");
    texture->format = FMT_4BPP_PAL;
}


void bitmap_handle_8bpp(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_8BPP_PAL\n");
    texture->format = FMT_8BPP_PAL;
}


void bitmap_handle_16bpp_rgb565(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_16BPP_RGB565\n");
    texture->format = FMT_16BPP_RGB565;
}


void bitmap_handle_16bpp_xrgb1555(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_16BPP_XRGB_1555\n");
    texture->format = FMT_16BPP_XRGB_1555;
}


void bitmap_handle_16bpp_argb1555(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_16BPP_ARGB_1555\n");
    texture->format = FMT_16BPP_ARGB_1555;
}


void bitmap_handle_24bpp_rgb888(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_24BPP_RGB888\n");
    texture->format = FMT_24BPP_RGB888;
}


void bitmap_handle_32bpp_xrgb8888(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_32BPP_XRGB8888\n");
    texture->format = FMT_32BPP_XRGB8888;
}


void bitmap_handle_32bpp_argb8888(texture_resource_class_obj_t *texture, uint8_t *data, uint32_t data_len){
    ENGINE_PRINTF("\t format: \t\t\tFMT_32BPP_ARGB8888\n");
    texture->format = FMT_32BPP_ARGB8888;
}


void bitmap_decipher_format(texture_resource_class_obj_t *texture, uint16_t color_count, bmih_v1_t *info_v1, bmih_v2_t *info_v2, bmih_v3_t *info_v3, uint8_t *data, uint32_t data_len){
    if(info_v1->bi_bit_count == 1){
        bitmap_handle_1bpp(texture, data, data_len);
    }else if(info_v1->bi_bit_count == 4){
        bitmap_handle_4bpp(texture, data, data_len);
    }else if(info_v1->bi_bit_count == 8){
        bitmap_handle_8bpp(texture, data, data_len);
    }else if(info_v1->bi_bit_count == 16){
        if(info_v2->bi_red_mask == RED_565_MASK && info_v2->bi_green_mask == GREEN_565_MASK && info_v2->bi_blue_mask == BLUE_565_MASK && info_v3->bi_alpha_mask == 0){
            bitmap_handle_16bpp_rgb565(texture, data, data_len);
        }else if(info_v2->bi_red_mask == RED_1555_MASK && info_v2->bi_green_mask == GREEN_1555_MASK && info_v2->bi_blue_mask == BLUE_1555_MASK && info_v3->bi_alpha_mask == 0){
            bitmap_handle_16bpp_xrgb1555(texture, data, data_len);
        }else if(info_v2->bi_red_mask == RED_1555_MASK && info_v2->bi_green_mask == GREEN_1555_MASK && info_v2->bi_blue_mask == BLUE_1555_MASK && info_v3->bi_alpha_mask == ALPHA_1555_MASK){
            bitmap_handle_16bpp_argb1555(texture, data, data_len);
        }else{
            bitmap_handle_16bpp_rgb565(texture, data, data_len);    // If none of the above, must be a v1 header and we'll assume 565 (not sure if best default)
        }
    }else if(info_v1->bi_bit_count == 24){
        bitmap_handle_24bpp_rgb888(texture, data, data_len);
    }else if(info_v1->bi_bit_count == 32){
        if(info_v3->bi_alpha_mask == 0){
            bitmap_handle_32bpp_xrgb8888(texture, data, data_len);
        }else if(info_v3->bi_alpha_mask == ALPHA_8888_MASK){
            bitmap_handle_32bpp_argb8888(texture, data, data_len);
        }
    }
}


mp_obj_t texture_resource_class_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    ENGINE_INFO_PRINTF("New TextureResource");
    mp_arg_check_num(n_args, n_kw, 1, 2, false);

    texture_resource_class_obj_t *self = mp_obj_malloc_with_finaliser(texture_resource_class_obj_t, &texture_resource_class_type);
    self->base.type = &texture_resource_class_type;

    // Set flag indicating if file data is to be stored in
    // ram or not (faster if stored in ram, up to programmer)
    if(n_args == 2){
        self->in_ram = mp_obj_get_int(args[1]);
    }else{
        self->in_ram = false;
    }

    // Always loaded into ram on unix port
    #if defined(__unix__)
        self->in_ram = true;
    #endif

    // BMP parsing: https://en.wikipedia.org/wiki/BMP_file_format
    // https://learn.microsoft.com/en-us/windows/win32/gdi/bitmap-storage
    // Variable names are from https://github.com/WerWolv/ImHex patterns
    engine_file_open_read(0, args[0]);
    engine_file_seek(0, 0, MP_SEEK_SET);

    // Basic information we need about the bitmap
    bmfh_t header;
    bmih_v1_t info_v1;
    bmih_v2_t info_v2;
    bmih_v3_t info_v3;
    bitmap_get_header_and_info(&header, &info_v1, &info_v2, &info_v3);

    uint32_t color_table_offset = 0;        // Offset to start of color table
    uint32_t color_table_size_in_file = 0;  // Number of bytes the color table is using in the file
    uint16_t color_count = 0;               // Number of 16-bit colors we will need (need to calculate this, not all bitmaps have the clr_used field filled out)
    uint16_t color_table_size = 0;          // Number of bytes needed to store all the 16-bit colors

    info_v1.bi_size_image = header.bf_size - header.bf_off_bits;                                    // Not all exporters fill out `bi_size_image`, calculate it instead
    if(info_v1.bi_bit_count < 16){
        color_table_offset = sizeof(bmfh_t) + info_v1.bi_size;                                      // Color table is after 14 bytes `bmfh` section and variable `bmih` section
        color_table_size_in_file = header.bf_size - (color_table_offset + info_v1.bi_size_image);   // If indexed bitmap, calculate size of file color dex table (consists of u32s)
        color_count = color_table_size_in_file / 4;                                                 // Number of colors in color table (might not use all available, so calculate it)
        color_table_size = color_count * 2;                                                         // How many bytes we need to store enough bytes for 16-bit versions of these colors
    }  

    // Print information
    ENGINE_PRINTF("TextureResource: BMP parameters parsed from '%s':\n", mp_obj_str_get_str(args[0]));
    ENGINE_PRINTF("\t bf_size: \t\t\t%d\n", header.bf_size);
    ENGINE_PRINTF("\t bf_off_bits: \t\t\t%lu\n", header.bf_off_bits);
    ENGINE_PRINTF("\t bi_size: \t\t\t%lu\n", info_v1.bi_size);
    ENGINE_PRINTF("\t bi_width: \t\t\t%lu\n", info_v1.bi_width);
    ENGINE_PRINTF("\t bi_height: \t\t\t%lu\n", info_v1.bi_height);
    ENGINE_PRINTF("\t bi_bit_count: \t\t\t%lu\n", info_v1.bi_bit_count);
    ENGINE_PRINTF("\t bi_size_image: \t\t%lu\n", info_v1.bi_size_image);
    ENGINE_PRINTF("\t color_table_offset: \t\t%lu\n", color_table_offset);
    ENGINE_PRINTF("\t color_table_size_in_file: \t%lu\n", color_table_size_in_file);
    ENGINE_PRINTF("\t color_count: \t\t\t%lu\n", color_count);
    ENGINE_PRINTF("\t color_table_size: \t\t%lu\n", color_table_size);

    // For bit-depths below 16 bits, the colors are stored
    // in a color table. The color table is 24-bits in the
    // file but will be converted to 16-bit RGB 565 so that
    // copying to the screen buffer is faster
    if(info_v1.bi_bit_count < 16){
        mp_obj_array_t *colors = engine_resource_get_space_bytearray(color_table_size, true);
        bitmap_get_and_fill_color_table((uint16_t*)colors->items, color_table_offset, color_count);
        self->colors = colors;
    }else{
        self->colors = mp_const_none;   // No color table for higher than 8 bit-depths
    }

    // Now that we know the bitmap information, fill out some
    // of the `TextureResource` attributes
    self->width = info_v1.bi_width;
    self->height = info_v1.bi_height;
    self->bit_depth = info_v1.bi_bit_count;
    mp_obj_array_t *data = engine_resource_get_space_bytearray(info_v1.bi_size_image, self->in_ram);

    // After getting the color table, should be at pixel data
    bitmap_decipher_format(self, color_count, &info_v1, &info_v2, &info_v3, data->items, data->len);
    self->data = data;

    engine_file_close(0);

    // // https://en.wikipedia.org/wiki/BMP_file_format#:~:text=optional%20color%20list.-,Pixel%20storage,-%5Bedit%5D
    // // Due to the format and padding to multiples of 4
    // // in bitmaps, an arrow drawn pointing up in a 7 x 7
    // // bitmap will be stored as (- is padding):
    // //
    // // X X X . X X X -
    // // X X X . X X X -
    // // X X X . X X X -
    // // X X X . X X X -
    // // X . X . X . X -
    // // X X . . . X X -
    // // X X X . X X X -
    // //
    // // This means, when using the serial storing API
    // // below, we need grab chunks of bitmap data from
    // // end to start, and store pixels going as:
    // // 1. Start at at the bottom-left of the above diagram adn store that pixel
    // // 2. Move to the right and store those pixels
    // // 3. After moving to the right and reaching the end of the padding, go up a row and back to the start on the left

    // // This is the real width of the bitmap pixel data array, needed for calculating offsets
    // uint16_t padded_bitmap_width = (uint16_t)(ceilf((float)bitmap_width / 2.0f) * 2.0f);

    // // Get space for texture in SRAM/FLASH and start storing API
    // 
    // engine_resource_start_storing(self->data, self->in_ram);

    // uint8_t temp_row_pixels_buffer[512];

    // // Fetch pixel data from filesystem row by row from end to start
    // for(int32_t y=bitmap_height-1; y>=0 ; y--){
    //     // Seek to start of bitmap pixel row at current y
    //     // and store how many bytes we'll need to fetch
    //     // for this row
    //     uint32_t offset = y * padded_bitmap_width + 0;
    //     engine_file_seek(0, bitmap_pixel_data_offset + (offset*2) + 0, MP_SEEK_SET);
    //     uint16_t remaining_width = bitmap_width * 2;

    //     while(remaining_width != 0){
    //         // Read up to 256 bytes of pixel data at a time
    //         uint16_t amount_to_read = MIN(512, remaining_width);

    //         uint16_t read_amount = engine_file_read(0, temp_row_pixels_buffer, amount_to_read);
    //         remaining_width -= read_amount;

    //         for(uint16_t i=0; i<read_amount/2; i++){
    //             engine_resource_store_u16(((uint16_t*)temp_row_pixels_buffer)[i]);
    //         }
    //     }
    // }

    // engine_resource_stop_storing();
    
    return MP_OBJ_FROM_PTR(self);
}


// Class methods
static mp_obj_t texture_resource_class_del(mp_obj_t self_in){
    ENGINE_INFO_PRINTF("TextureResource: Deleted (freeing texture data)");

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(texture_resource_class_del_obj, texture_resource_class_del);


uint16_t texture_resource_get_pixel(texture_resource_class_obj_t *texture, uint32_t offset){
    uint16_t *texture_data = ((mp_obj_array_t*)texture->data)->items;
    return texture_data[offset];
}


/*  --- doc ---
    NAME: TextureResource
    ID: TextureResource
    DESC: Object that holds information about bitmaps. The file needs to be a 16-bit RGB565 .bmp file
    PARAM:  [type=string]       [name=filepath]  [value=string]
    PARAM:  [type=boolean]      [name=in_ram]    [value=True of False (False by default)]
    ATTR:   [type=float]        [name=width]     [value=any (read-only)]
    ATTR:   [type=float]        [name=height]    [value=any (read-only)]
    ATTR:   [type=bytearray]    [name=data]      [value=RGB565 bytearray (note, if in_ram is False, then writing to this is not a valid operation)]
*/ 
static void texture_resource_class_attr(mp_obj_t self_in, qstr attribute, mp_obj_t *destination){
    ENGINE_INFO_PRINTF("Accessing TextureResource attr");

    texture_resource_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if(destination[0] == MP_OBJ_NULL){          // Load
        switch(attribute){
            case MP_QSTR___del__:
                destination[0] = MP_OBJ_FROM_PTR(&texture_resource_class_del_obj);
                destination[1] = self_in;
            break;
            case MP_QSTR_width:
                destination[0] = mp_obj_new_int(self->width);
            break;
            case MP_QSTR_height:
                destination[0] = mp_obj_new_int(self->height);
            break;
            case MP_QSTR_bit_depth:
                destination[0] = mp_obj_new_int(self->bit_depth);
            break;
            case MP_QSTR_format:
                destination[0] = mp_obj_new_int(self->format);
            break;
            case MP_QSTR_colors:
                destination[0] = self->colors;
            break;
            case MP_QSTR_data:
                destination[0] = self->data;
            break;
            default:
                return; // Fail
        }
    }else if(destination[1] != MP_OBJ_NULL){    // Store
        switch(attribute){
            case MP_QSTR_data:
            {
                mp_obj_array_t *new_bytearray = destination[1];
                mp_obj_array_t *cur_bytearray = self->data;
                if(cur_bytearray->len != new_bytearray->len){
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("TextureResource: ERROR: Can't set texture data to new bytearray, lengths do not match!"));
                }
                self->data = destination[1];
            }
            break;
            case MP_QSTR_colors:
            {
                mp_obj_array_t *new_bytearray = destination[1];
                mp_obj_array_t *cur_bytearray = self->colors;
                if(cur_bytearray->len != new_bytearray->len){
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("TextureResource: ERROR: Can't set texture colors to new bytearray, lengths do not match!"));
                }
                self->colors = destination[1];
            }
            break;
            case MP_QSTR_bit_depth:
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("TextureResource: ERROR: Bit depth of a texture cannot be set!"));
            break;
            case MP_QSTR_format:
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("TextureResource: ERROR: Format of a texture cannot be set!"));
            break;
            default:
                return; // Fail
        }

        // Success
        destination[0] = MP_OBJ_NULL;
    }
}


// Class attributes
static const mp_rom_map_elem_t texture_resource_class_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_1BPP_PAL), MP_ROM_INT(FMT_1BPP_PAL) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_4BPP_PAL), MP_ROM_INT(FMT_4BPP_PAL) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_8BPP_PAL), MP_ROM_INT(FMT_8BPP_PAL) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_16BPP_RGB565), MP_ROM_INT(FMT_16BPP_RGB565) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_16BPP_XRGB_555), MP_ROM_INT(FMT_16BPP_XRGB_1555) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_16BPP_ARGB_555), MP_ROM_INT(FMT_16BPP_ARGB_1555) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_24BPP_RGB888), MP_ROM_INT(FMT_24BPP_RGB888) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_32BPP_XRGB8888), MP_ROM_INT(FMT_32BPP_XRGB8888) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_32BPP_ARGB8888), MP_ROM_INT(FMT_32BPP_ARGB8888) },
};
static MP_DEFINE_CONST_DICT(texture_resource_class_locals_dict, texture_resource_class_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    texture_resource_class_type,
    MP_QSTR_TextureResource,
    MP_TYPE_FLAG_NONE,

    make_new, texture_resource_class_new,
    attr, texture_resource_class_attr,
    locals_dict, &texture_resource_class_locals_dict
);