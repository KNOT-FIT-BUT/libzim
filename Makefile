# Makefile

meson=$(HOME)/.local/bin/meson
xapian=/usr/include/xapian.h

.PHONY: all

all: $(meson)
	$(meson) . build
	ninja -C build

$(meson):
	pip3 install --user meson

$(xapian):
	sudo apt install libxapian-dev

# EOF
