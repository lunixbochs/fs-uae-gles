uname := $(shell uname -a)
ifneq ($(findstring Msys,$(uname)),)
os = windows
else ifneq ($(findstring Darwin,$(uname)),)
os = macosx
else
os = linux
endif

all:
	cd CAPSImage && ./configure
	make -C CAPSImage
ifeq ($(os), windows)
	mkdir -p out
	mv CAPSImage/libcapsimage.so.4.2 out/CAPSImg.dll
endif

