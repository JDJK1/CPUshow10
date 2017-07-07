# Makefile for Magic Eye linux driver

ccflags-y := -DDEBUG

# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq (${KERNELRELEASE},)
	obj-m := magiceye_driver.o

# Otherwise we were called directly from the command line.
# Invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	${MAKE} -C ${KERNELDIR} SUBDIRS=${PWD} modules

clean:
	${MAKE} -C ${KERNELDIR} SUBDIRS=${PWD} clean
endif
