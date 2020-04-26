
/*
* creates dangerous files. use with caution!
* i'm putting it in the public domain. feel free to use and abuse
* what you find here. however, it would be nice if you could
* reference me (apfeltee, https://github.com/apfeltee), though it's not necessary.
*/

#include <stdio.h>
#include <string.h>

#define STR(s) \
    { sizeof(s)-1, s }

struct dummy
{
    int len;
    const char str[128];
};


static struct dummy patterns[] =
{
    STR("this\ewill\ebreak\"windows\rexplorer .exe "),
    STR("  can't\raccess\" a file\elike\2this. txt"),
    STR("  *< this*might>\1break\rcygwin\n$$$.txt"),
    STR("..\r"),
    STR(".*"),
    STR("*.*"),
    STR("\"\""),
    STR("''"),
    STR("\355\261\243\355\261\271\355\261\247000b000000311fe00588cedf589ec86d"),
    STR("\r"),
    STR("\n"),
    STR("\t"),
    STR("\v"),
    STR("\f"),
    STR("              \uFFFFF--whodoesthis"),
    STR("nul.com"),
    STR("aux.com"),
    STR("com1.com"),
    STR("nul"),
    STR("aux"),
    {0, "\0"},
    
};

/* print a string potentially containing terminal-breaking characters to file $fo */
static void printdangerstr(FILE* fo, const char* str, int len)
{
    int i;
    int ch;
    for(i=0; i<len; i++)
    {
        ch = str[i];
        if((ch >= 32) && (ch <= 126))
        {
            switch(ch)
            {
                case '"':
                    fprintf(fo, "\\\"");
                    break;
                default:
                    fputc(ch, fo);
                    break;
            }
        }
        else
        {
            switch(ch)
            {
                case '\0':
                    fprintf(fo, "\\0");
                    break;
                case '\n':
                    fprintf(fo, "\\n");
                    break;
                case '\r':
                    fprintf(fo, "\\r");
                    break;
                case '\t':
                    fprintf(fo, "\\t");
                    break;
                case '\a':
                    fprintf(fo, "\\a");
                    break;
                default:
                    fprintf(fo, "\\x%2X", ch & 0xFF);
                    break;
            }
        }
    }
}

int main()
{
    int i;
    int len;
    char path[1024 + 1];
    const char* odir;
    const char* str;
    FILE* fh;
    odir = "dangerzone/";
    for(i=0; patterns[i].len != 0; i++)
    {
        memset(path, 0, 1024);
        len = patterns[i].len;
        str = patterns[i].str;
        strcat(path, odir);
        strncat(path, str, len);
        fprintf(stderr, "creating \"");
        printdangerstr(stderr, path, len+strlen(odir));
        fprintf(stderr, "\" ... ");
        if((fh = fopen(path, "wb")) != NULL)
        {
            fprintf(stderr, "ok\n");
            fprintf(fh, "you managed to read %.*s. good for you!\n", len, str);
            fclose(fh);
        }
        else
        {
            fprintf(stderr, "failed\n");
        }
    }
}
