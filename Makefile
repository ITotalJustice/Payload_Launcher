#---------------------------------------------------------------------------------
# A really basic makefile made by me!
# I have no idea what i'm doing, but I somehow got it to work!
# Feel free to point and laugh...
#---------------------------------------------------------------------------------

all:
	$(MAKE) -C nro
	$(MAKE) -C exefs
	
#---------------------------------------------------------------------------------

clean:
	$(MAKE) -C nro clean
	$(MAKE) -C exefs clean
	rm -rf out
	rm -rf out

#---------------------------------------------------------------------------------

dist: all
	rm -rf out/PayloaderNX
	rm -rf out/exefs
	mkdir -p out/PayloaderNX/switch/payloadernx/payloads
	mkdir -p out/exefs/title-examples/0100000000001003
	mkdir -p out/exefs/title-examples/010000000000100B
	mkdir -p out/exefs/title-examples/0100000000001013
	touch out/exefs/title-examples/0100000000001003/controller.deleteme
	touch out/exefs/title-examples/010000000000100B/eshop-applet.deleteme
	touch out/exefs/title-examples/0100000000001013/user-applet
	cp nro/payloadernx.nro out/PayloaderNX/switch/payloadernx
	cp exefs/exefs.nsp out/exefs
	cp exefs/exefs.nsp out/exefs/title-examples/010000000000100B
	cp exefs/exefs.nsp out/exefs/title-examples/0100000000001003
	cp exefs/exefs.nsp out/exefs/title-examples/0100000000001013
	cp README.md out/PayloaderNX
	cp FAQ.md out/PayloaderNX
	cp README.md out/exefs
	cp FAQ.md out/exefs
	cd out/PayloaderNX; zip -r ../../PayloaderNX.zip ./*; cd ../;
	cd out/exefs; zip -r ../../exefs.zip ./*; cd ../;

#---------------------------------------------------------------------------------