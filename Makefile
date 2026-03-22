EXE ?= punch

all:
	meson setup build --buildtype=release
	meson compile -C build
	cp build/punch $(EXE)

test:
	meson setup build --buildtype=debugoptimized
	meson test -C build
