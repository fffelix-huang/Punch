EXE ?= punch

all:
	meson setup build --buildtype=release
	meson compile -C build
	cp build/punch $(EXE)
