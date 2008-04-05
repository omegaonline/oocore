CC	     =  g++
INCLUDES     =   -I include/ -I include/OTL -I src/OOCore/

CFLAGS       =  -Wall -g -fno-rtti -Wno-non-virtual-dtor -fPIC -pthread -O0 -fvisibility=hidden -DOMEGA_DEBUG $(INCLUDES)
CXXFLAGS     =  $(CFLAGS)
LDFLAGS      =  -lACE
TARGET	     =  libOOCored.so

SRCS	     = src/OOCore/Registry.cpp     	\
	       src/OOCore/RunningObjectTable.cpp\
	       src/OOCore/StdObjectManager.cpp	\
	       src/OOCore/Threading.cpp		\
	       src/OOCore/Types.cpp		\
	       src/OOCore/UserSession.cpp	\
	       src/OOCore/WireImpl.cpp		\
	       src/OOCore/WireProxy.cpp		\
	       src/OOCore/WireStub.cpp		\
	       src/OOCore/Xml.cpp		\
	       src/OOCore/Activation.cpp	\
	       src/OOCore/Channel.cpp		\
	       src/OOCore/Exception.cpp		\
	       src/OOCore/OOCore.cpp		\
	       src/OOCore/OOCore_precomp.h

OBJS	     = src/OOCore/Registry.o     	\
	       src/OOCore/RunningObjectTable.o 	\
	       src/OOCore/StdObjectManager.o	\
	       src/OOCore/Threading.o		\
	       src/OOCore/Types.o		\
	       src/OOCore/UserSession.o		\
	       src/OOCore/WireImpl.o		\
	       src/OOCore/WireProxy.o		\
	       src/OOCore/WireStub.o		\
	       src/OOCore/Xml.o			\
	       src/OOCore/Activation.o		\
	       src/OOCore/Channel.o		\
	       src/OOCore/Exception.o		\
	       src/OOCore/OOCore.o		\

# force dependecy generation
build: $(TARGET)
	echo "making OOCore library $(TARGET)"

full:clean build

$(TARGET): depend $(OBJS)
	$(CC) -shared $(OBJS) $(LDFLAGS) $(CFLAGS) -Wl -o $(TARGET)  

clean:
	rm -f $(OBJS) $(TARGET)
	[ -f src/OOCore/.depend ] && rm -f src/OOCore/.depend 

src/OOCore/.depend : depend

depend: $(SRCS)
	$(CC) -MM $(CFLAGS) $(SRCS) > src/OOCore/.depend
include src/OOCore/.depend
