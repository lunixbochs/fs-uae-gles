uname := $(shell uname -a)

android := 0
pandora := 1

ifeq ($(pandora), 1)
os = pandora
cc = arm-angstrom-linux-gnueabi-gcc
cxx = arm-angstrom-linux-gnueabi-g++
ar = arm-angstrom-linux-gnueabi-ar
else
ifeq ($(android), 1)
os = android
cc = arm-linux-androideabi-gcc
cxx = arm-linux-androideabi-g++
ar = arm-linux-androideabi-ar
else
ifneq ($(findstring Msys,$(uname)),)
os = windows
cc = gcc
cxx = g++
ar = ar
else ifneq ($(findstring Darwin,$(uname)),)
os = macosx
cc = gcc
cxx = g++
ar = ar
else
os = linux
cc = gcc
cxx = g++
ar = ar
endif
endif
endif

debug := 0
optimize := 1
prefix := /usr
