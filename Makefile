
PROG     =  blueplate
#MODULES  ?= connman desktop mail
MODULES  ?= desktop mail
VER      =  0.1
CC       ?= gcc
MODDEFS  =  $(foreach mod, ${MODULES}, -Dmodule_${mod})
DEFS     =  -Dprogram_name=${PROG} -Dprogram_ver=${VER} ${MODDEFS}
#DEPS     = x11 dbus-1
DEPS     =  x11
CFLAGS   += $(shell pkg-config --cflags ${DEPS}) ${DEFS}
#LDLIBS   += $(shell pkg-config --libs ${DEPS}) -lm -lpthread
LDLIBS   += $(shell pkg-config --libs ${DEPS}) -lm
PREFIX   ?= /usr
HEADERS  =  config.h blueplate.h
VPATH    =  src

${PROG}: ${MODULES:%=%.o}

%.o: %.c ${HEADERS}

install: ${PROG}
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}

clean:
	@rm -f ${PROG}-${VER}.tar.gz ${MODULES:%=%.o}

distclean: clean
	@rm -f ${PROG}

dist: distclean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean dist distclean
