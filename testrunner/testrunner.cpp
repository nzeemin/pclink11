// testrunner.cpp : Runs tests for PCLINK11.
//

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#include "testrunner.h"

typedef std::string string;


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


void remove_file(string testdirpath, string filename)
{
    if (filename.empty())
        return;

    //std::cout << "Removing " << filename << std::endl;
    string fullpath = testdirpath + "\\" + filename;
    remove(fullpath.c_str());
    //TODO: Check return value
}

void rename_file(string testdirpath, string filename, string filenamenew)
{
    if (filename.empty())
        return;

    //std::cout << "Renaming " << filename << " to " << filenamenew << std::endl;
    string fullpath = testdirpath + "\\" + filename;
    string fullpathnew = testdirpath + "\\" + filenamenew;
    if (0 != rename(fullpath.c_str(), fullpathnew.c_str()))
    {
        std::cout << "Failed to rename file " << filename << " to " << filenamenew << " : errno " << errno << std::endl;
        return;
    }
}

void remove_test_artifacts(const TestDescriptor & test)
{
    string testdirpath = string("tests\\") + test.directory;

    // Remove *-my.log *-my.SAV *-my.MAP *-my.STB
    string filenamelogmy = findfile_bymask(testdirpath, "*-my.log");
    remove_file(testdirpath, filenamelogmy);
    string filenamesavmy = findfile_bymask(testdirpath, "*-my.SAV");
    remove_file(testdirpath, filenamesavmy);
    string filenamemapmy = findfile_bymask(testdirpath, "*-my.MAP");
    remove_file(testdirpath, filenamemapmy);
    string filenamestbmy = findfile_bymask(testdirpath, "*-my.STB");
    remove_file(testdirpath, filenamestbmy);
}

void process_test_run(string workingdir, string modulename, string commandline, string outfilename)
{
    SECURITY_ATTRIBUTES sa;  memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hOutFile = ::CreateFile(
            outfilename.c_str(),
            GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOutFile == INVALID_HANDLE_VALUE)
    {
        std::cout << "Failed to open log file: " << ::GetLastError() << std::endl;
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
        std::cout << "Failed to run the test: " << ::GetLastError() << std::endl;
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

void process_test(const TestDescriptor & test)
{
    string testdirpath = string("tests\\") + test.directory;

    // Prepare command line
    string pclink11path = "Debug\\pclink11.exe";
    string commandline = string(test.commandline);
    string outfilename = testdirpath + "\\" + test.name + "-my.log";

    // Make sure we have some .OBJ files in the test folder
    string filenameobj = findfile_bymask(testdirpath, string(test.name) + ".OBJ");
    if (filenameobj.empty())
    {
        std::cout << "Failed to run the test: no .OBJ files found in " << testdirpath << std::endl;
        return;
    }

    // Run the test
    //std::cout << test.name << ": " << pclink11path << " : " << commandline << " at " << testdirpath << std::endl;
    std::cout << test.directory << " / " << test.name << std::endl;
    process_test_run(testdirpath, pclink11path, commandline, outfilename);

    // Rename files
    string filenamesav = findfile_bymask(testdirpath, string(test.name) + ".SAV");
    rename_file(testdirpath, filenamesav, string(test.name) + "-my.SAV");
    string filenamesys = findfile_bymask(testdirpath, string(test.name) + ".SYS");
    rename_file(testdirpath, filenamesys, string(test.name) + "-my.SAV");
    string filenamemap = findfile_bymask(testdirpath, string(test.name) + ".MAP");
    rename_file(testdirpath, filenamemap, string(test.name) + "-my.MAP");
    string filenamestb = findfile_bymask(testdirpath, string(test.name) + ".STB");
    rename_file(testdirpath, filenamestb, string(test.name) + "-my.STB");
}

int main(int argc, char *argv[])
{
    // Show title message
    std::cout << "TestRunner utility for PCLINK11 project" << std::endl;

    //TODO: Parse command line

    // Run all tests
    for (int testno = 0; testno < g_TestNumber; testno++)
    {
        const TestDescriptor & test = g_Tests[testno];

        // Remove files from previous test runs
        remove_test_artifacts(test);

        // Run the test
        process_test(test);
    }

    std::cout << "Total tests executed: " << g_TestNumber << std::endl;

    return 0;
}
