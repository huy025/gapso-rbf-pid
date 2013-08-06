DESTDIR=/usr/
ROOTDIR=$(DESTDIR)
LIBDIR=/usr/lib/
SBINDIR=/sbin
CONFDIR=/etc/iproute2
DOCDIR=/share/doc/iproute2
MANDIR=/share/man
ARPDDIR=/var/lib/arpd
INCDIR1=./include/

# Path to db_185.h include
DBM_INCLUDE:=$(ROOTDIR)/usr/include

SHARED_LIBS = y

DEFINES= -DRESOLVE_HOSTNAMES -DLIBDIR=\"$(LIBDIR)\"
ifneq ($(SHARED_LIBS),y)
DEFINES+= -DNO_SHARED_LIBS
endif

#options if you have a bind>=4.9.4 libresolv (or, maybe, glibc)
LDLIBS=-lresolv
ADDLIB=

#options for decnet
ADDLIB+=dnet_ntop.o dnet_pton.o

#options for ipx
ADDLIB+=ipx_ntop.o ipx_pton.o

CC = gcc
HOSTCC = gcc
CCOPTS = -D_GNU_SOURCE -O2 -Wstrict-prototypes -Wall
CFLAGS = $(CCOPTS) -I../include $(DEFINES) -g
YACCFLAGS = -d -t -v

SUBDIRS=iproute2-3.1.0 modules queuedata 

LIBNETLINK=../lib/libnetlink.a ../lib/libutil.a
LDLIBS += $(LIBNETLINK)

all: Config
	@set -e; \
	for i in $(SUBDIRS); \
	do $(MAKE) $(MFLAGS) -C $$i; done

Config:
	sh configure $(KERNEL_INCLUDE)

install: all

snapshot:
	echo "static const char SNAPSHOT[] = \""`date +%y%m%d`"\";" \
		> include/SNAPSHOT.h

clean:
	rm -f cscope.*
	@for i in $(SUBDIRS); \
	do $(MAKE) $(MFLAGS) -C $$i clean; done

clobber:
	touch Config
	$(MAKE) $(MFLAGS) clean
	rm -f Config

distclean: clobber

cscope:
	cscope -b -q -R -Iinclude -sip -slib -smisc -snetem -stc

.EXPORT_ALL_VARIABLES:
