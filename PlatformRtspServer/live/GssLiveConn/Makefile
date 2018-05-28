INCLUDES = -Iinclude
PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
##### Change the following for your environment:
SSL = openssl
COMPILE_OPTS =		$(INCLUDES) -I. -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DALLOW_SERVER_PORT_REUSE
C =			c
C_COMPILER =		cc
C_FLAGS =		$(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP =			cpp
CPLUSPLUS_COMPILER =	c++
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall -DBSD=1 $(CPPFLAGS) $(CXXFLAGS)
OBJ =			o
LINK =			c++ -o
LINK_OPTS =		-L. $(LDFLAGS)
CONSOLE_LINK_OPTS =	$(LINK_OPTS)
LIBRARY_LINK =		ar cr 
LIBRARY_LINK_OPTS =	
LIB_SUFFIX =			a
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
EXE =

ifeq ($(SSL),openssl) 
LIBS_SSL_DIR := ../openssl
LIBS_SSL :=  -lp2p -pthread -lssl -lcrypto -ldl
CONSOLE_LINK_OPTS +=		-DSSL_MODE  -I$(LIBS_SSL_DIR)/include -L$(LIBS_SSL_DIR)/lib
endif
##### End of variables to change

NAME = libgssliveconn
ALL = $(NAME).$(LIB_SUFFIX)
all:	$(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

GROUPSOCK_LIB_OBJS = GssLiveConn.$(OBJ) Log.$(OBJ) MyClock.$(OBJ) MySemaphore.$(OBJ)

GssLiveConn.$(CPP):	include/GssLiveConn.h
include/GssLiveConn.h:	include/MyClock.h
include/NetAddress.hh:	include/NetCommon.h include/MySemaphore.h
Log.$(CPP):	include/Log.h
MyClock.$(CPP):		include/MyClock.h
MySemaphore.$(CPP):	include/MySemaphore.h

libgssliveconn.$(LIB_SUFFIX): $(GROUPSOCK_LIB_OBJS) \
    $(PLATFORM_SPECIFIC_LIB_OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
		$(GROUPSOCK_LIB_OBJS)

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

install: install1 $(INSTALL2)
install1: libgssliveconn.$(LIB_SUFFIX)
	  install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(LIBDIR)
	  install -m 644 include/*.hh include/*.h 
	  install -m 644 libgssliveconn.$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)
install_shared_libraries: libgroupsock.$(LIB_SUFFIX)
	  ln -fs libgssliveconn.$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)/libgssliveconn.$(SHORT_LIB_SUFFIX)
	  ln -fs libgssliveconn.$(LIB_SUFFIX) $(DESTDIR)$(LIBDIR)/libgssliveconn.so

##### Any additional, platform-specific rules come here:
