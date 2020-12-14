# Makefile

meson=$(HOME)/.local/bin/meson
xapian=/usr/include/xapian.h
zstd=/usr/include/zstd.h

.PHONY: all dependencies

all: dependencies
	$(meson) . build
	ninja -C build

dependencies: $(meson) $(xapian) $(zstd)

$(meson):
	pip3 install --user meson

$(xapian):
	sudo apt install libxapian-dev

$(zstd):
	sudo apt install libzstd-dev

# EOF
