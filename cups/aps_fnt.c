
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include "aps_fnt.h"

#define FILE_VERSION        1
#define FILE_HEADER         "_APS_FONT_TOOL_"

#define EMPTY_CHAR          ((conv_idx_t)-1)

#define MALLOC_SIZE         4096

#define MALLOC(ptr,nbr)     (((ptr) = malloc((nbr) * sizeof(*(ptr)))) != NULL)

#define FREAD(ptr,nbr,file) ((fread((ptr),sizeof(*(ptr)),(nbr),(file))) == (size_t)nbr)

#define CLEAR(ptr,nbr)      memset(ptr, 0, nbr * sizeof(*(ptr)))

#define SET_ERR(x)          (P(fnt)->error = (x))

//FNT PIXELS
#define PIX_WHITE       0
#define PIX_BLACK       1
#define PIX_CUTTED      2
#define PIX_DARK_CUTTED 3

typedef unsigned int  conv_idx_t;
typedef uint8_t* char_data_t;

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
    conv_idx_t  *conv_tab;
    int         char_list_size;
    char_data_t *char_list;

}aps_fnt_t;

/*
 * struct of header to easy decode the file header
 */
#pragma pack(4)
typedef struct 
{
    char    header[sizeof(FILE_HEADER)];
    int32_t version;
    char    name[FNT_NAME_SIZE+1];
    int32_t nbrcar;
    int32_t width;
    int32_t height;
    int32_t downstroke;
    int32_t compression;
} aps_fnt_file_header;
#pragma pack()

/* PRIVATE FUNCTIONS --------------------------------------------------------*/
#define P(x)    ((aps_fnt_t*)(x))

/*
 * -----------------------------------------------------------------------------
 * Name      :  load_file
 * Purpose   :  load the content of aps font file
 *              
 * Inputs    :  fnt      : pointer to the font class to destroy
 *              f        : FILE* opened aps font file
 * Outputs   :  <>
 * Return    :  <0 on error, 0 otherwise
 * -----------------------------------------------------------------------------
 */
static int load_file(void *fnt,FILE *f)
{
    int32_t i32;

    aps_fnt_file_header fh;


    if (!FREAD(&fh,1,f))
        return SET_ERR(fntERR_READ_HEADER);
    
/* 
 * ====================
 * Get Header 
 * ====================
 */
    memcpy(P(fnt)->name,fh.name,sizeof(P(fnt)->name));
    /* To be sure */
    P(fnt)->name[FNT_NAME_SIZE]=0;

#if __BYTE_ORDER == __BIG_ENDIAN
    P(fnt)->version     = bswap_32(fh.version);
    P(fnt)->nbrcar      = bswap_32(fh.nbrcar);
    P(fnt)->width       = bswap_32(fh.width);
    P(fnt)->height      = bswap_32(fh.height);
    P(fnt)->downstroke  = bswap_32(fh.downstroke);
    P(fnt)->compression = bswap_32(fh.compression);
#else
    P(fnt)->version     = fh.version;
    P(fnt)->nbrcar      = fh.nbrcar;
    P(fnt)->width       = fh.width;
    P(fnt)->height      = fh.height;
    P(fnt)->downstroke  = fh.downstroke;
    P(fnt)->compression = fh.compression;
#endif


/* 
 * ====================
 * Get Convertion table
 * ====================
 */

    if (!MALLOC(P(fnt)->conv_tab,P(fnt)->nbrcar))
        return SET_ERR(fntERR_ALLOC_INDEX_TABLE);

    if (!FREAD(P(fnt)->conv_tab,P(fnt)->nbrcar,f))
        return SET_ERR(fntERR_READ_INDEX_TABLE);

#if __BYTE_ORDER == __BIG_ENDIAN
    {
        int i;
        conv_idx_t *p;
        conv_idx_t tmp;

        p = P(fnt)->conv_tab;
        i = P(fnt)->nbrcar;

        while(i--)
        {
            tmp = bswap_32(*p);
            *p = tmp;
            p++;
        }
    }
#endif

/* 
 * ====================
 * Get Character table
 * ====================
 */

    if (!FREAD(&i32, 1, f))
        return SET_ERR(fntERR_READ_FONT_BANK_SIZE);

#if __BYTE_ORDER == __BIG_ENDIAN
    P(fnt)->char_list_size = bswap_32(i32);
#else
    P(fnt)->char_list_size = i32;
#endif

    if (!MALLOC(P(fnt)->char_list, P(fnt)->char_list_size))
        return SET_ERR(fntERR_ALLOC_FONT_BANK);

    CLEAR(P(fnt)->char_list, P(fnt)->char_list_size);

    {
        int i           = P(fnt)->char_list_size;
        char_data_t *p  = P(fnt)->char_list;
        int char_buf_size;

        char_buf_size = aps_fnt_get_character_buffer_size(fnt);

        while(i--)
        {

            if (!MALLOC(*p,char_buf_size))
                return SET_ERR(fntERR_ALLOC_A_CHAR_BIPMAP);

            if (!FREAD(*p, aps_fnt_get_character_buffer_size(fnt), f))
                return SET_ERR(fntERR_READ_A_CHAR_BIPMAP);

            p++;
        }
    }
    return SET_ERR(fntERR_OK);
}


/* PUBLIC FUNCTIONS --------------------------------------------------------*/

/*
 * -----------------------------------------------------------------------------
 * Name      :  aps_fnt_get_details
 * Purpose   :  return all details of fonts
 *              
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  fntERROR (don't erase the current font error)
 * -----------------------------------------------------------------------------
 */
fntERROR    aps_fnt_get_details(void *fnt, aps_fnt_details_t *p)
{
    if (p != NULL)
        memset(p,0,sizeof(*p));

    if (fnt == NULL)
        return fntERR_PTR_NULL;

    p->error            = P(fnt)->error;
    p->version          = P(fnt)->version; 
    p->nbrcar           = P(fnt)->nbrcar; 
    p->width            = P(fnt)->width; 
    p->height           = P(fnt)->height; 
    p->downstroke       = P(fnt)->downstroke; 
    p->compression      = P(fnt)->compression; 
    p->char_list_size   = P(fnt)->char_list_size; 

    p->name[FNT_NAME_SIZE]=0;
    
    return fntERR_OK;
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  aps_fnt_error_str
 * Purpose   :  return string error of the module
 *              
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  NULL on malloc error, otherwise error can be get by aps_fnt_error()
 *                                    error can be occured only with path != NULL
 * -----------------------------------------------------------------------------
 */
const char* aps_fnt_error_str (void *fnt)
{
    switch(P(fnt)->error)
    {
        case fntERR_LINE_FULL:
            return "line text line is full, line will be wrapped.";
        case fntERR_OK:
            return "OK";
        case fntERR_READ_HEADER:
            return "Can't read in file the aps font header file";
        case fntERR_ALLOC_INDEX_TABLE:
            return "Can't allocate memorie to store index converstion table";
        case fntERR_READ_INDEX_TABLE:
            return "Can't read in file memorie to store index converstion table";
        case fntERR_READ_FONT_BANK_SIZE:
            return "Can't read in file the number of bipmap stored in font file";
        case fntERR_ALLOC_FONT_BANK:
            return "Can't allocate the array of pointer to the character bipmap";
        case fntERR_ALLOC_A_CHAR_BIPMAP:
            return "Can't allocate the memorie for a character in font";
        case fntERR_READ_A_CHAR_BIPMAP:
            return "Can't read in file the bipmap of a character in font";
        case fntERR_PTR_NULL:
            return "Class font pointer is NULL!?!?";
        case fntERR_FILE_OPEN:
            return "Can't open the font file";
        case fntERR_CHAR_VALUE_NEGATIVE:
            return "this character have a code value below zero";
        case fntERR_CHAR_VALUE_TO_HIGH:
            return "this character have a code to high for this font";
        case fntERR_CHAR_EMPTY:
            return "this character is empty in this font";
        case fntERR_CHAR_BIPMAP_PTR_NULL:
            return "there is no bipmap for this character ?!?!";
        default:
            return "Error unknown";
    }
}
/*
 * -----------------------------------------------------------------------------
 * Name      :  aps_fnt_error
 * Purpose   :  return error code of the module
 *              
 * Inputs    :  <>
 * Outputs   :  <>
 * Return    :  NULL on malloc error, otherwise error can be get by aps_fnt_error()
 *                                    error can be occured only with path != NULL
 * -----------------------------------------------------------------------------
 */
int aps_fnt_error(void *fnt)
{
    if (fnt == NULL)
        return fntERR_PTR_NULL;
    return P(fnt)->error;
}
/*
 * -----------------------------------------------------------------------------
 * Name      :  aps_fnt_create
 * Purpose   :  Create a font class and return the pointer of it class
 *              
 * Inputs    :  path      : path of the aps font files, can be null if the 
 *                          font file is loaded later by aps_fnt_load function
 * Outputs   :  <>
 * Return    :  NULL on any error, otherwise the pointer to created class
 * -----------------------------------------------------------------------------
 */
void* aps_fnt_create(char *path)
{
    aps_fnt_t *fnt;

    if (!MALLOC(fnt,1))
        return NULL;

    memset(fnt,0,sizeof(*fnt));

    if (path!=NULL)
    {
        aps_fnt_load(fnt,path);
    }
    return fnt;
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  aps_fnt_free
 * Purpose   :  free all memory used by font class 
 *              
 * Inputs    :  fnt      : pointer to the font class to destroy
 * Outputs   :  <>
 * Return    :  <>
 * -----------------------------------------------------------------------------
 */
void aps_fnt_free(void *fnt)
{
    if (fnt == NULL)
        return; 

    if (P(fnt)->conv_tab != NULL)
        free(P(fnt)->conv_tab);

    if (P(fnt)->char_list != NULL)
    {
        char_data_t *p = P(fnt)->char_list;

        while (P(fnt)->char_list_size--)
        {
            if (*p != NULL)
                free(*p);
            p++;
        }
        free(P(fnt)->char_list);
    }
}

/*
 * -----------------------------------------------------------------------------
 * Name      :  aps_fnt_get_character_buffer_size
 * Purpose   :  return the size in byte needed to code one character of this font
 *              
 * Inputs    :  fnt      : pointer to the font class to destroy
 * Outputs   :  <>
 * Return    :  size in byte used to code a character bitmap
 * -----------------------------------------------------------------------------
 */
int aps_fnt_get_character_buffer_size(void* fnt)
{
    if (fnt == NULL)
        return SET_ERR(fntERR_PTR_NULL); 

    return P(fnt)->width * P(fnt)->height;
}


/*
 * -----------------------------------------------------------------------------
 * Name      :  aps_fnt_load
 * Purpose   :  unload previous font if one is loaded and load the new font file
 *              
 * Inputs    :  fnt      : pointer to the font class to destroy
 *              path     : path of the aps font files 
 * Outputs   :  <>
 * Return    :  size in byte used to code a character bitmap
 * -----------------------------------------------------------------------------
 */
int aps_fnt_load(void* fnt, char *path)
{
    FILE *f = fopen(path,"rb");
    int error;

    if (fnt == NULL)
        return SET_ERR(fntERR_PTR_NULL); 

    if (f == NULL)
        return SET_ERR(fntERR_FILE_OPEN);

    aps_fnt_free(fnt);

    error = load_file(fnt,f);

    fclose(f);
    
    return error;
}


int aps_fnt_get_high(void *fnt)
{
    if (fnt == NULL)
        return SET_ERR(fntERR_PTR_NULL); 

    return P(fnt)->height;
}

int aps_fnt_draw_char(void *fnt, uint8_t *graphic_buf, int* pix, int printer_width, int c)
{
    if (fnt == NULL)
        return SET_ERR(fntERR_PTR_NULL); 

    if ((P(fnt)->width + *pix) >= (printer_width * 8))
        return SET_ERR(fntERR_LINE_FULL);

    if (c < 0) 
        return SET_ERR(fntERR_CHAR_VALUE_NEGATIVE);

    if (c >= P(fnt)->nbrcar)
        return SET_ERR(fntERR_CHAR_VALUE_TO_HIGH);

    conv_idx_t char_idx = P(fnt)->conv_tab[c];
    if (char_idx == EMPTY_CHAR)
        return SET_ERR(fntERR_CHAR_EMPTY);

    char_data_t p_char_bmp = P(fnt)->char_list[char_idx];
    if (p_char_bmp == NULL)
       return SET_ERR(fntERR_CHAR_BIPMAP_PTR_NULL); 

    {
        int dotline;
        int pixel;
        int start_mask;

        dotline = P(fnt)->height;

        graphic_buf += (int)(*pix/8);
        start_mask   = 0x80 >> (*pix%8);

        while (dotline--)
        {
            uint8_t *p_out;
            uint8_t mask;

            p_out = graphic_buf;
            mask  = start_mask;
            pixel = P(fnt)->width;

            while (pixel--)
            {
                if (mask == 0)
                {
                    mask = 0x80;
                    p_out++;
                }

                if (*p_char_bmp++ != PIX_WHITE)
                {
                    *p_out |= mask;
                }

                mask >>=1;
            }

            graphic_buf += printer_width;

        }
    }

    *pix += P(fnt)->width;

    return SET_ERR(fntERR_OK);
}

