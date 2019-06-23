// testrunner.h : Headers for TestRunner.
//

struct TestDescriptor
{
    const char * directory;
    const char * name;
    const char * commandline;
};
extern const TestDescriptor g_Tests[];
extern const int g_TestNumber;