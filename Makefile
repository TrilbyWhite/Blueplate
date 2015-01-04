
PROG     =  blueplate
VER      =  0.1
CC       ?= gcc
DEFS     =  -DPROGRAM_NAME=${PROG} -DPROGRAM_VER=${VER}
DEPS     = x11
CFLAGS   += $(shell pkg-config --cflags ${DEPS}) ${DEFS}
LDLIBS   += $(shell pkg-config --libs ${DEPS}) -lm
PREFIX   ?= /usr
MODULES  ?=  mail desktop
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
