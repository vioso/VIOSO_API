CLEANEXTS = o so

SOURCES = logging.cpp PathHelper.cpp VIOSOWarpBlend.cpp VWF.cpp GL/GLWarpBlend.cpp GL/GLWarpBlendXPL.cpp 3rdparty/tinyxml2/tinyxml2.cpp

INTSUBDIRS = GL

OUTPUTFILE = libVIOSOWarpBlend.so

INSTALLDIR = ../bin

CXXFLAGS = -shared -fPIC -std=c++2a
CCFLAGS = -Wall

LDFLAGS = -lGL -lGLU

.PHONY: all
all: $(OUTPUTFILE)
$(OUTPUTFILE): $(subst .cpp,.o,$(SOURCES)) 3rdparty/aes/aes.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: install
install:
	mkdir -p $(INSTALLDIR)
	cp -p $(OUTPUTFILE) $(INSTALLDIR)

.PHONY: clean
clean:
	for file in $(CLEANEXTS); do rm -f *.$$file; done
	for file in $(CLEANEXTS); do rm -f GL/*.$$file; done
	for file in $(CLEANEXTS); do rm -f 3rdparty/tinyxml2/*.$$file; done
	for file in $(CLEANEXTS); do rm -f 3rdparty/aes/*.$$file; done
 
