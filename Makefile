# find the OS
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# Compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_CFLAGS ?= -W -Wall -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -shared

	ifeq ($(GCOV_MODE),TRUE)
		SHOBJ_CFLAGS += -fprofile-arcs -ftest-coverage -DGCOV_MODE
		LDFLAGS += -lgcov -lgcc -lm  -lc
	endif
else
	SHOBJ_CFLAGS ?= -W -Wall -dynamic -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -bundle -undefined dynamic_lookup
endif

TOPDIR := $(shell pwd)
SRCDIR := ${TOPDIR}/src
CJSONDIR := ${SRCDIR}/cJSON

SOURCE_FILES_C := ${CJSONDIR}/cJSON.c ${CJSONDIR}/cJSON_Utils.c ${SRCDIR}/tairdoc.c
OBJS := $(SOURCE_FILES_C:.c=.o)
INCLUDE = -I ${SRCDIR} -I ${CJSONDIR}

vpath %.c ${SRCDIR}

all: tairdoc.so

%.o: %.c
	$(CC) ${INCLUDE} $(CFLAGS) $(SHOBJ_CFLAGS) -fPIC -c $< -o $@

tairdoc.so: ${OBJS}
	$(CC) $^ -o $@ $(SHOBJ_LDFLAGS) ${LDFLAGS}

clean:
	-rm -rf *.so *.o ${OBJS}
	-rm -rf ${SRCDIR}/*.gcda ${SRCDIR}/*.gcno ${SRCDIR}/*.gcov
	-rm -rf ${CJSONDIR}/*.gcda ${CJSONDIR}/*.gcno ${CJSONDIR}/*.gcov