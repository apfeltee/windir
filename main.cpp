
#define _CRT_SECURE_NO_WARNINGS

#define UNICODE
#define _UNICODE


#include <iostream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <cmath>
#include <fcntl.h>

// windows headers
#include <windows.h>
#include <shlwapi.h>
#include <tchar.h> 
#include <io.h>
#include "iohack.h"


// thirdparty headers
#include "optionparser.hpp"

// libraries, etc
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")

struct Item;

using string_t = std::basic_string<TCHAR>;
using Callback = std::function<void(const Item&)>;


struct Options
{
    bool wantreverse  = false;
    bool wantdosname  = false;
    bool wantsorted   = false;
    bool wantfullpath = false;
    bool wantrelpath  = false;
    bool wantcrlf     = false;
    bool fileonly     = false;
    bool alsonormal   = false; 
};

struct Item
{
    string_t origname = TEXT("");
    string_t name = TEXT("");
    WIN32_FIND_DATA fdata = {};
    LARGE_INTEGER fsize = {};
    bool isdir = false;
    bool isshort = false;
};

static void disperror(const TCHAR* funcname) 
{ 
    // Retrieve the system error message for the last-error code
    LPVOID msgbuf;
    TCHAR* fstr;
    DWORD dw = GetLastError(); 
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&msgbuf,
        0, NULL
    );
    fstr = (TCHAR*)(&msgbuf);

    //stdio::err.clear();
    //stdio::out.clear();

    //stdio::cfprintf(stderr, TEXT("ERROR: in '%s': %s\n"), funcname, msgbuf);
    stdio::err << "ERROR in '" << funcname << "': " << fstr << std::endl;

    {

        char cbuf[1024 * 8];
        wcstombs(cbuf, fstr, wcslen(fstr)+1);
        throw std::runtime_error(cbuf);
    }
}

static size_t to_wchar_str(wchar_t* buf, const char* str, size_t len)
{
    /*
    * fun fact: there's no difference between those. although,
    * arguably, mbstowcs is slightly better, due to being standardized.
    */
    //return mbstowcs(buf, str, len);
    return MultiByteToWideChar(0, 0, str, len+1, buf, len+1);
}

template<typename CharT>
int strcmpnat(const std::basic_string<CharT>& a, const std::basic_string<CharT>&b);

template<> int strcmpnat<char>(const std::string& a, const std::string& b)
{
    const char* stra;
    const char* strb;
    wchar_t bufa[a.size() + 1];
    wchar_t bufb[b.size() + 1];
    memset(bufa, 0, a.size());
    memset(bufb, 0, b.size());
    stra = a.c_str();
    strb = b.c_str();
    to_wchar_str(bufa, stra, a.size());
    to_wchar_str(bufb, strb, b.size());
    return StrCmpLogicalW(bufa, bufb);
}

template<> int strcmpnat<wchar_t>(const std::wstring& a, const std::wstring& b)
{
    return StrCmpLogicalW(a.c_str(), b.c_str());
}

static string_t format_size(int64_t sz)
{
    enum { kBufSize = 50 };
    size_t bufsz;
    float fres;
    int64_t pres;
    int64_t exp;
    TCHAR obuf[kBufSize + 1];
    string_t result;
    static const char units[] = "BKMGTPE";
    if(sz == 0)
    {
        return TEXT("0B");
    }
    exp = int64_t(std::log(sz) / std::log(1024));
    if(exp > 6)
    {
        exp = 6;
    }
    pres = std::pow(1024, exp);
    fres = (float(sz) / pres);
    bufsz = swprintf(obuf, kBufSize, TEXT("%.1f%c"), fres, units[exp]);
    //bufsz = stdio::csprintf(obuf, kBufSize, TEXT("%.1f%c"), fres, units[exp]);
    result.append(obuf, bufsz);
    return result;
}

static bool windir(const string_t& dirpath, const Options& opts, Callback func)
{
    std::vector<Item> cache;
    WIN32_FIND_DATA ffd;
    LARGE_INTEGER filesize;
    TCHAR dirnamebuf[MAX_PATH + 1024];
    HANDLE fhnd;
    DWORD estatus;
    bool mustcache;
    mustcache = (opts.wantreverse || opts.wantsorted);
    estatus = 0;
    fhnd = INVALID_HANDLE_VALUE;
    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.
    if(dirpath.size() > (MAX_PATH - 3))
    {
        fprintf(stderr, "dir path too long\n");
        stdio::cfprintf(stderr, TEXT("ERROR: directory path too long\n"));
        return false;
    }
    //StringCchCopy(dirnamebuf, MAX_PATH, dirpath.c_str());
    unistr::cstrncpy(dirnamebuf, dirpath.c_str(), MAX_PATH);
    if(PathIsDirectory(dirnamebuf))
    {
        //StringCchCat(dirnamebuf, MAX_PATH, TEXT("\\*"));
        unistr::cstrcat(dirnamebuf, TEXT("\\*"));
        fhnd = FindFirstFile(dirnamebuf, &ffd);
        if (INVALID_HANDLE_VALUE == fhnd) 
        {
            fprintf(stderr, "findfirstfile: bad handle\n");
            disperror(TEXT("FindFirstFile"));
            return false;
        } 
        do
        {
            Item rtitem;
            rtitem.origname = ffd.cFileName;
            rtitem.fdata = ffd;
            rtitem.isdir = false;
            rtitem.isshort = false;
            rtitem.fsize.LowPart = 0;
            rtitem.fsize.HighPart = 0;
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                rtitem.isdir = true;
                //if((wcscmp(ffd.cFileName, TEXT(".")) == 0) || (wcscmp(ffd.cFileName, TEXT("..")) == 0))
                if((unistr::cstrcmp(ffd.cFileName, TEXT(".")) == 0) || (unistr::cstrcmp(ffd.cFileName, TEXT("..")) == 0))
                {
                    continue;
                }
            }
            else
            {
                filesize.LowPart = ffd.nFileSizeLow;
                filesize.HighPart = ffd.nFileSizeHigh;
                rtitem.fsize = filesize;
            }
            if(opts.wantdosname)
            {
                /*
                * if filename fits in 8.3 scheme as-is, then cAlternateFileName
                * will just be full of NULs - funnily, while rationally this
                * ought to eval to an equiv of NULL, C++ does not actually
                * permit comparing arrays to NULL. consistency my ass.
                */
                if(ffd.cAlternateFileName[0] == 0)
                {
                    rtitem.name = ffd.cFileName;
                }
                else
                {
                    rtitem.name = ffd.cAlternateFileName;
                    rtitem.isshort = true;
                }
            }
            else
            {
                rtitem.name = ffd.cFileName;
            }
            if(mustcache)
            {
                cache.push_back(rtitem);
            }
            else
            {
                func(rtitem);
            }
        } while (FindNextFile(fhnd, &ffd) != 0);
        estatus = GetLastError();
        if(estatus != ERROR_NO_MORE_FILES) 
        {
            fprintf(stderr, "findfirstfile: no more files\n");
            disperror(TEXT("FindFirstFile"));
            return false;
        }
        FindClose(fhnd);
        if(mustcache)
        {
            if(opts.wantsorted == true)
            {
                std::cerr << "sorting ... " << std::endl;
                std::sort(std::begin(cache), std::end(cache), [](Item& a, Item& b) -> bool
                {
                    return (strcmpnat(a.name, b.name) < 0);
                });
            }
            /*
            * reverse must ALWAYS be the last in the list of
            * operations - otherwise, it would just be a no-op!
            */
            if(opts.wantreverse)
            {
               std::reverse(std::begin(cache), std::end(cache));
            }
            for(auto& item: cache)
            {
                func(item);
            }
        }
        return true;
    }
    else
    {
        fprintf(stderr, "path is not a directory\n");
        stdio::cfprintf(stderr, TEXT("ERROR: path '%s' is not a directory\n"), dirnamebuf);
    }
    return false;
}

static string_t make_relpath(const Item& item, const string_t& dir)
{
    string_t relpath;
    relpath.append(dir);
    relpath.push_back('/');
    relpath.append(item.name);
    return relpath;
}

static string_t make_fullpath(const Item& item, const string_t& dir)
{
    int ch;
    size_t i;
    size_t rt;
    TCHAR fpathbuf[MAX_PATH + 1];
    string_t fpath;
    string_t relpath;
    relpath = make_relpath(item, dir);
    rt = GetFullPathName(relpath.c_str(), MAX_PATH, fpathbuf, NULL);
    if(rt == 0)
    {
        disperror(TEXT("make_fullpath"));
        return relpath;
    }
    /*
    * we could just do
    *   fpath.append(fpathbuf, rt);
    * but we also kinda want forward slashes, so ...
    * also, .reserve() is in fact needed, at least for GCC.
    * for some retarded fucking bullshit reason, libstdc++'s basic_string does
    * not expand memory when .push_back() is being used.
    * how does that happen? i mean, obv GNU has no QA, but this is ridiculous
    */
    fpath.reserve(rt);
    for(i=0; i<rt; i++)
    {
        ch = fpathbuf[i];
        if(ch == TEXT('\\'))
        {
            ch = TEXT('/');
        }
        fpath.push_back(ch);
    }
    return fpath;
}

template<typename StreamT>
static void print_filename(StreamT& os, const string_t& str)
{
    size_t ci;

    //os << "(len=" << str.size() << ") ";
    for(ci=0; ci<str.size(); ci++)
    {
        auto ch = str[ci];
        #if 1
        try
        {
        #endif
            os /*<< '[' << ci << ']'*/ << ch;
        #if 1
        }
        catch(std::runtime_error& e)
        {
            stdio::err << "exception in print_filename: " << e.what() << std::endl;
        }
        #endif
        os.clear();
    }
    //os << TEXT("fin");
    
}

static void format_out(const string_t& dir, const Item& item, const Options& opts)
{
    string_t opath;
    #if 0
    try
    {
    #endif
        opath = item.name;
        if(opts.wantfullpath || opts.wantrelpath)
        {
            if(opts.wantfullpath)
            {
                opath = make_fullpath(item, dir);
            }
            else if(opts.wantrelpath)
            {
                opath = make_relpath(item, dir);
            }
        }
        if(not opts.fileonly)
        {
            stdio::out << format_size(item.fsize.QuadPart) << '\t';
        }
        //stdio::out << opath;
        print_filename(stdio::out, opath);
        if(opts.alsonormal && item.isshort)
        {
            stdio::out << TEXT("\t=>\t") << item.origname;
        }
        if(opts.wantcrlf)
        {
            stdio::out << '\r';
        }
        stdio::out << '\n' << std::flush;
    #if 0
    }
    catch(std::runtime_error& e)
    {
        //stdio::out.clear();
        //stdio::err.clear();
        stdio::err << "exception in format_out: " << e.what() << std::endl;
    }
    #endif
}

int main(int argc, char* argv[])
{
    size_t i;
    int exitstatus;
    std::vector<string_t> args;
    string_t path;
    Options opts;
    OptionParser prs;
    exitstatus = 0;

    /* disable buffering across I/O */
    /*
    std::setvbuf(stdout, NULL, _IONBF, 0);
    std::setvbuf(stderr, NULL, _IONBF, 0);
    stdio::out.setf(std::ios::unitbuf);
    stdio::err.setf(std::ios::unitbuf);
    stdio::out.rdbuf()->pubsetbuf(0, 0);
    stdio::err.rdbuf()->pubsetbuf(0, 0);
    */
    /*
    stdio::out.setf((std::ios_base::fmtflags)0);
    stdio::out.exceptions(stdio::out.badbit);
    stdio::err.exceptions(stdio::err.badbit);
    */

    //#if defined(_MSVC) || defined(_WIN32)
    #if defined(_O_U8TEXT)
        //_setmode(_fileno(stdout), _O_BINARY);
        //_setmode(_fileno(stdout), _O_WTEXT);
        _setmode(_fileno(stdout), _O_U8TEXT);
    #endif
    prs.tail()
        << "\n"
        << "most options can be combined - for example:\n"
        << "  '-Ffx' will print the full path, with the 8.3 filename, but without filesize\n"
        << "  '-Fxsr' prints the full path, 8.3 filename, and sort in reverse\n"
        << "  '-CRx' prints 8.3 filename, relative to the directory provided, and emits CRLF\n"
    ;
    prs.on({"-r", "--reverse"}, "reverse output", [&]{
        opts.wantreverse = true;
    });
    prs.on({"-x", "--short"}, "output MS-DOS 8.3 filenames", [&]{
        opts.wantdosname = true;
    });
    prs.on({"-X", "-n", "--combined"}, "output MS-DOS 8.3 alongside normal filename", [&]{
        opts.wantdosname = true;
        opts.alsonormal = true;
    });
    prs.on({"-s", "--sort"}, "sort using StrCmpLogical", [&]{
        opts.wantsorted = true;
    });
    prs.on({"-f", "--fileonly"}, "output filename only", [&]{
        opts.fileonly = true;
    });
    prs.on({"-F", "--fullpath"}, "output absolute path", [&]{
        opts.wantfullpath = true;
    });
    prs.on({"-R", "--relpath"}, "output relative path", [&]{
        opts.wantrelpath = true;
    });
    prs.on({"-C", "--crlf"}, "(legacy support only) terminate lines with \\r\\n (CRLF) instead of only \\n (LF)", [&]{
        opts.wantcrlf = true;
    });
    
    try
    {
        
        try
        {
            prs.parse(argc, argv);
        }
        catch(std::runtime_error& e)
        {
            stdio::err << TEXT("commandline flags error: ") << e.what() << std::endl;
        }
        catch(...)
        {
            stdio::err << TEXT("some other commandline flags error occured...?") << std::endl;
        }
        for(i=0; i<prs.size(); i++)
        {
            auto s = prs.positional(i);
            args.push_back(string_t(s.begin(), s.end()));
        }
        if(args.size() == 0)
        {
            args.push_back(TEXT("."));
        }
        for(i=0; i<args.size(); i++)
        {
            path = args[i];
            try
            {
                exitstatus = (int)windir(path, opts, [&](const Item& item)
                {
                    format_out(path, item, opts);
                });
            }
            catch(std::exception& e)
            {
                stdio::err << "exception in windir(): " << e.what() << std::endl;
            }
            
        }
    }
    catch(std::runtime_error& e)
    {
        fprintf(stderr, "error: %s\n", e.what());
        stdio::err << TEXT("error: ") << e.what() << std::endl;
        return 1;
    }
    return exitstatus;
    
}
