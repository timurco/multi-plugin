# ------------------------------------------------------------
# Convenience Makefile for MultiPlugin workflows
# ------------------------------------------------------------

BUILD_DIR ?= build
CONFIG ?= RelWithDebInfo
JOBS ?= 8

CMAKE ?= cmake

CACHE_FILE := $(BUILD_DIR)/CMakeCache.txt

.PHONY: help configure build install clean distclean debug release relwithdebinfo

help:
	@echo "MultiPlugin shortcuts"
	@echo "  make build              - configure (if needed) and build ($(CONFIG))"
	@echo "  make install            - build then install to host plug-in folders"
	@echo "  make debug              - build with CONFIG=Debug"
	@echo "  make release            - build with CONFIG=Release"
	@echo "  make relwithdebinfo     - build with CONFIG=RelWithDebInfo"
	@echo "  make clean              - clean build artifacts in $(BUILD_DIR)"
	@echo "  make distclean          - remove $(BUILD_DIR)"
	@echo ""
	@echo "Override paths on the command line, e.g.:"
	@echo "  make build AE_SDK_PATH=/custom/ae OFX_PATH=/custom/ofx CONFIG=Release"

configure: $(CACHE_FILE)

$(CACHE_FILE):
	@if [ ! -d "$(AE_SDK_PATH)" ]; then \
		echo "AE_SDK_PATH not found: $(AE_SDK_PATH)"; \
		exit 1; \
	fi
	@if [ ! -d "$(OFX_PATH)" ]; then \
		echo "OFX_PATH not found: $(OFX_PATH)"; \
		exit 1; \
	fi
	$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(CONFIG) \
		-DAE_SDK_PATH="$(AE_SDK_PATH)" \
		-DOFX_PATH="$(OFX_PATH)"

build: configure
	$(CMAKE) --build $(BUILD_DIR) --config $(CONFIG) -- -j$(JOBS)

install: build
	$(CMAKE) --install $(BUILD_DIR) --config $(CONFIG)

clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		$(CMAKE) --build $(BUILD_DIR) --config $(CONFIG) --target clean; \
	fi

distclean:
	rm -rf "$(BUILD_DIR)"

debug:
	$(MAKE) build CONFIG=Debug

release:
	$(MAKE) build CONFIG=Release

relwithdebinfo:
	$(MAKE) build CONFIG=RelWithDebInfo
