// testrunner.cpp : Runs tests for PCLINK11.
//

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#include "testrunner.h"

typedef std::string string;

bool g_verbose = false;  // Verbose mode

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
const char * PCLINK11_PATH = "Debug\\pclink11.exe";
const char * PATH_SEPARATOR = "\\";
#else
const char * TESTS_SUB_DIR = "tests/";
const char * PCLINK11_PATH = "../../pclink11";
const char * PATH_SEPARATOR = "/";
#endif

#ifdef _MSC_VER
// Get first file by mask in the directory. Win32 specific method
string findfile_bymask(const string & dirname, const string & mask)
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

void remove_file(const string & testdirpath, const string & filename)
{
    if (filename.empty())
        return;

    //std::cout << "Removing " << filename << std::endl;
    string fullpath = testdirpath + PATH_SEPARATOR + filename;
    if (0 != remove(fullpath.c_str()))
    {
        std::cout << "Failed to delete file " << filename << " : errno " << errno << std::endl;
        return;
    }
}

void rename_file(const string & testdirpath, const string & filename, const string & filenamenew)
{
    if (filename.empty())
        return;

    //std::cout << "Renaming " << filename << " to " << filenamenew << std::endl;
    string fullpath = testdirpath + PATH_SEPARATOR + filename;
    string fullpathnew = testdirpath + PATH_SEPARATOR + filenamenew;
    if (0 != rename(fullpath.c_str(), fullpathnew.c_str()))
    {
        std::cout << "Failed to rename file " << filename << " to " << filenamenew << " : errno " << errno << std::endl;
        return;
    }
}

void remove_test_artifacts(const TestDescriptor & test)
{
    string testdirpath = string(TESTS_SUB_DIR) + test.directory;

    // Remove *-my.log *-my.SAV *-my.LDA *-my.MAP *-my.STB
    string filenamelogmy = findfile_bymask(testdirpath, "-my.log");
    remove_file(testdirpath, filenamelogmy);
    string filenamesavmy = findfile_bymask(testdirpath, "-my.SAV");
    remove_file(testdirpath, filenamesavmy);
    string filenameldamy = findfile_bymask(testdirpath, "-my.LDA");
    remove_file(testdirpath, filenameldamy);
    string filenamemapmy = findfile_bymask(testdirpath, "-my.MAP");
    remove_file(testdirpath, filenamemapmy);
    string filenamestbmy = findfile_bymask(testdirpath, "-my.STB");
    remove_file(testdirpath, filenamestbmy);
}

#ifdef _MSC_VER
void process_test_run(const string & workingdir, const string & modulename, const string & commandline, const string & outfilename)
{
    string outfilenamewithdir = workingdir + PATH_SEPARATOR + outfilename;
    SECURITY_ATTRIBUTES sa;  memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hOutFile = ::CreateFile(
            outfilenamewithdir.c_str(),
            GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOutFile == INVALID_HANDLE_VALUE)
    {
        std::cout << "Failed to open log file: error " << ::GetLastError() << std::endl;
        return;
    }

    string fullcommand = modulename + " " + commandline;
    char command[512];
    strcpy(command, fullcommand.c_str());

    STARTUPINFO si;  memset(&si, 0, sizeof(si));  si.cb = sizeof(si);
    si.hStdOutput = hOutFile;
    si.hStdError = hOutFile;
    si.dwFlags |= STARTF_USESTDHANDLES;
    PROCESS_INFORMATION pi;  memset(&pi, 0, sizeof(pi));
    if (!::CreateProcess(
            modulename.c_str(), command, NULL, NULL, TRUE,
            CREATE_NO_WINDOW, NULL, workingdir.c_str(), &si, &pi))
    {
        std::cout << "Failed to run the test: error " << ::GetLastError() << std::endl;
        ::CloseHandle(hOutFile);
        return;
    }

    // Wait until child process exits
    ::WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(hOutFile);
}
#else
void process_test_run(const string & workingdir, const string & modulename, const string & commandline, const string & outfilename)
{
    char bufcwd[PATH_MAX];
    getwd(bufcwd);
    chdir(workingdir.c_str());

    string fullcommand = modulename + " " + commandline + ">" + outfilename;
    //std::cout << "Running the test: " << fullcommand << std::endl;
    int result = std::system(fullcommand.c_str());
    if (result != 0)
    {
        std::cout << "Failed to run the test: result " << result << std::endl;
    }

    chdir(bufcwd);
}
#endif

void process_test(const TestDescriptor & test)
{
    SetTextAttribute(TEXTATTRIBUTES_NORMAL);
    std::cout << test.directory << " / " << test.name << std::endl;
    SetTextAttribute(TEXTATTRIBUTES_WARNING);

    string testdirpath = string(TESTS_SUB_DIR) + test.directory;

    // Prepare command line
    string pclink11path = PCLINK11_PATH;
    string commandline = string(test.commandline);
    string outfilename = string(test.name) + "-my.log";

    // Make sure we have some .OBJ files in the test folder
    //string filenameobj = findfile_bymask(testdirpath, string(test.name) + ".OBJ");
    //if (filenameobj.empty())
    //{
    //    std::cout << "Failed to run the test: no .OBJ files found in " << testdirpath << std::endl;
    //    return;
    //}

    // Run the test
    process_test_run(testdirpath, pclink11path, commandline, outfilename);

    // Rename files
    string filenamesav = findfile_bymask(testdirpath, string(test.name) + ".SAV");
    rename_file(testdirpath, filenamesav, string(test.name) + "-my.SAV");
    string filenamesys = findfile_bymask(testdirpath, string(test.name) + ".SYS");
    rename_file(testdirpath, filenamesys, string(test.name) + "-my.SAV");
    string filenamerel = findfile_bymask(testdirpath, string(test.name) + ".REL");
    rename_file(testdirpath, filenamerel, string(test.name) + "-my.SAV");
    string filenamelda = findfile_bymask(testdirpath, string(test.name) + ".LDA");
    rename_file(testdirpath, filenamelda, string(test.name) + "-my.LDA");
    string filenamemap = findfile_bymask(testdirpath, string(test.name) + ".MAP");
    rename_file(testdirpath, filenamemap, string(test.name) + "-my.MAP");
    string filenamestb = findfile_bymask(testdirpath, string(test.name) + ".STB");
    rename_file(testdirpath, filenamestb, string(test.name) + "-my.STB");
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
            else
            {
                std::cout << "Unknown option: " << option << std::endl;
            }
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
    std::cout << "TestRunner utility for PCLINK11 project" << std::endl;
    SetTextAttribute(TEXTATTRIBUTES_NORMAL);

    // Parse command line
    parse_commandline(argc, argv);

    // Run all tests
    for (int testno = 0; testno < g_TestNumber; testno++)
    {
        const TestDescriptor & test = g_Tests[testno];
        //std::cout << "Test #" << testno << " " << test.name << std::endl;

        // Remove files from previous test runs
        remove_test_artifacts(test);

        // Run the test
        process_test(test);
    }

    SetTextAttribute(TEXTATTRIBUTES_TITLE);
    std::cout << "Total tests executed: " << g_TestNumber << std::endl;
    SetTextAttribute(TEXTATTRIBUTES_NORMAL);

    return 0;
}
