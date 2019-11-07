# pclink11
Attempt to port PDP-11 LINKer to PC C/C++ — learning how the linker works, a hard way.

[![Build status](https://ci.appveyor.com/api/projects/status/3lt4c9rxx2bv0g0g?svg=true)](https://ci.appveyor.com/project/nzeemin/pclink11)
[![Build Status](https://travis-ci.org/nzeemin/pclink11.svg?branch=master)](https://travis-ci.org/nzeemin/pclink11)
[![CodeFactor](https://www.codefactor.io/repository/github/nzeemin/pclink11/badge)](https://www.codefactor.io/repository/github/nzeemin/pclink11)

The code (partially) ported from MACRO-11 sources to C/C++.
Port source: RT-11 LINK V05.45.

## Status: Work in Progress 🚧

Currently the PCLINK11 links most of test OBJ files properly, but we have some troubles linking tests with libraries.

## Usage ✨
Command line:

`pclink11 <input files and options>`

Options:
 - `/EXECUTE:filespec` — Specifies the name of the memory image file
 - `/NOBITMAP` `/X` — Do not emit bit map
 - `/WIDE` `/W` — Produces a load map that is 132-columns wide
 - `/ALPHABETIZE` `/A` — Lists global symbols on the link map in alphabetical order
 - `/SYMBOLTABLE` `/STB` — Generates a symbol table file (.STB file)
 - `/MAP` — Generates map file

Input files and options are space-separated.

Examples:
 - `pclink11 HELLO.OBJ` — link the object file, will produce `HELLO.SAV` executable
 - `pclink11 /MAP /SYMBOLTABLE /EXECUTE:LD.SYS LD.OBJ SYSLIB.OBJ /X` — link object file with system library, produce map file and symbol file, save output as `LD.SYS`, do not put bitmap in the first block
 - `pclink11 TEST1.OBJ TEST2.OBJ /MAP /WIDE /A` — link two object files, generate map file with wide format, alphabetize list of symbols

## Testing Strategy
Folder `tests` contains several dozens sub-folders with .OBJ files.

First, we use [RT-11 simulator](http://emulator.pdp-11.org.ru/RT-11/distr/) written by Dmitry Patronov to produce "etalon"/"original" output files, they renamed with `-11` suffix — see `!runtest11.cmd` command file.
Then, we run the `pclink11` with to produce "our" output files, they renamed with `-my` suffix — see `!runtestmy.cmd` command file and `testrunner` utility.
And finally, we compare "original" files with "our" files, line-to-line or byte-to-byte, using `testanalyzer` utility.

"Our" MAP files differs in the first line (program name, date/time, no page number), and there's no paging, so no page header lines.
SAV/SYS and STB files are binary, compared byte-to-byte, should be no differences.
Log files are absolutely different, we're not comparing them.

## TODO 👷

First priority:
 - Fix bugs for the failing test cases
 
Second priority:
 - More tests, currently we have 94 test cases
 - Compile under Linux/MacOS, configure CI to compile under Linux/MacOS on every commit
 - Reduce amount of logging, add option for verbosity level

Not implemented now, and not sure we will:
 - Link for foreground execution
 - LDA output - produce a file in absolute binary loader format
 - Overlays
 - Linkage with separated instructions/data spaces

## Links 🔗
 - [DEC-11-ZLDA-D PDP-11 LINK-11 Linker and LIBR-11 Librarian May71](https://archive.org/details/bitsavers_decpdp11do11LINK11LinkerandLIBR11LibrarianMay71_1259623)
 - [macro-11 - cross-assembler by Richard Krehbiel](https://github.com/simh/simtools/tree/master/crossassemblers/macro11)
