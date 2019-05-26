// testanalyzer.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

typedef std::string string;
typedef std::vector<string> stringvec;

bool g_verbose = false;  // Verbose mode
bool g_all = false;  // Show all mode
stringvec g_testnames;
int g_maxchunkstoshow = 6;

HANDLE g_hConsole;
int g_testcount = 0;
int g_testskipped = 0;
int g_testsfailed = 0;

#define TEXTATTRIBUTES_TITLE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define TEXTATTRIBUTES_NORMAL (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define TEXTATTRIBUTES_WARNING (FOREGROUND_RED | FOREGROUND_GREEN)
#define TEXTATTRIBUTES_DIFF (FOREGROUND_RED | FOREGROUND_GREEN)


// Get list of sub-directories for a directory. Win32 specific method
// See http://www.martinbroadhurst.com/list-the-files-in-a-directory-in-c.html
void list_directory_subdirs(const string& dirname, stringvec& v)
{
    string pattern(dirname);
    pattern.append("\\*");
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && data.cFileName[0] != '.')
                v.push_back(data.cFileName);
        }
        while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
}

// Get first file by mask in the directory. Win32 specific method
string findfile_bymask(const string& dirname, const string& mask)
{
    string pattern(dirname);
    pattern.append("\\").append(mask);
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                FindClose(hFind);
                return data.cFileName;
            }
        }
        while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
    return "";
}


// Process my log, show NOT IMPLEMENTED lines, make sure it ends with SUCCESS
bool process_mylog(string& mylogfilepath)
{
    bool haserrors = false;
    string prefix = "  my.LOG: ";
    std::vector<string> problems;

    std::ifstream file(mylogfilepath);  //TODO: open for read only
    //TODO: check if open fails

    // Enumerate the lines and collect all problematic lines
    string str;
    bool hassuccess = false;
    while (std::getline(file, str))
    {
        if (str.find("*** NOT IMPLEMENTED") == 0)
        {
            problems.push_back(str);
            haserrors = true;
        }
        if (str.find("ERROR") == 0)
        {
            problems.push_back(str);
            haserrors = true;
        }
        if (str.find("SUCCESS") == 0)
        {
            hassuccess = true;
        }
    }
    // Sort and unique the list of problematic lines
    sort(problems.begin(), problems.end());
    problems.erase(unique(problems.begin(), problems.end()), problems.end());
    // Report the lines
    for (std::vector<string>::iterator it = problems.begin(); it != problems.end(); ++it)
    {
        std::cout << prefix << *it << std::endl;
    }
    if (!haserrors && !hassuccess)
        std::cout << prefix << "has no SUCCESS line" << std::endl;

    return !haserrors && hassuccess;
}

// Compare MAP files as text, ignore page header lines
bool process_map_files(string& filepathmap11, string& filepathmapmy)
{
    std::ifstream file1(filepathmap11);  //TODO: open for read only
    std::ifstream file2(filepathmapmy);  //TODO: open for read only
    //TODO: check if open fails

    // Enumerate the lines and count diffs
    string str1, str2;
    int count1 = 0, count2 = 0, diffcount = 0, line = 0;
    for (;;)
    {
        bool res1 = (bool)std::getline(file1, str1);
        if (res1 && str1.find("RT-11 LINK ") == 0)  // skip top header line
            res1 = (bool)std::getline(file1, str1);
        if (res1 && str1.find("\fRT-11 LINK ") == 0)  // skip next page three header lines
        {
            std::getline(file1, str1);
            std::getline(file1, str1);
            res1 = (bool)std::getline(file1, str1);
        }
        bool res2 = (bool)std::getline(file2, str2);
        if (res2 && str2.find("PCLINK11 ") == 0)  // skip top header line
            res2 = (bool)std::getline(file2, str2);
        if (!res1 && !res2)
            break;
        if (res1) count1++;
        if (res2) count2++;
        if (res1 && res2)
        {
            if (str1 != str2)
                diffcount++;
        }
        else
        {
            diffcount++;
        }
        line++;
    }

    if (diffcount > 0)
    {
        std::cout << "  MAP files has diffs in " << diffcount << " lines" << std::endl;
    }

    return (diffcount == 0);
}

bool countdiff_binary_files(string& filepath11, string& filepathmy, const string& filekind)
{
    bool hasdiffs = false;

    std::ifstream f1(filepath11, std::ifstream::binary | std::ifstream::ate);
    if (f1.fail())
        return false;
    std::ifstream f2(filepathmy, std::ifstream::binary | std::ifstream::ate);
    if (f2.fail())
        return false;
    std::streamoff size1 = f1.tellg();
    std::streamoff size2 = f2.tellg();
    if (size1 != size2)
    {
        hasdiffs = true;
    }
    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);

    // Size to compare
    std::streamoff size = (size1 < size2) ? size1 : size2;
    const std::streamoff blocksize = 512;
    // Compare file contents
    int diffcount = 0;
    std::streamoff remaining = size;
    char buffer1[blocksize], buffer2[blocksize];
    while (remaining > 0)
    {
        std::streamoff sizetoread = (remaining >= blocksize) ? blocksize : remaining - blocksize;

        f1.read(buffer1, sizetoread);
        f2.read(buffer2, sizetoread);
        //TODO: check for file read errors
        for (size_t offset = 0; offset < sizetoread; offset++)
        {
            if (buffer1[offset] != buffer2[offset])
                diffcount++;
        }

        remaining -= sizetoread;
    }

    if (size1 != size2 || diffcount > 0)
    {
        std::cout << "  " << filekind << " files has ";
        if (size1 != size2)
            std::cout << "diff in size: " << size1 << " / " << size2;
        if (size1 != size2 && diffcount > 0)
            std::cout << ", and ";
        if (diffcount > 0)
            std::cout << "diffs in " << diffcount << " bytes";
        std::cout << std::endl;
        hasdiffs = true;
    }

    return !hasdiffs;
}

void showdiff_binary_files(string& filepath11, string& filepathmy, const string& filekind, int maxchunkstoshow)
{
    std::ifstream f1(filepath11, std::ifstream::binary | std::ifstream::ate);
    if (f1.fail())
        return;
    std::ifstream f2(filepathmy, std::ifstream::binary | std::ifstream::ate);
    if (f2.fail())
        return;
    std::streamoff size1 = f1.tellg();
    std::streamoff size2 = f2.tellg();
    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);

    // Size to compare
    std::streamoff size = (size1 < size2) ? size1 : size2;
    const std::streamoff blocksize = 512;
    // Compare file contents
    int chunkshown = 0;
    std::streamoff remaining = size;
    char buffer1[blocksize], buffer2[blocksize];
    unsigned int baseaddress = 0;
    while (remaining > 0)
    {
        std::streamoff sizetoread = (remaining >= blocksize) ? blocksize : remaining;

        f1.read(buffer1, sizetoread);
        f2.read(buffer2, sizetoread);
        //TODO: check for file read errors

        size_t offset = 0;  // offset in buffer1 and buffer2
        int reminblock = (int)sizetoread;
        while (reminblock > 0)
        {
            int chunksize = (reminblock >= 16) ? 16 : reminblock;
            bool chunkhasdiffs = memcmp(buffer1 + offset, buffer2 + offset, chunksize) != 0;
            if (chunkhasdiffs)
            {
                char buf[16];
                sprintf(buf, "%04x", baseaddress + offset);
                SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_NORMAL);
                std::cout << "    11." + filekind + "  " + buf + " ";
                for (int i = 0; i < chunksize; i++)
                {
                    bool isdiff = (buffer1[offset + i] != buffer2[offset + i]);
                    SetConsoleTextAttribute(g_hConsole, isdiff ? TEXTATTRIBUTES_DIFF : TEXTATTRIBUTES_NORMAL);
                    sprintf(buf, " %02x", (unsigned char)buffer1[offset + i]);
                    std::cout << buf;
                }
                SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_NORMAL);
                std::cout << " ";
                for (int i = 0; i < chunksize; i++)
                {
                    bool isdiff = (buffer1[offset + i] != buffer2[offset + i]);
                    SetConsoleTextAttribute(g_hConsole, isdiff ? TEXTATTRIBUTES_DIFF : TEXTATTRIBUTES_NORMAL);
                    sprintf(buf, " %02x", (unsigned char)buffer2[offset + i]);
                    std::cout << buf;
                }
                SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_NORMAL);
                std::cout << "  my." << filekind << std::endl;

                chunkshown++;
                if (chunkshown >= maxchunkstoshow)
                    break;
            }
            reminblock -= chunksize;
            offset += chunksize;
        }

        remaining -= sizetoread;
        baseaddress += (unsigned int)sizetoread;
        if (chunkshown >= maxchunkstoshow)
            break;
    }
    SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_WARNING);
}

// Compare SAV files as binary
bool process_sav_files(string& filepathsav11, string& filepathsavmy)
{
    bool res = countdiff_binary_files(filepathsav11, filepathsavmy, "SAV");
    if (!res && g_verbose)
        showdiff_binary_files(filepathsav11, filepathsavmy, "SAV", g_maxchunkstoshow);
    return res;
}

// Compare STB files as binary
bool process_stb_files(string& filepathstb11, string& filepathstbmy)
{
    bool res = countdiff_binary_files(filepathstb11, filepathstbmy, "STB");
    if (!res && g_verbose)
        showdiff_binary_files(filepathstb11, filepathstbmy, "STB", g_maxchunkstoshow);
    return res;
}

void process_test(string& stestdirname)
{
    SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_NORMAL);
    std::cout << "Test " << stestdirname << std::endl;
    SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_WARNING);

    string stestdirpath = "tests\\" + stestdirname;
    //string filenamelog11 = findfile_bymask(stestdirpath, "*-11.log");
    string filenamelogmy = findfile_bymask(stestdirpath, "*-my.log");
    string filenamemap11 = findfile_bymask(stestdirpath, "*-11.map");
    string filenamemapmy = findfile_bymask(stestdirpath, "*-my.map");
    string filenamesav11 = findfile_bymask(stestdirpath, "*-11.sav");
    string filenamesavmy = findfile_bymask(stestdirpath, "*-my.sav");
    string filenamestb11 = findfile_bymask(stestdirpath, "*-11.stb");
    string filenamestbmy = findfile_bymask(stestdirpath, "*-my.stb");

    bool isfileabsent = false;
    bool resmylog = true, resmaps = true, ressavs = true, resstbs = true;

    //if (filenamelog11.empty())
    //{
    //    std::cout << "  File not found: *-11.log" << std::endl;
    //    isfileabsent = true;
    //}
    if (filenamelogmy.empty())
    {
        std::cout << "  File not found: *-my.log" << std::endl;
        isfileabsent = true;
    }
    else
    {
        resmylog = process_mylog(stestdirpath + "\\" + filenamelogmy);
    }

    if (filenamemap11.empty())
    {
        std::cout << "  File not found: *-11.MAP" << std::endl;
        isfileabsent = true;
    }
    if (filenamemapmy.empty())
    {
        std::cout << "  File not found: *-my.MAP" << std::endl;
        isfileabsent = true;
    }
    if (filenamesav11.empty())
    {
        std::cout << "  File not found: *-11.SAV" << std::endl;
        isfileabsent = true;
    }
    if (filenamesavmy.empty())
    {
        std::cout << "  File not found: *-my.SAV" << std::endl;
        isfileabsent = true;
    }
    if (filenamestb11.empty())
    {
        std::cout << "  File not found: *-11.STB" << std::endl;
        isfileabsent = true;
    }
    if (filenamestbmy.empty())
    {
        std::cout << "  File not found: *-my.STB" << std::endl;
        isfileabsent = true;
    }

    if (!filenamemap11.empty() && !filenamemapmy.empty())
    {
        resmaps = process_map_files(stestdirpath + "\\" + filenamemap11, stestdirpath + "\\" + filenamemapmy);
    }
    if (!filenamesav11.empty() && !filenamesavmy.empty())
    {
        ressavs = process_sav_files(stestdirpath + "\\" + filenamesav11, stestdirpath + "\\" + filenamesavmy);
    }
    if (!filenamestb11.empty() && !filenamestbmy.empty())
    {
        resstbs = process_stb_files(stestdirpath + "\\" + filenamestb11, stestdirpath + "\\" + filenamestbmy);
    }

    if (isfileabsent)
    {
        std::cout << "  Test skipped." << std::endl;
        g_testskipped++;
        return;
    }
    if (!resmylog || !resmaps || !ressavs || !resstbs)
        g_testsfailed++;
    else if (g_verbose)
    {
        SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_NORMAL);
        std::cout << "  PASSED" << std::endl;
    }
}

void parse_commandline(int argc, char *argv[])
{
    for (int argi = 1; argi < argc; argi++)
    {
        const char* arg = argv[argi];
        if (arg[0] == '\\' || arg[0] == '-')
        {
            string option = arg + 1;
            if (option == "v" || option == "verbose")
                g_verbose = true;
            else if (option == "a" || option == "all")
                g_all = true;
            else
            {
                std::cout << "Unknown option: " << option << std::endl;
            }
        }
        else
        {
            // Assuming that any non-option argument is a test to select
            g_testnames.push_back(arg);
        }
    }
}

int main(int argc, char *argv[])
{
    // Show title message
    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_TITLE);
    std::cout << "TestAnalyzer utility for PCLINK11 project" << std::endl;
    SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_NORMAL);

    // Parse command line
    parse_commandline(argc, argv);

    // Apply options
    if (g_all)
    {
        g_maxchunkstoshow = INT_MAX;
    }

    if (g_testnames.empty())  // Tests not selected by command-line argument, let's collect all the tests names
    {
        // Get list of sub-directories in tests directory
        list_directory_subdirs("tests", g_testnames);
    }

    // Analyze all the tests
    g_testcount = 0;
    g_testskipped = 0;
    for (stringvec::iterator it = g_testnames.begin(); it != g_testnames.end(); ++it)
    {
        g_testcount++;
        string stestdirname = *it;

        process_test(stestdirname);
    }

    // Show totals
    SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_TITLE);
    std::cout << "TOTAL tests: " << g_testcount;
    if (g_verbose)
    {
        int testspassed = g_testcount - g_testsfailed;
        std::cout << ", passed: " << testspassed << " (" << testspassed * 100.0 / g_testcount << "%)";
    }
    if (g_testsfailed > 0)
    {
        SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_WARNING);
        std::cout << ", failed: " << g_testsfailed << " (" << g_testsfailed * 100.0 / g_testcount << "%)";
    }
    if (g_testskipped > 0)
    {
        SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_WARNING);
        std::cout << ", skipped: " << g_testskipped;
    }
    std::cout << std::endl;
    SetConsoleTextAttribute(g_hConsole, TEXTATTRIBUTES_NORMAL);

    return 0;
}
