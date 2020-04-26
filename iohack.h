
#pragma once
#include <iostream>
#include <cstring>
#include <cstdio>

#if defined(UNICODE) || defined(_UNICODE)
    #define STDIOHACK_HDR_UNICODE
#endif

#if !defined(TEXT)
    #ifdef STDIOHACK_HDR_UNICODE
        #define TEXT(x) L##x
    #else
        #define TEXT(x) x
    #endif
#endif

namespace stdio
{
    auto& out =
        #ifdef STDIOHACK_HDR_UNICODE
            std::wcout
        #else
            std::cout
        #endif
    ;

    auto& err =
        #ifdef STDIOHACK_HDR_UNICODE
            std::wcerr
        #else
            std::cerr
        #endif
    ;

    /*
       int printf(const char *format, ...);
       int fprintf(FILE *stream, const char *format, ...);
       int dprintf(int fd, const char *format, ...);
       int sprintf(char *str, const char *format, ...);
       int snprintf(char *str, size_t size, const char *format, ...);

       int wprintf(const wchar_t *format, ...);
       int fwprintf(FILE *stream, const wchar_t *format, ...);
       int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
    */

    template<typename... ArgsT>
    int csprintf(char* buf, size_t bufsz, const char* fmt, ArgsT&&... args)
    {
        return std::snprintf(buf, bufsz, fmt, args...);
    }

    template<typename... ArgsT>
    int csprintf(wchar_t* buf, size_t bufsz, const wchar_t* fmt, ArgsT&&... args)
    {
        return std::swprintf(buf, bufsz, fmt, args...);
    }

    template<typename... ArgsT>
    int cfprintf(FILE* fh, const char* fmt, ArgsT&&... args)
    {
        return std::fprintf(fh, fmt, args...);
    }

    template<typename... ArgsT>
    int cfprintf(FILE* fh, const wchar_t* fmt, ArgsT&&... args)
    {
        return std::fwprintf(fh, fmt, args...);
    }


    /*
        int fputc(  
           int c,  
           FILE *stream   
        );  
        wint_t fputwc(  
           wchar_t c,  
           FILE *stream   
        );  
    */
    template<typename CharT>
    int cfputc(CharT ch, FILE* fh);

    template<>
    int cfputc<char>(char ch, FILE* fh)
    {
        return ::fputc(ch, fh);
    }

    template<>
    int cfputc<wchar_t>(wchar_t ch, FILE* fh)
    {
        return fputwc(ch, fh);
    }
}

namespace unistr
{
    /*
        int strcmp(  
           const char *string1,  
           const char *string2   
        );  
        int wcscmp(  
           const wchar_t *string1,  
           const wchar_t *string2   
        );  
        int _mbscmp(  
           const unsigned char *string1,  
           const unsigned char *string2   
        );  
    */

    template<typename CharT>
    int cstrcmp(const CharT* a, const CharT* b/* , bool icase */);

    template<>
    int cstrcmp<char>(const char* a, const char* b)
    {
        return ::strcmp(a, b);
    }

    template<>
    int cstrcmp<wchar_t>(const wchar_t* a, const wchar_t* b)
    {
        return ::wcscmp(a, b);
    }

    /*
        char *strncpy(  
           char *strDest,  
           const char *strSource,  
           size_t count   
        );  
        char *_strncpy_l(  
           char *strDest,  
           const char *strSource,  
           size_t count,  
           locale_t locale   
        );  
        wchar_t *wcsncpy(  
           wchar_t *strDest,  
           const wchar_t *strSource,  
           size_t count   
        );  
    */

    template<typename CharT>
    CharT* cstrncpy(CharT* dest, const CharT* src, size_t cnt);

    template<>
    char* cstrncpy<char>(char* dest, const char* src, size_t cnt)
    {
        return ::strncpy(dest, src, cnt);
    }

    template<>
    wchar_t* cstrncpy<wchar_t>(wchar_t* dest, const wchar_t* src, size_t cnt)
    {
        return ::wcsncpy(dest, src, cnt);
    }

    /*
        char *strcat(  
           char *strDestination,  
           const char *strSource   
        );  
        wchar_t *wcscat(  
           wchar_t *strDestination,  
           const wchar_t *strSource   
        );  
    */

    template<typename CharT>
    CharT* cstrcat(CharT* dest, const CharT* src);

    template<>
    char* cstrcat<char>(char* dest, const char* src)
    {
        return ::strcat(dest, src);
    }

    template<>
    wchar_t* cstrcat<wchar_t>(wchar_t* dest, const wchar_t* src)
    {
        return ::wcscat(dest, src);
    }

}
