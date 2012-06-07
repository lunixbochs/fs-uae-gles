uname := $(shell uname -a)
ifneq ($(findstring Msys,$(uname)),)
    os = windows
else ifneq ($(findstring Darwin,$(uname)),)
    os = macosx
else
    os = linux
endif

debug := 0

warnings = -Wall
errors = -Werror=implicit-function-declaration
cppflags =
cflags = -std=c99 $(warnings) $(errors) -Iinclude \
		`sdl-config --cflags` `pkg-config --cflags glib-2.0`
objects = obj/emu_emu.o obj/emu_video.o obj/emu_audio.o obj/emu_input.o \
		obj/emu_menu.o obj/emu_texture.o obj/emu_font.o \
		obj/emu_render.o obj/emu_dialog.o obj/emu_hud.o obj/emu_actions.o \
		obj/emu_shader.o obj/emu_util.o obj/emu_netplay.o \
		obj/emu_scanlines.o\
		\
		obj/ml_ml.o obj/ml_windows.o obj/ml_linux.o obj/ml_render.o \
		obj/ml_sdl.o obj/ml_rawinput.o obj/ml_macosx.o \
		\
		obj/base.o obj/log.o obj/config.o obj/image.o obj/ref.o \
		obj/glee.o obj/opengl.o obj/thread.o obj/string.o obj/filesys.o \
		obj/unicode.o obj/init.o obj/random.o obj/list.o obj/eventlog.o

ldflags =
libs =

ifeq ($(debug), 1)
	cflags += -g -O0 -fno-inline
else
	cflags += -g -O2
endif

ifeq ($(os), windows)
	cppflags += -DWINDOWS -DWIN32
	cppflags += -DWITH_SDL_VIDEO -DWITH_SDL_INPUT -DWITH_OPENAL
	library = libfsemu.a
else ifeq ($(os), macosx)
	#cflags += -arch i386 -arch x86_64
	#ldflags += -arch i386 -arch x86_64
	cflags += -arch i386
	ldflags += -arch i386
	cppflags += -DMACOSX
	cppflags += -I/System/Library/Frameworks/OpenAL.framework/Headers
	cppflags += -DWITH_SDL_VIDEO -DWITH_SDL_INPUT -DWITH_OPENAL
	library = libfsemu.a
else
	cppflags += -DLINUX
	cppflags += -DWITH_SDL_VIDEO -DWITH_SDL_INPUT -DWITH_OPENAL
	library = libfsemu.a
	shared = libfsemu.so.1.0
	shared_soname = libfsemu.so.1
	shared_linkname = libfsemu.so
	shared_ldflags += -shared -Wl,--as-needed -Wl,-soname=$(library_soname)
	#libs += `sdl-config --libs` `pkg-config --libs glib-2.0` -lGL
endif

all: libfsemu

libfsemu: out/$(library)

obj/%.o: src/%.c
	gcc $(cppflags) $(cflags) -c $< -o $@

obj/emu_%.o: src/emu/%.c
	gcc $(cppflags) $(cflags) -c $< -o $@

obj/ml_%.o: src/ml/%.c
	gcc $(cppflags) $(cflags) -c $< -o $@

out/$(library): $(objects)
ifeq ($(os), macosx)
	rm -f $@
endif
	ar cru $@ $(objects)
ifeq ($(os), macosx)
	ranlib $@
endif

out/$(shared): $(objects)
	gcc $(shared_ldflags) $(objects) $(libs) -o $@

fs-test:
	make -C test

clean:
	rm -f obj/*.o
	rm -f out/*.a
	rm -f out/*.so*
	make -C test clean

prefix := /usr

install:
	mkdir -p $(DESTDIR)$(prefix)/include
	cp -a include/* $(DESTDIR)$(prefix)/include/
	mkdir -p $(DESTDIR)$(prefix)/lib
	cp -a out/$(shared) $(DESTDIR)$(prefix)/lib/
	cp -a out/$(library) $(DESTDIR)$(prefix)/lib/
	ln -s $(shared) $(DESTDIR)$(prefix)/lib/$(shared_linkname)
	mkdir -p $(DESTDIR)$(prefix)/lib/pkgconfig/
	cp -a *.pc $(DESTDIR)$(prefix)/lib/pkgconfig/

