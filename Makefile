TARGET = owata
VER = $(shell date +%Y%m%d)
OBJS = charConv.o main.o pg.o psp2ch.o psp2chAudio.o psp2chError.o psp2chFavorite.o psp2chForm.o \
psp2chImageView.o psp2chIni.o psp2chIta.o psp2chMenu.o psp2chNet.o psp2chWlan.o psp2chRes.o \
psp2chResWindow.o psp2chReg.o psp2chSearch.o psp2chThread.o pspdialogs.o psphtmlviewer.o \
psp2chImageReader.o libCat/Cat_Network.o libCat/Cat_Resolver.o \
SMEMO/smemomain.o SMEMO/zenkaku.o SMEMO/shinonomefont.o \
SMEMO/draw.o SMEMO/sime.o SMEMO/graphics.o \
SMEMO/framebuffer.o SMEMO/osk.o SMEMO/filedialog.o SMEMO/strInput.o \
unzip/miniunz.o unzip/unzip.o unzip/ioapi.o \
unrarlib/filestr.o unrarlib/recvol.o unrarlib/rs.o unrarlib/scantree.o unrarlib/rar.o \
unrarlib/strlist.o unrarlib/strfn.o unrarlib/pathfn.o unrarlib/int64.o unrarlib/savepos.o \
unrarlib/global.o unrarlib/file.o unrarlib/filefn.o unrarlib/filcreat.o unrarlib/archive.o \
unrarlib/arcread.o unrarlib/unicode.o unrarlib/system.o unrarlib/isnt.o unrarlib/crypt.o \
unrarlib/crc.o unrarlib/rawread.o unrarlib/encname.o unrarlib/resource.o unrarlib/match.o \
unrarlib/timefn.o unrarlib/rdwrfn.o unrarlib/consio.o unrarlib/options.o unrarlib/ulinks.o \
unrarlib/errhnd.o unrarlib/rarvm.o unrarlib/rijndael.o unrarlib/getbits.o unrarlib/sha1.o \
unrarlib/extinfo.o unrarlib/extract.o unrarlib/volume.o unrarlib/list.o unrarlib/find.o \
unrarlib/unpack.o unrarlib/cmddata.o

INCDIR = ./include
CFLAGS = -G0 -O2 -Wall -W -fshort-wchar -fno-pic -mno-check-zero-division
#CFLAGS = -O2 -G0 -Wall
CXX = g++
#CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
CXXFLAGS = $(CFLAGS) -fno-rtti
ASFLAGS = $(CFLAGS) -c

LIBDIR = ./lib
LDFLAGS = 
BUILD_PRX = 0
PSP_FW_VERSION= 401
PSP_LARGE_MEMORY = 1
ENCRYPT = 0
LIBS = -lpsprtc -lpsppower -lpspgu -lpspssl -lpsphttp -lpspwlan -lpspaudiocodec -lpspaudio -lpspmp3
LIBS += -ljpeg -lpng -lgif -lz -lm -lonig -lpspdebug -lstdc++ -lpspkubridge

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = freeze owata+1 $(VER)
PSP_EBOOT_ICON = icon0.png

PSPSDK=$(shell psp-config --pspsdk-path)
include ./build.mak

PARAM.SFO : $(OBJS)
