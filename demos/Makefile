SUBDIRS := imx-camera-rotation-g2d imx-camera-rotation-opencv imx-camera-rotation-opengl

.PHONY: all $(SUBDIRS) clean

# Default target: build everything in subdirectories
all: $(SUBDIRS)

$(SUBDIRS):
	@echo "Entering subdirectory: $@"
	$(MAKE) -C $@


install:
	@echo "Installing binaries..."
	install -d bin
	for dir in $(SUBDIRS); do \
		install $$dir/$$dir bin/; \
	done

clean:
	@echo "Cleaning up in subdirectories..."
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -rf bin
