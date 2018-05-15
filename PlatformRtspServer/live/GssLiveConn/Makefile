INCLUDES = -Iinclude
PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
##### Change the following for your environment:
COMPILE_OPTS =		$(INCLUDES) -I. -g -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
C =			c
C_COMPILER =		cc
C_FLAGS =		$(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP =			cpp
CPLUSPLUS_COMPILER =	c++
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall -DBSD=1 $(CPPFLAGS) $(CXXFLAGS)
OBJ =			o
LINK =			c++ -o
LDFLAGS := $(LDFLAGS) 	-lp2p.a
LINK_OPTS =		-L./lib $(LDFLAGS)
CONSOLE_LINK_OPTS =	$(LINK_OPTS)
LIBRARY_LINK =		ar cr 
LIBRARY_LINK_OPTS =
LIB_SUFFIX =			a
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
EXE =
##### End of variables to change

NAME = libgssliveconn
ALL = $(NAME).$(LIB_SUFFIX)
all:	$(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

GSSLIVECONN_LIB_OBJS = GssLiveConn.$(OBJ) MyClock.$(OBJ) MySemaphore.$(OBJ) Log.$(OBJ)

GssLiveConn.$(CPP):	include/GssLiveConn.h
include/GssLiveConn.h:	include/gss_transport.h include/p2p_dispatch.h include/p2p_transport.h include/MyClock.h
MyClock.$(CPP):     include/MyClock.h
MySemaphore.$(CPP):    include/MySemaphore.h
Log.$(CPP):    include/Log.h

libgssliveconn.$(LIB_SUFFIX): $(GSSLIVECONN_LIB_OBJS) \
    $(PLATFORM_SPECIFIC_LIB_OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
		$(GSSLIVECONN_LIB_OBJS) 

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

install: install1 $(INSTALL2)
install1: libgssliveconn.$(LIB_SUFFIX)
	  install -d $(DESTDIR)$(PREFIX)/include/gssliveconn $(DESTDIR)$(LIBDIR)
	  install -m 644 include/*.hh include/*.h $(DESTDIR)$(PREFIX)/include/gssliveconn
	  install -m 644 libgssliveconn.$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)
install_shared_libraries: libgssliveconn.$(LIB_SUFFIX)
	  ln -fs libgssliveconn.$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)/libgssliveconn.$(SHORT_LIB_SUFFIX)
	  ln -fs libgssliveconn.$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)/libgssliveconn.so

##### Any additional, platform-specific rules come here:
