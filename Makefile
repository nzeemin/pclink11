
CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall

SOURCES = main.cpp util.cpp

OBJECTS = main.o util.o

all: pclink11

pclink11: version.h $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o pclink11 $(OBJECTS)

version.h:
	$(eval GIT_REVISION=$(shell git rev-list HEAD --count))
	@echo The revision is $(GIT_REVISION)
	@echo "" > version.h
	@echo "#define APP_REVISION $(GIT_REVISION)" >> version.h
	@echo "" >> version.h
	@echo "#define APP_VERSION_STRING \"V0.$(GIT_REVISION)\"" >> version.h

.PHONY: clean

clean:
	rm -f $(OBJECTS)
	rm version.h
