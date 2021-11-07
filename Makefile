CC  ?= cc
CXX ?= c++

PKGCONFIG ?= pkg-config

USE_VCL ?= 0
USE_LTO ?= 0

CXXSTD  ?= c++17

OPT_LEVEL ?= 3
CFLAGS    += -O$(OPT_LEVEL)
LDFLAGS   += -O$(OPT_LEVEL)

ifeq ($(USE_LTO),1)
CFLAGS  += -flto
LDFLAGS += -flto
endif

CFLAGS   += `$(PKGCONFIG) --cflags gtk4 luajit`
CFLAGS   += -mavx2 -mfma
CFLAGS   += -Wall -Wextra -Wpedantic
CXXFLAGS += $(CFLAGS) -std=$(CXXSTD)

LDLIBS += `$(PKGCONFIG) --libs gtk4 luajit`
LDLIBS += -lpthread -lm

BUILD_DIR ?= build

OS ?= $(shell uname)

OSDEPS ?= osdeps
DARWINDEPS ?= darwindeps
LINUXDEPS  ?= linuxdeps
MACOS_APP_NAME ?= Captain\ Sonar\ Assist.app

CSA = csa
TESTBENCH = testbench

ifeq ($(OS),Darwin)
MENU = menu_darwin.ui
TARGET = $(MACOS_APP_NAME)
else
MENU = menu.ui
TARGET = $(CSA)
endif

.PHONY: default
default: $(TARGET)

.PHONY: all
all: $(TARGET) $(TESTBENCH)

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


$(CSA): $(BUILD_DIR)/main.o $(BUILD_DIR)/csa_alloc.o $(BUILD_DIR)/healthbars.o $(BUILD_DIR)/engineering.o $(BUILD_DIR)/firstmate.o $(BUILD_DIR)/tracker.o $(BUILD_DIR)/maprender.o $(BUILD_DIR)/accelerated.o $(BUILD_DIR)/resources.o $(BUILD_DIR)/csa_error.o
	$(CC) $(LDFLAGS) -o $(CSA) $(BUILD_DIR)/main.o $(BUILD_DIR)/csa_alloc.o $(BUILD_DIR)/healthbars.o $(BUILD_DIR)/engineering.o $(BUILD_DIR)/firstmate.o $(BUILD_DIR)/tracker.o $(BUILD_DIR)/maprender.o $(BUILD_DIR)/accelerated.o $(BUILD_DIR)/resources.o $(BUILD_DIR)/csa_error.o $(GTK_LDLIBS) $(LUAJIT_LDLIBS) $(LDLIBS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: main.c main.h csa_alloc.h engineering.h healthbars.h firstmate.h tracker.h csa_error.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(BUILD_DIR)/main.o main.c

$(BUILD_DIR)/csa_alloc.o: csa_alloc.c csa_alloc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/csa_alloc.o csa_alloc.c

$(BUILD_DIR)/csa_error.o: csa_error.c csa_error.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/csa_error.o csa_error.c

$(BUILD_DIR)/accelerated.o: accelerated.cpp accelerated.h main.h csa_alloc.h tracker.h | $(BUILD_DIR)
ifeq ($(USE_VCL),1)
	git submodule update --init
endif
	$(CXX) $(CXXFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -DUSE_VECTORS=$(USE_VCL) -c -o $(BUILD_DIR)/accelerated.o accelerated.cpp

$(BUILD_DIR)/maprender.o: maprender.c tracker.h main.h csa_alloc.h accelerated.h csa_error.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(BUILD_DIR)/maprender.o maprender.c

$(BUILD_DIR)/tracker.o: tracker.c tracker.h main.h csa_alloc.h csa_error.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(BUILD_DIR)/tracker.o tracker.c

$(BUILD_DIR)/firstmate.o: firstmate.c firstmate.h main.h csa_alloc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c -o $(BUILD_DIR)/firstmate.o firstmate.c

$(BUILD_DIR)/engineering.o: engineering.c engineering.h main.h csa_alloc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c -o $(BUILD_DIR)/engineering.o engineering.c

$(BUILD_DIR)/healthbars.o: healthbars.c healthbars.h main.h csa_alloc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c -o $(BUILD_DIR)/healthbars.o healthbars.c

$(BUILD_DIR)/resources.o: resources.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Wno-overlength-strings $(GTK_CFLAGS) -c -o $(BUILD_DIR)/resources.o resources.c

resources.c: csa.gresource.xml resources/builder/menu.ui resources/* resources/*/* resources/*/*/* resources/*/*/*/* maps/*
	glib-compile-resources --generate-source --target=resources.c csa.gresource.xml

resources/builder/menu.ui: $(OSDEPS)/$(MENU)
	cp $(OSDEPS)/$(MENU) resources/builder/menu.ui

$(TESTBENCH): $(BUILD_DIR)/testbench.o $(BUILD_DIR)/csa_alloc.o $(BUILD_DIR)/tracker.o $(BUILD_DIR)/maprender.o $(BUILD_DIR)/accelerated.o $(BUILD_DIR)/resources.o $(BUILD_DIR)/csa_error.o
	$(CC) $(LDFLAGS) -o $(TESTBENCH) $(BUILD_DIR)/testbench.o $(BUILD_DIR)/csa_alloc.o $(BUILD_DIR)/tracker.o $(BUILD_DIR)/maprender.o $(BUILD_DIR)/accelerated.o $(BUILD_DIR)/resources.o $(BUILD_DIR)/csa_error.o $(GTK_LDLIBS) $(LUAJIT_LDLIBS) $(LDLIBS)

$(BUILD_DIR)/testbench.o: testbench.c main.h csa_alloc.h tracker.h accelerated.h csa_error.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(LUAJIT_CFLAGS) -c -o $(BUILD_DIR)/testbench.o testbench.c

.PHONY: clean
clean:
	rm -f $(CSA) $(TESTBENCH) testbench.png resources.c resources/builder/menu.ui $(BUILD_DIR)/*
ifeq ($(OS),Darwin)
	rm -rf $(MACOS_APP_NAME)
endif
