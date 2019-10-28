// testanalyzer.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS

#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#endif

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

int g_testcount = 0;
int g_testskipped = 0;
int g_testsfailed = 0;

#ifdef _MSC_VER
HANDLE g_hConsole;
#define TEXTATTRIBUTES_TITLE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define TEXTATTRIBUTES_NORMAL (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define TEXTATTRIBUTES_WARNING (FOREGROUND_RED | FOREGROUND_GREEN)
#define TEXTATTRIBUTES_DIFF (FOREGROUND_RED | FOREGROUND_GREEN)
#define SetTextAttribute(ta) SetConsoleTextAttribute(g_hConsole, ta)
#else
#define SetTextAttribute(ta) {}
#endif

#ifdef _MSC_VER
const char * TESTS_SUB_DIR = "tests\\";
const char * PATH_SEPARATOR = "\\";
#else
const char * TESTS_SUB_DIR = "tests/";
const char * PATH_SEPARATOR = "/";
#endif

#ifdef _MSC_VER
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
#else
// Get list of sub-directories for a directory. POSIX method
// See http://www.martinbroadhurst.com/list-the-files-in-a-directory-in-c.html
void list_directory_subdirs(const string& dirname, stringvec& v)
{
    DIR* dirp = opendir(dirname.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL)
    {
        if ((dp->d_type & DT_DIR) == 0 || dp->d_name[0] == '.')
            continue;

        v.push_back(dp->d_name);
    }
    closedir(dirp);
}
#endif

#ifdef _MSC_VER
// Get first file by mask in the directory. Win32 specific method
string findfile_bymask(const string& dirname, const string& mask)
{
    string pattern(dirname);
    pattern.append("\\*").append(mask);
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
#else
// Get first file by mask in the directory. POSIX method
string findfile_bymask(const string& dirname, const string& mask)
{
    DIR* dirp = opendir(dirname.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL)
    {
        if (dp->d_type & DT_DIR)
            continue;

        string filename(dp->d_name);
        if (filename.size() < mask.size() ||
            0 != filename.compare(filename.size() - mask.size(), mask.size(), mask))
            continue;

        closedir(dirp);
        return filename;
    }
    closedir(dirp);
    return "";
}
#endif


// Process my log, show NOT IMPLEMENTED lines, make sure it ends with SUCCESS
bool process_mylog(string mylogfilepath, stringvec& problems)
{
    std::ifstream file(mylogfilepath, std::ifstream::in);
    if (file.fail())
        return false;

    // Enumerate the lines and collect all problematic lines
    string str;
    bool haserrors = false;
    int warningcount = 0;
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
        if (str.find("WARNING") == 0)
        {
            if (g_verbose)
                problems.push_back(str);
            warningcount++;
        }
        if (str.find("SUCCESS") == 0)
        {
            hassuccess = true;
        }
    }

    // Sort and unique the list of problematic lines
    sort(problems.begin(), problems.end());
    problems.erase(unique(problems.begin(), problems.end()), problems.end());

    if (!g_verbose && warningcount > 0)
        problems.push_back("Has " + std::to_string(warningcount) + " WARNINGs.");
    if (!haserrors && !hassuccess)
        problems.push_back("Has no SUCCESS line.");

    return !haserrors && hassuccess;
}

// Compare MAP files as text, ignore page header lines
bool process_map_files(string filepathmap11, string filepathmapmy, stringvec& problems)
{
    std::ifstream file1(filepathmap11, std::ifstream::in);
    if (file1.fail())
        return false;
    std::ifstream file2(filepathmapmy, std::ifstream::in);
    if (file2.fail())
        return false;

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
        problems.push_back("MAP files has diffs in " + std::to_string(diffcount) + " lines");
    }

    return (diffcount == 0);
}

bool countdiff_binary_files(string& filepath11, string& filepathmy, const string& filekind, stringvec& problems)
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
        string message = filekind + " files has ";
        if (size1 != size2)
            message += "diff in size: " + std::to_string(size1) + " / " + std::to_string(size2);
        if (size1 != size2 && diffcount > 0)
            message += ", and ";
        if (diffcount > 0)
            message += "diffs in " + std::to_string(diffcount) + " bytes";
        problems.push_back(message);
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
                sprintf(buf, "%04x", (unsigned int)(baseaddress + offset));
                SetTextAttribute(TEXTATTRIBUTES_NORMAL);
                std::cout << "    11." + filekind + "  " + buf + " ";
                for (int i = 0; i < chunksize; i++)
                {
                    bool isdiff = (buffer1[offset + i] != buffer2[offset + i]);
                    SetTextAttribute(isdiff ? TEXTATTRIBUTES_DIFF : TEXTATTRIBUTES_NORMAL);
                    sprintf(buf, " %02x", (unsigned char)buffer1[offset + i]);
                    std::cout << buf;
                }
                SetTextAttribute(TEXTATTRIBUTES_NORMAL);
                std::cout << " ";
                for (int i = 0; i < chunksize; i++)
                {
                    bool isdiff = (buffer1[offset + i] != buffer2[offset + i]);
                    SetTextAttribute(isdiff ? TEXTATTRIBUTES_DIFF : TEXTATTRIBUTES_NORMAL);
                    sprintf(buf, " %02x", (unsigned char)buffer2[offset + i]);
                    std::cout << buf;
                }
                SetTextAttribute(TEXTATTRIBUTES_NORMAL);
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
    SetTextAttribute(TEXTATTRIBUTES_WARNING);
}

void process_test(string& stestdirname)
{
    string stestdirpath = TESTS_SUB_DIR + stestdirname;
    string filenamelogmy = findfile_bymask(stestdirpath, "-my.log");
    string filenamemap11 = findfile_bymask(stestdirpath, "-11.MAP");
    string filenamemapmy = findfile_bymask(stestdirpath, "-my.MAP");
    string filenamesav11 = findfile_bymask(stestdirpath, "-11.SAV");
    string filenamesavmy = findfile_bymask(stestdirpath, "-my.SAV");
    string filenamestb11 = findfile_bymask(stestdirpath, "-11.STB");
    string filenamestbmy = findfile_bymask(stestdirpath, "-my.STB");

    stringvec filesnotfound;
    stringvec logproblems;
    stringvec fileproblems;
    bool resmylog = true, resmaps = true, ressavs = true, resstbs = true;
    bool nomap = stestdirname.size() > 6 && stestdirname.compare(stestdirname.size() - 6, 6, "-NOMAP") == 0;
    bool nostb = stestdirname.size() > 6 && stestdirname.compare(stestdirname.size() - 6, 6, "-NOSTB") == 0;

    // Start analyzing the files, but no console output before the test name
    if (filenamelogmy.empty())
        filesnotfound.push_back("*-my.log");
    else
        resmylog = process_mylog(stestdirpath + PATH_SEPARATOR + filenamelogmy, logproblems);

    if (!nomap && filenamemap11.empty())
        filesnotfound.push_back("*-11.MAP");
    if (!nomap && filenamemapmy.empty())
        filesnotfound.push_back("*-my.MAP");
    if (filenamesav11.empty())
        filesnotfound.push_back("*-11.SAV");
    if (filenamesavmy.empty())
        filesnotfound.push_back("*-my.SAV");
    if (!nostb && filenamestb11.empty())
        filesnotfound.push_back("*-11.STB");
    if (!nostb && filenamestbmy.empty())
        filesnotfound.push_back("*-my.STB");

    bool isfileabsent = filesnotfound.size() > 0;

    if (!filenamemap11.empty() && !filenamemapmy.empty())
    {
        resmaps = process_map_files(stestdirpath + PATH_SEPARATOR + filenamemap11, stestdirpath + PATH_SEPARATOR + filenamemapmy, fileproblems);
    }
    string filepathsav11 = stestdirpath + PATH_SEPARATOR + filenamesav11;
    string filepathsavmy = stestdirpath + PATH_SEPARATOR + filenamesavmy;
    if (!filenamesav11.empty() && !filenamesavmy.empty())
    {
        ressavs = countdiff_binary_files(filepathsav11, filepathsavmy, "SAV", fileproblems);
    }
    string filepathstb11 = stestdirpath + PATH_SEPARATOR + filenamestb11;
    string filepathstbmy = stestdirpath + PATH_SEPARATOR + filenamestbmy;
    if (!filenamestb11.empty() && !filenamestbmy.empty())
    {
        resstbs = countdiff_binary_files(filepathstb11, filepathstbmy, "STB", fileproblems);
    }

    // Reporting all the problems found
    bool passed = !isfileabsent && resmylog && resmaps && ressavs && resstbs;
    if (g_verbose || !passed)
    {
        SetTextAttribute(TEXTATTRIBUTES_NORMAL);
        std::cout << "Test " << stestdirname;
        if (g_verbose && passed)
            std::cout << " - PASSED";
        std::cout << std::endl;
        SetTextAttribute(TEXTATTRIBUTES_WARNING);
    }
    if (filesnotfound.size() > 0)
    {
        std::cout << "  Files not found:";
        for (std::vector<string>::iterator it = filesnotfound.begin(); it != filesnotfound.end(); ++it)
        {
            std::cout << " " << *it;
        }
        std::cout << std::endl;
    }
    if (logproblems.size() > 0)
    {
        const string mylogprefix = "  my.LOG: ";
        for (std::vector<string>::iterator it = logproblems.begin(); it != logproblems.end(); ++it)
        {
            std::cout << mylogprefix << *it << std::endl;
        }
    }
    if (fileproblems.size() > 0)
    {
        for (std::vector<string>::iterator it = fileproblems.begin(); it != fileproblems.end(); ++it)
        {
            std::cout << "  " << *it << std::endl;
        }
    }
    if (g_verbose)
    {
        if (!ressavs)
            showdiff_binary_files(filepathsav11, filepathsavmy, "SAV", g_maxchunkstoshow);
        if (!resstbs)
            showdiff_binary_files(filepathstb11, filepathstbmy, "STB", g_maxchunkstoshow);
    }

    // Update counters and finish with this test
    if (!resmylog)
        g_testsfailed++;
    else if (isfileabsent)
    {
        std::cout << "  Test skipped." << std::endl;
        g_testskipped++;
    }
    else if (!resmaps || !ressavs || !resstbs)
        g_testsfailed++;
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
#ifdef _MSC_VER
    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    SetTextAttribute(TEXTATTRIBUTES_TITLE);
    std::cout << "TestAnalyzer utility for PCLINK11 project" << std::endl;
    SetTextAttribute(TEXTATTRIBUTES_NORMAL);

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

        //std::cout << "Test #" << g_testcount << " " << stestdirname << std::endl;
        process_test(stestdirname);
    }

    // Show totals
    SetTextAttribute(TEXTATTRIBUTES_TITLE);
    std::cout << "TOTAL tests: " << g_testcount;
    if (g_verbose)
    {
        int testspassed = g_testcount - g_testsfailed;
        std::cout << ", passed: " << testspassed << " (" << testspassed * 100.0 / g_testcount << "%)";
    }
    if (g_testsfailed > 0)
    {
        SetTextAttribute(TEXTATTRIBUTES_WARNING);
        std::cout << ", failed: " << g_testsfailed << " (" << g_testsfailed * 100.0 / g_testcount << "%)";
    }
    if (g_testskipped > 0)
    {
        SetTextAttribute(TEXTATTRIBUTES_WARNING);
        std::cout << ", skipped: " << g_testskipped;
    }
    std::cout << std::endl;
    SetTextAttribute(TEXTATTRIBUTES_NORMAL);

    if (g_testsfailed > 0)
        return 5;
    if (g_testskipped > 0)
        return 2;
    return 0;
}
