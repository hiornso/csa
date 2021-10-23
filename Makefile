CC?=cc
CXX?=c++

PKGCONFIG?=pkg-config
UNZIP?=unzip

USE_VCL?=0
USE_LTO?=0
CXXSTD?=c++17

OPT_LEVEL?=3
ifeq ($(USE_LTO),1)
CFLAGS_OPT?=-O$(OPT_LEVEL) -flto
LDFLAGS_OPT?=-O$(OPT_LEVEL) -flto
else
CFLAGS_OPT?=-O$(OPT_LEVEL)
LDFLAGS_OPT?=-O$(OPT_LEVEL)
endif

CFLAGS+=$(CFLAGS_OPT) -mavx2 -mfma
CFLAGS+= -Wall -Wextra -Wpedantic
CXXFLAGS+=$(CFLAGS) -std=$(CXXSTD)

LDFLAGS+=$(LDFLAGS_OPT)
LDLIBS+=-lpthread -lm

GTK_CFLAGS?=`$(PKGCONFIG) --cflags gtk4`
GTK_LDLIBS?=`$(PKGCONFIG) --libs gtk4`

LUAJIT_CFLAGS?=`$(PKGCONFIG) --cflags luajit`
LUAJIT_LDLIBS?=`$(PKGCONFIG) --libs luajit`

OS?=$(shell uname)

OBJDIR?=obj
OSDEPS?=osdeps
DARWINDEPS?=darwindeps
LINUXDEPS?=linuxdeps
MACOS_APP_NAME?=Captain\ Sonar\ Assist.app

DISTRIB_FILENAME:=$(shell date +"csa_%F_%H-%M-%S")
DISTRIB_DIR?=distrib
DO_GZIP_DISTRIB?=1
DO_LRZIP_DISTRIB?=1
ifeq ($(OS),Darwin)
TAR?=gtar
else
TAR?=tar
endif
LRZIP?=lrzip

ifeq ($(OS),Darwin)
MENU=menu_darwin.ui
TARGET=$(MACOS_APP_NAME)
else
MENU=menu.ui
TARGET=csa
endif

.PHONY: default
default: $(TARGET)

.PHONY: all
all: $(TARGET) testbench

.PHONY: install
install: $(TARGET)
ifeq ($(OS),Darwin)
	cp -r $(MACOS_APP_NAME) /Applications/
else ifeq ($(OS),Linux)
	cp csa /usr/local/bin/captain-sonar-assist
	cp $(LINUXDEPS)/captain_sonar_assist.desktop /usr/share/applications/
	cp $(LINUXDEPS)/captain_sonar_assist.svg /usr/share/icons/hicolor/scalable/apps/captain_sonar_assist.svg
	gtk4-update-icon-cache -f /usr/share/icons/hicolor
else
	@echo "Sorry, this Makefile does not support installation on your OS."
endif

.PHONY: uninstall
uninstall:
ifeq ($(OS),Darwin)
	rm -r /Applications/$(MACOS_APP_NAME)
else ifeq ($(OS),Linux)
	rm /usr/local/bin/captain-sonar-assist
	rm /usr/share/applications/captain_sonar_assist.desktop
	rm /usr/share/icons/hicolor/scalable/apps/captain_sonar_assist.svg
	gtk4-update-icon-cache -f /usr/share/icons/hicolor
else
	@echo "This Makefile does not support installation on your OS, so there is nothing to uninstall."
endif
	
$(MACOS_APP_NAME): csa $(DARWINDEPS)/Info.plist $(DARWINDEPS)/AppIcon.icns
	mkdir -p $(MACOS_APP_NAME)/Contents/MacOS/
	mkdir -p $(MACOS_APP_NAME)/Contents/Resources/
	cp $(DARWINDEPS)/Info.plist $(MACOS_APP_NAME)/Contents/
	cp $(DARWINDEPS)/AppIcon.icns $(MACOS_APP_NAME)/Contents/Resources/
	cp csa $(MACOS_APP_NAME)/Contents/MacOS/

csa: $(OBJDIR)/main.o $(OBJDIR)/csa_alloc.o $(OBJDIR)/healthbars.o $(OBJDIR)/engineering.o $(OBJDIR)/firstmate.o $(OBJDIR)/tracker.o $(OBJDIR)/maprender.o $(OBJDIR)/accelerated.o $(OBJDIR)/resources.o $(OBJDIR)/csa_error.o
	$(CC) $(LDFLAGS) -o csa $(OBJDIR)/main.o $(OBJDIR)/csa_alloc.o $(OBJDIR)/healthbars.o $(OBJDIR)/engineering.o $(OBJDIR)/firstmate.o $(OBJDIR)/tracker.o $(OBJDIR)/maprender.o $(OBJDIR)/accelerated.o $(OBJDIR)/resources.o $(OBJDIR)/csa_error.o $(GTK_LDLIBS) $(LUAJIT_LDLIBS) $(LDLIBS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/main.o: $(OBJDIR) main.c main.h csa_alloc.h engineering.h healthbars.h firstmate.h tracker.h csa_error.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(OBJDIR)/main.o main.c

$(OBJDIR)/csa_alloc.o: $(OBJDIR) csa_alloc.c csa_alloc.h
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/csa_alloc.o csa_alloc.c

$(OBJDIR)/csa_error.o: $(OBJDIR) csa_error.c csa_error.h
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/csa_error.o csa_error.c

$(OBJDIR)/accelerated.o: $(OBJDIR) accelerated.cpp accelerated.h main.h csa_alloc.h tracker.h
ifeq ($(USE_VCL),1)
	$(UNZIP) vcl.zip
endif
	$(CXX) $(CXXFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -DUSE_VECTORS=$(USE_VCL) -c -o $(OBJDIR)/accelerated.o accelerated.cpp

$(OBJDIR)/maprender.o: $(OBJDIR) maprender.c tracker.h main.h csa_alloc.h accelerated.h csa_error.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(OBJDIR)/maprender.o maprender.c

$(OBJDIR)/tracker.o: $(OBJDIR) tracker.c tracker.h main.h csa_alloc.h csa_error.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(OBJDIR)/tracker.o tracker.c

$(OBJDIR)/firstmate.o: $(OBJDIR) firstmate.c firstmate.h main.h csa_alloc.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c -o $(OBJDIR)/firstmate.o firstmate.c

$(OBJDIR)/engineering.o: $(OBJDIR) engineering.c engineering.h main.h csa_alloc.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c -o $(OBJDIR)/engineering.o engineering.c

$(OBJDIR)/healthbars.o: $(OBJDIR) healthbars.c healthbars.h main.h csa_alloc.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c -o $(OBJDIR)/healthbars.o healthbars.c

$(OBJDIR)/resources.o: $(OBJDIR) resources.c
	$(CC) $(CFLAGS) -Wno-overlength-strings $(GTK_CFLAGS) -c -o $(OBJDIR)/resources.o resources.c

resources.c: csa.gresource.xml resources/builder/menu.ui resources/* resources/*/* resources/*/*/* resources/*/*/*/* maps/*
	glib-compile-resources --generate-source --target=resources.c csa.gresource.xml

resources/builder/menu.ui: $(OSDEPS)/$(MENU)
	cp $(OSDEPS)/$(MENU) resources/builder/menu.ui

testbench: $(OBJDIR)/testbench.o $(OBJDIR)/csa_alloc.o $(OBJDIR)/tracker.o $(OBJDIR)/maprender.o $(OBJDIR)/accelerated.o $(OBJDIR)/resources.o $(OBJDIR)/csa_error.o
	$(CC) $(LDFLAGS) -o testbench $(OBJDIR)/testbench.o $(OBJDIR)/csa_alloc.o $(OBJDIR)/tracker.o $(OBJDIR)/maprender.o $(OBJDIR)/accelerated.o $(OBJDIR)/resources.o $(OBJDIR)/csa_error.o $(GTK_LDLIBS) $(LUAJIT_LDLIBS) $(LDLIBS)

$(OBJDIR)/testbench.o: $(OBJDIR) testbench.c main.h csa_alloc.h tracker.h accelerated.h csa_error.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(OBJDIR)/testbench.o testbench.c

.PHONY: clean
clean:
	rm -f csa testbench testbench.png resources.c resources/builder/menu.ui $(OBJDIR)/*
	rm -rf vcl
ifeq ($(OS),Darwin)
	rm -rf $(MACOS_APP_NAME)
endif

.PHONY: distrib
distrib: clean
	mkdir -p $(DISTRIB_DIR)
ifeq ($(DO_GZIP_DISTRIB),1)
	$(TAR) --transform "s+^.+$(DISTRIB_FILENAME)+" -czf $(DISTRIB_DIR)/$(DISTRIB_FILENAME).tar.gz --exclude=distrib --exclude=resources/images/engineering/svgs --exclude=resources/images/first-mate/svgs .
endif
ifeq ($(DO_LRZIP_DISTRIB),1)
	$(TAR) --transform "s+^.+$(DISTRIB_FILENAME)+" -cf - --exclude=distrib --exclude=resources/images/engineering/svgs --exclude=resources/images/first-mate/svgs . | $(LRZIP) -q -o $(DISTRIB_DIR)/$(DISTRIB_FILENAME).tar.lrz
endif

.PHONY: clean_distrib
clean_distrib:
	rm -rf $(DISTRIB_DIR)