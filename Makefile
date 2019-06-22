
CXXFLAGS = -std=c++11 -O3 -Wall

SOURCES_DUMPOBJ = dumpobj/dumpobj.cpp
SOURCES_TESTANALYZER = testanalyzer/testanalyzer.cpp
SOURCES = main.cpp util.cpp $(SOURCES_DUMPOBJ) $(SOURCES_TESTANALYZER)

OBJECTS = main.o util.o
OBJECTS_DUMPOBJ = dumpobj.o
OBJECTS_TESTANALYZER = testanalyzer.o

all: pclink11 dumpobj testanalyzer

pclink11: version.h $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o pclink11 $(OBJECTS)

version.h:
	$(eval GIT_REVISION=$(shell git rev-list HEAD --count))
	@echo The revision is $(GIT_REVISION)
	@echo "" > version.h
	@echo "#define APP_REVISION $(GIT_REVISION)" >> version.h
	@echo "" >> version.h
	@echo "#define APP_VERSION_STRING \"V0.$(GIT_REVISION)\"" >> version.h

dumpobj:
	$(CXX) $(CXXFLAGS) -o dumpobj $(OBJECTS_DUMPOBJ)

testanalyzer:
	$(CXX) $(CXXFLAGS) -o testanalyzer $(OBJECTS_TESTANALYZER)

.PHONY: clean

clean:
	rm -f $(OBJECTS)
	rm version.h
