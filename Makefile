# Makefile

meson=$(HOME)/.local/bin/meson

.PHONY: all

all: $(meson)
	$(meson) . build
	ninja -C build

$(meson):
	pip3 install --user meson==0.43.0

# EOF
