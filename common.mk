uname := $(shell uname -a)

android := 0

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

debug := 0
optimize := 1
prefix := /usr
