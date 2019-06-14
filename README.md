# pclink11
Attempt to port PDP-11 LINKer to PC C/C++ — learning how the linker works, a hard way.

[![Build status](https://ci.appveyor.com/api/projects/status/3lt4c9rxx2bv0g0g?svg=true)](https://ci.appveyor.com/project/nzeemin/pclink11)

The code (partially) ported from MACRO-11 sources to C/C++.
Port source: RT-11 LINK V05.45.

## Status: work in progress 🚧

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

Input files and options are space-separated.

Examples:
 - `pclink11 HELLO.OBJ` — link the object file, will produce `HELLO.SAV` executable
 - `pclink11 /MAP /SYMBOLTABLE /EXECUTE:LD.SYS LD.OBJ SYSLIB.OBJ /X` — link object file with system library, produce map file and symbol file, save output as `LD.SYS`, do not put bitmap in the first block
 - `pclink11 TEST1.OBJ TEST2.OBJ /MAP /WIDE /A` — link two object files, generate map file with wide format, alphabetize list of symbols

## TODO 👷

First priority:
 - Fix bugs for the failing test cases
 - Option to NOT produce MAP file, currently we always make the MAP file
 - Option to NOT generate STB file, currently we always do

Second priority:
 - More tests, currently we have like 70 test cases
 - Compile under Linux/MacOS, configure CI to compile under Linux/MacOS on every commit

Not implemented now, and not sure we will:
 - LDA output - produce a file in absolute binary loader format
 - Overlays
 - Linkage with Separated instructions/data spaces

## Links 🔗
 - [DEC-11-ZLDA-D PDP-11 LINK-11 Linker and LIBR-11 Librarian May71](https://archive.org/details/bitsavers_decpdp11do11LINK11LinkerandLIBR11LibrarianMay71_1259623)
