include ../common.mk
version = $(strip $(shell cat ../VERSION))

bindist: program
	cp -a ../licenses fs-uae-$(version)-windows/
	cp -a ../README fs-uae-$(version)-windows/README.txt
	cp -a ../COPYING fs-uae-$(version)-windows/COPYING.txt
	cp -a ../example.conf fs-uae-$(version)-windows
	zip -r fs-uae-$(version)-windows.zip fs-uae-$(version)-windows

program:
	make -C ..
	rm -Rf fs-uae-$(version)-windows
	mkdir fs-uae-$(version)-windows
	cp /mingw/bin/zlib1.dll fs-uae-$(version)-windows
	cp /mingw/bin/libglib-2.0-0.dll fs-uae-$(version)-windows
	cp /mingw/bin/libgthread-2.0-0.dll fs-uae-$(version)-windows
	cp /mingw/bin/intl.dll fs-uae-$(version)-windows
	cp /mingw/bin/libpng*.dll fs-uae-$(version)-windows
	cp /mingw/bin/libgcc_s_*.dll fs-uae-$(version)-windows
	cp /mingw/bin/libstdc++*.dll fs-uae-$(version)-windows
	cp /mingw/bin/SDL.dll fs-uae-$(version)-windows
	#cp /mingw/dist/SDL.dll fs-uae-$(version)-windows
	cp -a ../out/*.dll fs-uae-$(version)-windows
	cp ../out/fs-uae.exe fs-uae-$(version)-windows/FS-UAE.exe
	#mv fs-uae-$(version)-windows/TEMP.exe fs-uae-$(version)-windows/FS-UAE.exe
	cp -a ../share fs-uae-$(version)-windows
	strip fs-uae-$(version)-windows/*.exe
	strip fs-uae-$(version)-windows/*.dll
	#python replace_icon.py fs-uae-$(version)-windows/FS-UAE.exe ../icon/fs-uae.ico

launcher:
	rm -Rf ../launcher/dist
	cd ../launcher && python setup_py2exe.py py2exe
	rm -Rf fs-uae-$(version)-windows/launcher
	mv ../launcher/dist fs-uae-$(version)-windows/launcher
	mv fs-uae-$(version)-windows/launcher/fs-uae-launcher.exe "fs-uae-$(version)-windows/launcher/FS-UAE Launcher.exe"
	python replace_icon.py "fs-uae-$(version)-windows/launcher/FS-UAE Launcher.exe" ../icon/fs-uae.ico

bindist-plus:
	zip -r fs-uae-plus-$(version)-windows.zip fs-uae-$(version)-windows

setup:
	iscc fs-uae-setup.iss
