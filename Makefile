DIR=freelunch

# Build directives

release:
	make -C $(DIR) CONF=linux-x86_64-normal-server-release

fastdebug:
	make -C $(DIR) CONF=linux-x86_64-normal-server-fastdebug

slowdebug:
	make -C $(DIR) CONF=linux-x86_64-normal-server-slowdebug

all:
	make release fastdebug slowdebug


# Configure directives

config-release:
	cd $(DIR) && bash configure --with-debug-level=release    # --disable-zip-debug-info

config-fastdebug:
	cd $(DIR) && bash configure --with-debug-level=fastdebug  # --disable-zip-debug-info

config-slowdebug:
	cd $(DIR) && bash configure --with-debug-level=slowdebug  # --disable-zip-debug-info

configureall:
	make config-release config-slowdebug config-fastdebug


# Export binaries (release only)

image:
	make -C $(DIR) images
	ln -s freelunch/build/linux-x86_64-normal-server-release/images/j2re-image freelunch-jre
	tar czfh freelunch-jre.tar.gz freelunch-jre
	rm freelunch-jre
#	tar czf freelunch-sdk.tar.gz freelunch/build/linux-x86_64-normal-server-release/images/j2sdk-image


# Cleaning

clean:
	make -C $(DIR) CONF=linux-x86_64-normal-server-release clean

cleanall:
	make -C $(DIR) CONF=linux-x86_64-normal-server-release clean
	make -C $(DIR) CONF=linux-x86_64-normal-server-fastdebug clean
	make -C $(DIR) CONF=linux-x86_64-normal-server-slowdebug clean
