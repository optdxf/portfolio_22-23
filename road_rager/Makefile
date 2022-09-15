CLIENT_LIBS = text_label collision forces map base64 sha1 utf8 server client packet global_constants shapes scene vector img_label ui_group ui_handler easing list sdl_wrapper2 body car polygon color
SERVER_LIBS = scene forces map vector shapes polygon server body car collision client handshake packet ws base64 color easing list queue sha1 utf8 global_constants 

FILE_SERVING_PORT = 9354
WEBSOCKET_PORT = 9355

#FILE_SERVING_PORT = $(shell cs3-port)
#WEBSOCKET_PORT = $(shell echo $$(( $(FILE_SERVING_PORT) + 1 )))

# find <dir> is the command to find files in a directory
# ! -name .gitignore tells find to ignore the .gitignore
# -type f only finds files
# -delete deletes all the files found
CLEAN_COMMAND = find out/ ! -name .gitignore ! -name client_v2.html -type f -delete && \
find bin/ ! -name .gitignore ! -name client_v2.html -type f -delete

# Use clang as the C compiler
CC = clang

# Flags to pass to clang:
# -Iinclude tells clang to look for #include files in the "include" folder
# -Wall turns on all warnings
# -g adds filenames and line numbers to the executable for useful stack traces
# -fno-omit-frame-pointer allows stack traces to be generated
#   (take CS 24 for a full explanation)
#-I/home/dominicph/.emscripten_cache/sysroot/include/
CFLAGS += -Inetwork -Igeometry -Iui -Icore -Iphysics -Iutilities $(shell sdl2-config --cflags) -Wall -g -fno-omit-frame-pointer

# Compiling with asan
ifdef ASAN
  CFLAGS += -fsanitize=address
  ifeq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND))
    $(shell touch .debug)
  endif
# Compiling without asan
else
  C_OPT_FLAG = -Ofast
  EM_OPT_FLAG = -O3
  ifneq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND))
    $(shell rm -f .debug)
  endif
endif

# Emscripten compilation section
# Flags to pass to emcc:
# -s EXIT_RUNTIME=1 shuts the program down properly
# -s ALLOW_MEMORY_GROWTH=1 allows for dynamic memory usage
# -s INITIAL_MEMORY sets the initial amount of memory
# -s USE_SDL=2 ports the sdl library.
# Other SDL ports are also included, like image and mixer
# -s ASSERTIONS=1 enables runtime checks for allocation errors
# -O2 specifies to compile the code with full optimizations. When debugging, you can change to O0 to step through code
# -g enables DWARF support, for debugging purposes
# -gsource-map --source-map-base http://localhost:8000/bin/ creates a source map from the C file for debugging
EMCC = emcc
EMCC_FLAGS = -s EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=655360000 -lwebsocket.js -s USE_SDL=2 -s USE_SDL_GFX=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2 -s ASSERTIONS=1 -g -gsource-map --source-map-base http://labradoodle.caltech.edu:$(FILE_SERVING_PORT)/bin/ --preload-file assets --use-preload-plugins

# Add preprocessor definitions for ports
CFLAGS += -D FILE_SERVING_PORT_STR="\"$(FILE_SERVING_PORT)\"" -D WEBSOCKET_PORT=$(WEBSOCKET_PORT) -D WEBSOCKET_PORT_STR="\"$(WEBSOCKET_PORT)\""
EMCC_FLAGS += -D FILE_SERVING_PORT_STR="\"$(FILE_SERVING_PORT)\"" -D WEBSOCKET_PORT=$(WEBSOCKET_PORT) -D WEBSOCKET_PORT_STR="\"$(WEBSOCKET_PORT)\""

# Compiler flag that links the program with the math library
LIB_MATH = -lm
# Compiler flags that link the program with the math library
# Note that $(...) substitutes a variable's value, so this line is equivalent to
LIBS = $(LIB_MATH) $(shell sdl2-config --libs) -lSDL2_gfx

# List of compiled .o files corresponding to CUSTOM_LIBS, e.g. "out/vector.o".
# Don't worry about the syntax; it's just adding "out/" to the start
# and ".o" to the end of each value in STUDENT_LIBS.
SERVER_OBJS = $(addprefix out/,$(SERVER_LIBS:=.o))
# List of compiled wasm.o files corresponding to STUDENT_LIBS
# Similarly to above, we add .wasm.o to the end of each value in STUDENT_LIBS
WASM_CLIENT_OBJS = $(addprefix out/,$(CLIENT_LIBS:=.wasm.o))
# WASM_SERVER_OBJS = $(addprefix out/,$(SERVER_LIBS:=.wasm.o))

# The first Make rule. It is relatively simple
# It builds the files in TEST_BINS and DEMO_BINS, as well as making the server for the demos
# "To build 'all', make sure all files in TEST_BINS and DEMO_BINS are up to date."
# You can execute this rule by running the command "make all", or just "make".
all: bin/server bin/client.js server

# Make the python server for your demos
# To run this, type 'make server'
# TEMPORARY; will be changed later
server: bin/server
	@echo "Go to \033[0;32mhttp://labradoodle.caltech.edu:$(FILE_SERVING_PORT)/bin/\033[0m to access the client." && \
	bin/server

# Any .o file in "out" is built from the corresponding C file.
# Although .c files can be directly compiled into an executable, first building
# .o files reduces the amount of work needed to rebuild the executable.
# For example, if only list.c was modified since the last build, only list.o
# gets recompiled, and clang reuses the other .o files to build the executable.
#
# "%" means "any string".
# Unlike "all", this target has a build command.
# "$^" is a special variable meaning "the source files"
# and $@ means "the target file", so the command tells clang
# to compile the source C file into the target .o file.
#out/%.o: server_library/%.c # source file may be found in "library"
#	$(CC) -c $(CFLAGS) $^ -o $@
out/%.o: core/%.c
	$(CC) -c $(C_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.o: physics/%.c
	$(CC) -c $(C_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.o: ui/%.c
	$(CC) -c $(C_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.o: utilities/%.c
	$(CC) -c $(C_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.o: geometry/%.c
	$(CC) -c $(C_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.o: network/%.c
	$(CC) -c $(C_OPT_FLAG) $(CFLAGS) $^ -o $@

# Emscripten compilation flags
# This is very similar to the above compilation, except for emscripten
out/%.wasm.o: core/%.c
	$(EMCC) -c $(EM_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.wasm.o: physics/%.c
	$(EMCC) -c $(EM_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.wasm.o: ui/%.c
	$(EMCC) -c $(EM_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.wasm.o: utilities/%.c
	$(EMCC) -c $(EM_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.wasm.o: geometry/%.c
	$(EMCC) -c $(EM_OPT_FLAG) $(CFLAGS) $^ -o $@
out/%.wasm.o: network/%.c
	$(EMCC) -c $(EM_OPT_FLAG) $(CFLAGS) $^ -o $@

# Builds bin/%.html by linking the necessary .wasm.o files.
# Unlike the out/%.wasm.o rule, this uses the LIBS flags and omits the -c flag,
# since it is building a full executable. Also notice it uses our EMCC_FLAGS
bin/client.js: out/client_launcher.wasm.o $(WASM_CLIENT_OBJS)
	$(EMCC) $(EM_OPT_FLAG) $(EMCC_FLAGS) $(CFLAGS) $(LIBS)  $^ -o $@

bin/server: out/server_launcher.o $(SERVER_OBJS)
	$(CC) $(C_OPT_FLAG) $(CFLAGS) $(LIB_MATH) -pthreads $^ -o $@

# Removes all compiled files.
clean:
	$(CLEAN_COMMAND)

# This special rule tells Make that "all", "clean", and "test" are rules
# that don't build a file.
.PHONY: all clean test
# Tells Make not to delete the .o files after the executable is built
.PRECIOUS: out/%.o
# Tells Make not to delete the wasm.o files after the executable is built
.PRECIOUS: out/%.wasm.o
