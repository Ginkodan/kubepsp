TARGET = $(BUILD_DIR)/kubepsp
BUILD_DIR = build
OBJS = $(BUILD_DIR)/main.o

# Compiler and flags
CFLAGS = -O0 -Wall -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

# PSP metadata
PSP_TITLE = kubepsp
PSP_APP_VER = 1.0
PSP_CATEGORY = MG

# Output files
EXTRA_TARGETS = $(BUILD_DIR)/EBOOT.PBP
PSP_EBOOT = $(BUILD_DIR)/EBOOT.PBP

# SDK path
PSPSDK = $(shell psp-config --pspsdk-path)

# Pattern rules: Compile to build/
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Include the PSP SDK build system
include $(PSPSDK)/lib/build.mak

# Post-build step to move PARAM.SFO into build/
postbuild:
	@mkdir -p $(BUILD_DIR)
	@if [ -f PARAM.SFO ]; then mv PARAM.SFO $(BUILD_DIR)/; fi

# Override default 'all' to include postbuild step
all: $(BUILD_DIR)/EBOOT.PBP postbuild

# Clean target
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) PARAM.SFO
