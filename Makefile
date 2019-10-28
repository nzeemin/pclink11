
CXXFLAGS = -std=c++11 -O3 -Wall

SOURCES_DUMPOBJ = dumpobj/dumpobj.cpp
SOURCES_TESTRUNNER = testrunner/testrunner.cpp testrunner/testlist.cpp
SOURCES_TESTANALYZER = testanalyzer/testanalyzer.cpp
SOURCES = main.cpp util.cpp $(SOURCES_DUMPOBJ) $(SOURCES_TESTANALYZER) $(SOURCES_TESTRUNNER)

OBJECTS_PCLINK11 = main.o util.o
OBJECTS_DUMPOBJ = dumpobj/dumpobj.o
OBJECTS_TESTRUNNER = testrunner/testrunner.o testrunner/testlist.o
OBJECTS_TESTANALYZER = testanalyzer/testanalyzer.o

all: pclink11 dumpobj testrunner testanalyzer

pclink11: version.h $(OBJECTS_PCLINK11)
	$(CXX) $(CXXFLAGS) -o pclink11 $(OBJECTS_PCLINK11)

version.h:
	$(eval GIT_REVISION=$(shell git rev-list HEAD --count))
	@echo The revision is $(GIT_REVISION)
	@echo "" > version.h
	@echo "#define APP_REVISION $(GIT_REVISION)" >> version.h
	@echo "" >> version.h
	@echo "#define APP_VERSION_STRING \"V0.$(GIT_REVISION)\"" >> version.h

dumpobj: $(OBJECTS_DUMPOBJ)
	$(CXX) $(CXXFLAGS) -o dumpobj/dumpobj $(OBJECTS_DUMPOBJ)

testrunner: $(OBJECTS_TESTRUNNER)
	$(CXX) $(CXXFLAGS) -o testrunner/testrunner $(OBJECTS_TESTRUNNER)

testanalyzer: $(OBJECTS_TESTANALYZER)
	$(CXX) $(CXXFLAGS) -o testanalyzer/testanalyzer $(OBJECTS_TESTANALYZER)

.PHONY: clean

clean:
	rm -f $(OBJECTS_PCLINK11)
	rm -f $(OBJECTS_DUMPOBJ)
	rm -f $(OBJECTS_TESTRUNNER)
	rm -f $(OBJECTS_TESTANALYZER)
	rm version.h
