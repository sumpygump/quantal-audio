# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine.
LDFLAGS +=

# Add .cpp and .c files to the build
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin is automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) $(wildcard res/*.svg)

# Include the VCV Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

# Run `make dist` to prepare the distributed files
# Run `make install` to install to local environment

# To run the Makefile in the `res` directory: to generate the optimized
# versions of module panel SVGs
images:
	$(MAKE) -C res

# Run to lint and apply defined codestyle fixes
lint:
	astyle --suffix=none --options=.astylerc -r 'src/*'