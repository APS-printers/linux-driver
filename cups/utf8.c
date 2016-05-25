
#ifdef UTF8_DEBUG
#   include <stdio.h>
#endif

#include <unistd.h>
#include "utf8.h"

#define BUFSIZE 4096    /*bytes*/


unsigned char buf[BUFSIZE];
unsigned char *p;
int size;
int fd;
int utf8;




static unsigned char get_char(void)
{

    if (size == 0)
    {
        if (fd < 0)
            return (unsigned char)-1;

        if ((size = read(fd,buf,sizeof(buf))) > 0 )
        { 
            p = buf;
#ifdef UTF8_DEBUG
            fprintf(stderr,"DEBUG: size:%d %s \n",size,buf);
#endif
        }
        else
        {
            fd = -1;
            return (unsigned char)-1;
        }
    }

    size--;
    return *p++;
}

void utf8_set_file(int _fd,int _utf8)
{
    fd = _fd;
    size = 0;
    utf8 = _utf8;
}

int utf8_get_code(void)
{

    int i;

    i = get_char();

    if (utf8) {
        if ((i & 0x80) == 0x80)
        {
            if ((i & 0xC0) == 0xC0)
            {
                i &= 0x1F;
                i <<= 6;
                i += (get_char() & 0x3F);
            }
            else if ((i & 0xE0) == 0xE0)
            {
                i &= 0x0F;
                i <<= 6;
                i += (get_char() & 0x3F);
                i <<= 6;
                i += (get_char() & 0x3F);
            }
            else if ((i & 0xF0) == 0xF0)
            {
                i &= 0x07;
                i <<= 6;
                i += (get_char() & 0x3F);
                i <<= 6;
                i += (get_char() & 0x3F);
                i <<= 6;
                i += (get_char() & 0x3F);
            }
        }
    }

#ifdef UTF8_DEBUG
    fprintf(stderr,"DEBUG: i:%i \n",i);
#endif

    if (fd < 0)
        return -1;

    return i;
}








