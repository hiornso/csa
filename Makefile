CC  ?= cc
CXX ?= c++

PKGCONFIG ?= pkg-config

USE_VCL ?= 0
USE_LTO ?= 0
OPENBLAS ?= 1
DEBUGINFO ?= 1

CFLAGS += -MMD -MP

CXXSTD ?= c++17

OPT_LEVEL ?= 3
CFLAGS    += -O$(OPT_LEVEL)
LDFLAGS   += -O$(OPT_LEVEL)

ifeq ($(DEBUGINFO),1)
CFLAGS += -g
endif

ifeq ($(USE_LTO),1)
CFLAGS  += -flto
LDFLAGS += -flto
endif

CFLAGS   += `$(PKGCONFIG) --cflags gtk4 luajit`
CFLAGS   += -march=native
CFLAGS   += -Wall -Wextra -Wpedantic -Wno-overlength-strings
CXXFLAGS += $(CFLAGS) -std=$(CXXSTD)

LDLIBS += `$(PKGCONFIG) --libs gtk4 luajit`
LDLIBS += -lpthread -lm -lgsl

ifeq ($(OPENBLAS),1)
CXXFLAGS += -DOPENBLAS=1
LDLIBS += -lopenblas
else
CXXFLAGS += -DOPENBLAS=0
LDLIBS += -lcblas
endif

BUILD_DIR ?= build

OS ?= $(shell uname)

OSDEPS ?= osdeps
DARWINDEPS ?= darwindeps
LINUXDEPS  ?= linuxdeps
MACOS_APP_NAME ?= Captain\ Sonar\ Assist.app

CSA = csa
TESTBENCH = testbench

OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/testbench.o $(BUILD_DIR)/csa_alloc.o $(BUILD_DIR)/healthbars.o $(BUILD_DIR)/engineering.o $(BUILD_DIR)/firstmate.o $(BUILD_DIR)/tracker.o $(BUILD_DIR)/maprender.o $(BUILD_DIR)/accelerated.o $(BUILD_DIR)/resources.o $(BUILD_DIR)/csa_error.o
CSA_OBJECTS = $(filter-out $(BUILD_DIR)/testbench.o, $(OBJECTS))
TESTBENCH_OBJECTS = $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/healthbars.o $(BUILD_DIR)/engineering.o $(BUILD_DIR)/firstmate.o, $(OBJECTS))

DEPENDS = $(patsubst %.o, %.d, $(OBJECTS))

ifeq ($(OS),Darwin)
MENU = $(OSDEPS)/menu_darwin.ui
TARGET = $(MACOS_APP_NAME)
else
MENU = $(OSDEPS)/menu.ui
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
	cp $(CSA) /usr/local/bin/captain-sonar-assist
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

$(MACOS_APP_NAME): $(CSA) $(DARWINDEPS)/Info.plist $(DARWINDEPS)/AppIcon.icns
	mkdir -p $(MACOS_APP_NAME)/Contents/MacOS/
	mkdir -p $(MACOS_APP_NAME)/Contents/Resources/
	cp $(DARWINDEPS)/Info.plist $(MACOS_APP_NAME)/Contents/
	cp $(DARWINDEPS)/AppIcon.icns $(MACOS_APP_NAME)/Contents/Resources/
	cp $(CSA) $(MACOS_APP_NAME)/Contents/MacOS/


-include $(DEPENDS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CSA): $(CSA_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TESTBENCH): $(TESTBENCH_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/accelerated.o: accelerated.cpp | $(BUILD_DIR)
ifeq ($(USE_VCL),1)
	git submodule update --init
endif
	$(CXX) $(CXXFLAGS) -DUSE_VECTORS=$(USE_VCL) -c -o $@ $<

resources.c: csa.gresource.xml resources/builder/menu.ui resources/* resources/*/* resources/*/*/* resources/*/*/*/* maps/*
	glib-compile-resources --generate-source --target=resources.c csa.gresource.xml

resources/builder/menu.ui: $(MENU)
	cp $(MENU) resources/builder/menu.ui

.PHONY: clean
clean:
	rm -f $(CSA) $(TESTBENCH) testbench.png resources.c resources/builder/menu.ui
	rm -rf $(BUILD_DIR) 
ifeq ($(OS),Darwin)
	rm -rf $(MACOS_APP_NAME)
endif
