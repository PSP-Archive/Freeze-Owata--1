PSPSDK=$(shell psp-config --pspsdk-path)
PSPDIR=$(shell psp-config --psp-prefix)
TARGET_LIB = libgif.a
OBJS =	dev2gif.o egif_lib.o dgif_lib.o gifalloc.o gif_font.o gif_err.o quantize.o qprintf.o getarg.o

CFLAGS = -fno-builtin -O2 -G0 -Wall -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDARG_H -D_GBA_NO_FILEIO

include $(PSPSDK)/lib/build.mak

install: $(TARGET_LIB)
	@echo "Installing libgif into $(PSPDIR)"
	@mkdir -p $(PSPDIR)/include $(PSPDIR)/lib
	@cp gif_lib.h $(PSPDIR)/include
	@cp libgif.a  $(PSPDIR)/lib
	@echo "Done"
