#ifndef _APS_FNT_H_
#define _APS_FNT_H_

#define FNT_NAME_SIZE       256

typedef enum
{
    fntERR_LINE_FULL            =   1,
    fntERR_OK                   =   0,
    fntERR_READ_HEADER          =  -1,
    fntERR_ALLOC_INDEX_TABLE    =  -2,
    fntERR_READ_INDEX_TABLE     =  -3,
    fntERR_READ_FONT_BANK_SIZE  =  -4,
    fntERR_ALLOC_FONT_BANK      =  -5,
    fntERR_ALLOC_A_CHAR_BIPMAP  =  -6,
    fntERR_READ_A_CHAR_BIPMAP   =  -7,
    fntERR_PTR_NULL             =  -9,
    fntERR_FILE_OPEN            = -10,
    fntERR_CHAR_VALUE_NEGATIVE  = -11,
    fntERR_CHAR_VALUE_TO_HIGH   = -12,
    fntERR_CHAR_EMPTY           = -13,
    fntERR_CHAR_BIPMAP_PTR_NULL = -14,
}fntERROR;


typedef struct 
{
    int         error;
    int         version;
    char        name[FNT_NAME_SIZE+1];
    int         nbrcar;
    int         width;
    int         height;
    int         downstroke;
    int         compression;
    int         char_list_size;
}aps_fnt_details_t;


void*       aps_fnt_create(char *path);
fntERROR    aps_fnt_load(void* fnt, char *path);
void        aps_fnt_free(void *fnt);

int         aps_fnt_get_character_buffer_size(void* fnt);
fntERROR    aps_fnt_get_high(void *fnt);

fntERROR    aps_fnt_draw_char(void *fnt, uint8_t *graphic_buf, int* pix, int printer_width, int c);

fntERROR    aps_fnt_get_details(void *fnt, aps_fnt_details_t *p);

fntERROR    aps_fnt_error(void *fnt);
const char* aps_fnt_error_str (void *fnt);

#endif

