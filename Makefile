include common.mk
version = $(strip $(shell cat VERSION))

all: fs-uae

cppflags = -DFSEMU -DFSUAE

ifeq ($(debug), 1)
cppflags += -DDEBUG
endif

ifeq ($(wildcard libfsemu), libfsemu)
libfsemu_dir = libfsemu
libfsemu/out/libfsemu.a:
	make -C libfsemu
libfsemu-target: libfsemu/out/libfsemu.a
else
libfsemu_dir = ../libfsemu
libfsemu-target:
endif

ifeq ($(wildcard libcapsimage), libcapsimage)
libcapsimage_dir = libcapsimage
libcapsimage/out/CAPSImg.dll:
	make -C libcapsimage
libcapsimage-target: libcapsimage/out/CAPSImg.dll
else
libcapsimage_dir = ../libcapsimage
libcapsimage-target:
endif

ifeq ($(android), 1)
warnings = 
#-Wall -Wno-unused-variable
#-Wno-unused-but-set-variable
errors = -Werror=implicit-function-declaration -Werror=return-type
cxxflags = $(warnings) $(errors) -Isrc/od-fs -Isrc/od-fs/include \
        -Isrc/include -Igen -Isrc -Isrc/od-win32/caps \
        -I$(libfsemu_dir)/include \
		-Wno-write-strings -ggdb
cflags = -std=c99 $(cxxflags)
ldflags =
libs = -L$(libfsemu_dir)/out -lfsemu  \
		-lpng -lz

else
warnings = 
#-Wall -Wno-unused-variable
#-Wno-unused-but-set-variable
errors = -Werror=implicit-function-declaration -Werror=return-type
cxxflags = $(warnings) $(errors) -Isrc/od-fs -Isrc/od-fs/include \
        -Isrc/include -Igen -Isrc -Isrc/od-win32/caps \
		`pkg-config --cflags glib-2.0` -I$(libfsemu_dir)/include \
		-Wno-write-strings `sdl-config --cflags` -ggdb
cflags = -std=c99 $(cxxflags)
ldflags =
libs = -L$(libfsemu_dir)/out -lfsemu `sdl-config --libs` \
		`pkg-config --libs glib-2.0 gthread-2.0` -lpng -lz
endif

ifeq ($(optimize), 1)
	cflags += -O2
	cxxflags += -O2
else
	cflags += -O0
	cxxflags += -O0
endif

#uae_warnings = -Wno-unused-value -Wno-uninitialized -Wno-sign-compare
#uae_warnings += -fpermissive -Wno-unused-function -Wno-format
#uae_warnings +=  -Wmissing-braces -Wall -Wno-sign-compare
uae_warnings = -Wall -Wno-sign-compare
generate = 0

ifeq ($(os), android)
cppflags += -DANDROID
cxxflags += 
libs +=
else ifeq ($(os), windows)
cppflags += -DWINDOWS
cxxflags += -U_WIN32 -UWIN32
libs += -lOpenGL32 -lGLU32 -lgdi32 -lWinmm -lOpenAL32 -lWs2_32
else ifeq ($(os), macosx)
cflags += -arch i386
cxxflags += -arch i386
ldflags += -arch i386
cppflags += -DMACOSX
libs += -framework OpenGL -framework Carbon -framework OpenAL
else ifeq($(os), pandora)
cppflags += -DHAVE_GLES -DPANDORA
cxxflags += -DHAVE_GLES -DPANDORA
libs += -lGLES_CM -lGLUES_CM -lopenal -ldl -lX11 -lEGL
generate = 0
else
libs += -lGL -lGLU -lopenal -ldl -lX11
generate = 0
endif

ifeq ($(debug), 1)
libs += -mno-windows
endif

objects = \
obj/fs-uae-config.o \
obj/fs-uae-input.o \
obj/fs-uae-joystick.o \
obj/fs-uae-keyboard.o \
obj/fs-uae-main.o \
obj/fs-uae-menu.o \
obj/fs-uae-mouse.o \
obj/fs-uae-paths.o \
obj/fs-uae-uae_config.o \
obj/fs-uae-version.o \
obj/fs-uae-video.o

ifeq ($(os), windows)
objects += obj/fs-uae.res
endif

uae_objects = \
obj/gen-blitfunc.o \
obj/gen-blittable.o \
obj/gen-cpudefs.o \
obj/gen-cpuemu_0.o \
obj/gen-cpuemu_11.o \
obj/gen-cpuemu_12.o \
obj/gen-cpuemu_20.o \
obj/gen-cpuemu_21.o \
obj/gen-cpuemu_31.o \
obj/gen-cpustbl.o \
obj/a2091.o \
obj/akiko.o \
obj/amax.o \
obj/ar.o \
obj/arcadia.o \
obj/aros.rom.o \
obj/audio.o \
obj/autoconf.o \
obj/blitter.o \
obj/blkdev.o \
obj/blkdev_cdimage.o \
obj/bsdsocket.o \
obj/calc.o \
obj/cd32_fmv.o \
obj/cdrom.o \
obj/cdtv.o \
obj/cia.o \
obj/cfgfile.o \
obj/consolehook.o \
obj/cpummu.o \
obj/crc32.o \
obj/custom.o \
obj/debug.o \
obj/disk.o \
obj/diskutil.o \
obj/dongle.o \
obj/drawing.o \
obj/driveclick.o \
obj/enforcer.o \
obj/ersatz.o \
obj/events.o \
obj/expansion.o \
obj/fdi2raw.o \
obj/filesys.o \
obj/fpp.o \
obj/fsdb.o \
obj/fsusage.o \
obj/gayle.o \
obj/gfxutil.o \
obj/hardfile.o \
obj/hrtmon.rom.o \
obj/identify.o \
obj/inputrecord.o \
obj/isofs.o \
obj/keybuf.o \
obj/main.o \
obj/memory.o \
obj/missing.o \
obj/native2amiga.o \
obj/newcpu.o \
obj/ncr_scsi.o \
obj/readcpu.o \
obj/rommgr.o \
obj/savestate.o \
obj/scsi.o \
obj/scsiemul.o \
obj/serial.o \
obj/sinctable.o \
obj/specialmonitors.o \
obj/statusline.o \
obj/traps.o \
obj/uaeexe.o \
obj/uaelib.o \
obj/uaeresource.o \
obj/uaeserial.o \
obj/writelog.o \
obj/zfile.o \
obj/zfile_archive.o \
obj/zip-archiver-unzip.o \
obj/dms-archiver-crc_csum.o \
obj/dms-archiver-getbits.o \
obj/dms-archiver-maketbl.o \
obj/dms-archiver-pfile.o \
obj/dms-archiver-tables.o \
obj/dms-archiver-u_deep.o \
obj/dms-archiver-u_heavy.o \
obj/dms-archiver-u_init.o \
obj/dms-archiver-u_medium.o \
obj/dms-archiver-u_quick.o \
obj/dms-archiver-u_rle.o \
obj/gen-drive_click.o \
obj/gen-drive_snatch.o \
obj/gen-drive_spin.o \
obj/gen-drive_spinnd.o \
obj/gen-drive_startup.o \
obj/od-fs-audio.o \
obj/od-fs-bsdsocket_host.o \
obj/od-fs-caps.o \
obj/od-fs-cda_play.o \
obj/od-fs-charset.o \
obj/od-fs-clock.o \
obj/od-fs-cdimage_stubs.o \
obj/od-fs-clipboard.o \
obj/od-fs-filesys_host.o \
obj/od-fs-fsdb_host.o \
obj/od-fs-hardfile_host.o \
obj/od-fs-input.o \
obj/od-fs-inputdevice.o \
obj/od-fs-keymap.o \
obj/od-fs-libamiga.o \
obj/od-fs-logging.o \
obj/od-fs-mman.o \
obj/od-fs-parser.o \
obj/od-fs-random.o \
obj/od-fs-roms.o \
obj/od-fs-picasso96.o \
obj/od-fs-stubs.o \
obj/od-fs-support.o \
obj/od-fs-util.o \
obj/od-fs-threading.o \
obj/od-fs-uae_host.o \
obj/od-fs-uaemisc.o \
obj/od-fs-version.o \
obj/od-fs-video.o

ifeq ($(generate), 1)

gen/genblitter: obj/genblitter.o obj/blitops.o obj/writelog.o
	$(cxx) $(cppflags) $(cxxflags) obj/genblitter.o obj/blitops.o \
			obj/writelog.o -o gen/genblitter

gen/gencpu: obj/cpudefs.o obj/gencpu.o obj/readcpu.o obj/missing.o \
		gen/cpudefs.cpp obj/util.o
	$(cxx) $(cppflags) $(cxxflags) obj/gencpu.o obj/readcpu.o obj/missing.o \
			gen/cpudefs.cpp obj/util.o -o gen/gencpu

gen/genlinetoscr:
	$(cxx) $(cppflags) $(cxxflags) genlinetoscr.cpp -o gen/genlinetoscr

gen/blit.h: gen/genblitter
	gen/genblitter i > gen/blit.h

gen/blitfunc.c: gen/genblitter gen/blitfunc.h
	gen/genblitter f > gen/blitfunc.c

gen/blitfunc.h: gen/genblitter
	gen/genblitter h > gen/blitfunc.h

gen/blittable.c: gen/genblitter gen/blitfunc.h
	gen/genblitter t > gen/blittable.c

gen/build68k:
	$(cxx) $(cppflags) $(cxxflags) build68k.cpp writelog.cpp -o gen/build68k

gen/cpudefs.cpp: gen/build68k table68k
	./gen/build68k < table68k > gen/cpudefs.cpp
	python util/fixtchar.py gen/cpudefs.cpp

gen/cpuemu_0.cpp: gen/gencpu
	cd gen && ./gencpu --optimized-flags

gen/cpustbl.cpp: gen/cpuemu_0.cpp

gen/cputbl.h: gen/cpuemu_0.cpp

gen/cpuemu_11.cpp: gen/cpuemu_0.cpp
gen/cpuemu_12.cpp: gen/cpuemu_0.cpp
gen/cpuemu_20.cpp: gen/cpuemu_0.cpp
gen/cpuemu_21.cpp: gen/cpuemu_0.cpp
gen/cpuemu_31.cpp: gen/cpuemu_0.cpp

endif

gen/drive_%.cpp: sound/drive_%.wav
	(echo "unsigned char drive_$*_data[] = {"; od -txC -v $< | \
sed -e "s/^[0-9]*//" -e s"/ \([0-9a-f][0-9a-f]\)/0x\1,/g" -e"\$$d") > $@
	echo "0x00};" >> $@
	echo "int drive_$*_data_size = " >> $@
	ls -l $< | awk '{ print $$5}' >> $@
	echo ";" >> $@

obj/gen-%.o: gen/%.cpp
	$(cxx) $(cppflags) $(cxxflags) -c $< -o $@

obj/%.o: src/%.cpp
	$(cxx) $(cppflags) $(cxxflags) $(uae_warnings) -c $< -o $@

obj/zip-archiver-%.o: src/archivers/zip/%.cpp
	$(cxx) $(cppflags) $(cxxflags) $(uae_warnings) -c $< -o $@

obj/dms-archiver-%.o: src/archivers/dms/%.cpp
	$(cxx) $(cppflags) $(cxxflags) $(uae_warnings) -c $< -o $@

obj/od-fs-%.o: src/od-fs/%.cpp
	$(cxx) $(cppflags) $(cxxflags) -c $< -o $@

obj/uae.a: gen/blit.h $(uae_objects)
ifeq ($(os), macosx)
	rm -f $@
endif
	$(ar) cru $@ $(uae_objects)
ifeq ($(os), macosx)
	ranlib $@
endif

obj/fs-uae.res: src/fs-uae/fs-uae.rc
	windres $< -O coff -o $@

obj/fs-uae-%.o: src/fs-uae/%.c
	$(cc) $(cppflags) $(cflags) -c $< -o $@

ifeq ($(os), windows)
out/CAPSImg.dll:
	cp $(libcapsimage_dir)/out/CAPSImg.dll out/
run_deps: out/CAPSImg.dll
else
run_deps:
endif

out/fs-uae: libfsemu-target libcapsimage-target obj/uae.a $(objects) \
		run_deps
	$(cxx) $(cxxflags) $(ldflags) $(objects) obj/uae.a $(libs) -o out/fs-uae

fs-uae: out/fs-uae

dist_dir=fs-uae-$(version)

distdir:
	rm -Rf $(dist_dir)/*
	mkdir -p $(dist_dir)
	mkdir -p $(dist_dir)/obj
	mkdir -p $(dist_dir)/out
	cp -a INSTALL README COPYING VERSION Changelog $(dist_dir)
	cp -a common.mk targets.mk $(dist_dir)
	# windows.mk macosx.mk debian.mk
	cp -a Makefile fs-uae.spec example.conf $(dist_dir)
	cp -a src sound share licenses $(dist_dir)
	mkdir -p $(dist_dir)/gen
	cp -a gen/*.cpp gen/*.c gen/*.h $(dist_dir)/gen

	mkdir -p $(dist_dir)/libfsemu
	cp -a ../libfsemu/COPYING $(dist_dir)/libfsemu
	cp -a ../libfsemu/README $(dist_dir)/libfsemu
	cp -a ../libfsemu/Makefile $(dist_dir)/libfsemu
	cp -a ../libfsemu/include $(dist_dir)/libfsemu
	cp -a ../libfsemu/src $(dist_dir)/libfsemu
	mkdir -p $(dist_dir)/libfsemu/obj
	mkdir -p $(dist_dir)/libfsemu/out

	mkdir -p $(dist_dir)/libcapsimage
	cp -a ../libcapsimage/Makefile $(dist_dir)/libcapsimage
	cp -a ../libcapsimage/CAPSImage $(dist_dir)/libcapsimage
	find $(dist_dir)/libcapsimage -name *.o -delete
	rm -f $(dist_dir)/libcapsimage/CAPSImage/config.h
	rm -f $(dist_dir)/libcapsimage/CAPSImage/config.log
	rm -f $(dist_dir)/libcapsimage/CAPSImage/config.cache
	rm -f $(dist_dir)/libcapsimage/CAPSImage/config.status
	cp -a ../libcapsimage/*.txt $(dist_dir)/libcapsimage
	mkdir -p $(dist_dir)/libcapsimage/out

	mkdir -p $(dist_dir)/macosx
	cp -a macosx/Makefile $(dist_dir)/macosx/
	cp -a macosx/fix-app.py $(dist_dir)/macosx/
	cp -a macosx/template $(dist_dir)/macosx/

	mkdir -p $(dist_dir)/windows
	cp -a windows/Makefile $(dist_dir)/windows/
	cp -a windows/replace_icon.py $(dist_dir)/windows/
	cp -a windows/fs-uae-setup.iss $(dist_dir)/windows/

	mkdir -p $(dist_dir)/debian
	cp -a debian/changelog $(dist_dir)/debian/
	cp -a debian/compat $(dist_dir)/debian/
	cp -a debian/control $(dist_dir)/debian/
	cp -a debian/copyright $(dist_dir)/debian/
	cp -a debian/rules $(dist_dir)/debian/
	cp -a debian/source $(dist_dir)/debian/

	mkdir -p $(dist_dir)/server
	cp -a server/fs_uae_netplay_server $(dist_dir)/server/
	find $(dist_dir)/server -name "*.pyc" -delete
	cp -a server/README $(dist_dir)/server/
	cp -a server/setup.py $(dist_dir)/server/

	mkdir -p $(dist_dir)/server/debian
	cp -a server/debian/changelog $(dist_dir)/server/debian/
	cp -a server/debian/compat $(dist_dir)/server/debian/
	cp -a server/debian/control $(dist_dir)/server/debian/
	cp -a server/debian/copyright $(dist_dir)/server/debian/
	cp -a server/debian/rules $(dist_dir)/server/debian/
	cp -a server/debian/preinst $(dist_dir)/server/debian/
	cp -a server/debian/source $(dist_dir)/server/debian/
	cp -a server/debian/*.init $(dist_dir)/server/debian/
	cp -a server/debian/*.default $(dist_dir)/server/debian/

	mkdir -p $(dist_dir)/server/scripts
	cp -a server/scripts/fs-uae-netplay-server $(dist_dir)/server/scripts/
	cp -a server/scripts/fs-uae-game-server $(dist_dir)/server/scripts/

	mkdir -p $(dist_dir)/launcher
	cp -a launcher/fs_uae_launcher $(dist_dir)/launcher/
	find $(dist_dir)/launcher -name "*.pyc" -delete
	cp -a launcher/README $(dist_dir)/server/
	cp -a launcher/fs-uae-launcher.py $(dist_dir)/launcher/
	cp -a launcher/setup.py $(dist_dir)/launcher/
	cp -a launcher/setup_py2exe.py $(dist_dir)/launcher/
	cp -a launcher/setup_py2app.py $(dist_dir)/launcher/

	mkdir -p $(dist_dir)/launcher/debian
	cp -a launcher/debian/changelog $(dist_dir)/launcher/debian/
	cp -a launcher/debian/compat $(dist_dir)/launcher/debian/
	cp -a launcher/debian/control $(dist_dir)/launcher/debian/
	cp -a launcher/debian/copyright $(dist_dir)/launcher/debian/
	cp -a launcher/debian/rules $(dist_dir)/launcher/debian/
	cp -a launcher/debian/source $(dist_dir)/launcher/debian/

	mkdir -p $(dist_dir)/launcher/scripts
	cp -a launcher/scripts/fs-uae-launcher $(dist_dir)/launcher/scripts/

	mkdir -p $(dist_dir)/util
	#cp -a util/fix_64_bit.py $(dist_dir)/util/
	#cd $(dist_dir) && python util/fix_64_bit.py
	cp -a util/update_version.py $(dist_dir)/util/
	cd $(dist_dir) && python util/update_version.py

	mkdir -p $(dist_dir)/icon
	cp icon/fs-uae.ico $(dist_dir)/icon/
	cp icon/fs-uae.icns $(dist_dir)/icon/
	cp icon/fs-uae-config.icns $(dist_dir)/icon/
	
	find $(dist_dir) -exec touch \{\} \;

distcheck: distdir
	cd $(dist_dir) && make

dist: distdir pubfiles-source
	tar zcfv fs-uae-$(version).tar.gz $(dist_dir)
	mkdir -p dist/files
	mv fs-uae-$(version).tar.gz dist/files/fs-uae-$(version).tar.gz
	cp doc/Default.fs-uae dist/files/
	cp server/fs_uae_netplay_server/game.py \
		dist/files/fs-uae-netplay-server.py
	cp server/fs_uae_netplay_server/game.py \
		dist/files/fs-uae-game-server-$(version).py
	cp stuff/joyconfig/joyconfig.py dist/files/fs-uae-gamepad-config.py

install:
	mkdir -p $(DESTDIR)$(prefix)/bin
	cp out/fs-uae $(DESTDIR)$(prefix)/bin/fs-uae
	mkdir -p $(DESTDIR)$(prefix)/share
	cp -a share/* $(DESTDIR)$(prefix)/share
	#cp -a share/applications $(DESTDIR)$(prefix)/share/
	#cp -a share/icons $(DESTDIR)$(prefix)/share/

	mkdir -p $(DESTDIR)$(prefix)/share/doc/fs-uae
	cp -a README COPYING example.conf $(DESTDIR)$(prefix)/share/doc/fs-uae
ifeq ($(wildcard libcapsimage), libcapsimage)
ifeq ($(os), windows)
else ifeq ($(os), macosx)
else
	mkdir -p $(DESTDIR)$(prefix)/lib/fs-uae
	cp $(libcapsimage_dir)/CAPSImage/libcapsimage.so.* \
			$(DESTDIR)$(prefix)/lib/fs-uae/libcapsimage.so
endif
endif

clean:
	rm -f gen/build68k gen/genblitter gen/gencpu gen/genlinetoscr
	rm -f obj/* out/*

distclean: clean

include targets.mk
