
# At a glance

Windir lists files and directories, even those that appear unusable in most circumstances.

Born out of frustration with files/directories containing illegal characters, that consistently broke  
Windows explorer, windir includes the functionality of cmd.exe's `dir /x`, plus some other useful bits.

# Synopsis

Windir should be fairly self-explanatory, especially its help message:

```

usage: [-h] [-r] [-x] [-X -n] [-s] [-f] [-F] [-R] [-C] <args ...>

available options:
  -h --help:                       show this help
  -r --reverse:                    reverse output
  -x --short:                      output MS-DOS 8.3 filenames
  -X -n --combined:                output MS-DOS 8.3 alongside normal filename
  -s --sort:                       sort using StrCmpLogical
  -f --fileonly:                   output filename only
  -F --fullpath:                   output absolute path
  -R --relpath:                    output relative path
  -C --crlf:                       (legacy support only) terminate lines with \r\n (CRLF) instead of only \n (LF)

most options can be combined - for example:
  '-Ffx' will print the full path, with the 8.3 filename, but without filesize
  '-Fxsr' prints the full path, 8.3 filename, and sort in reverse
  '-CRx' prints 8.3 filename, relative to the directory provided, and emits CRLF

```

The most important option for dealing with problematic file/directory names is `-x`, which
prints the 8.3 format name of a file.  

For example, given the filename `reallylongfilename.ohmygod.mcc`, it would print `REALLY~1.MCC`.  
Of course windir is written so that returned arguments can be safely piped to say, `xargs`.

# License

Licensed under the terms of the MIT License.

