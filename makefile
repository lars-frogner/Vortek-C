#
# Usage:
# make <argument 1> <argument 2> ...
#
# Arguments:
# <none>:  Compiles with no compiler flags.
# debug:   Compiles with flags useful for debugging.
# fast:    Compiles with flags for high performance.
# clean:   Deletes auxiliary files.
#
# To compile with additional flags, add the argument
# EXTRA_FLAGS="<flags>"

# Define variables
SRC_PATH = ./src
INCLUDE_PATH = ./include
OBJ_PATH = ./obj
BUILD_PATH = ./build

GLFW_PATH = /usr/local/Cellar/glfw/3.2.1

COMPILER = cc

EXECUTABLE_NAME = main.x
EXECUTABLE_LINK = ./${EXECUTABLE_NAME}
EXECUTABLE = ${BUILD_PATH}/${EXECUTABLE_NAME}
OBJECT_FILES = ${OBJ_PATH}/main.o ${OBJ_PATH}/window.o ${OBJ_PATH}/renderer.o ${OBJ_PATH}/trackball.o ${OBJ_PATH}/transform.o ${OBJ_PATH}/utils.o ${OBJ_PATH}/io.o ${OBJ_PATH}/extra_math.o ${OBJ_PATH}/error.o

COMPILATION_FLAGS = -Wno-deprecated-declarations
LINKING_FLAGS = -framework OpenGL

HEADER_PATH_FLAGS = -I${INCLUDE_PATH} -I${GLFW_PATH}/include

LIBRARY_PATH_FLAGS = -L${GLFW_PATH}/lib
LIBRARY_LINKING_FLAGS = -lglfw

DEBUGGING_COMPILE_FLAGS = -Og -W -Wall -fno-common -Wcast-align -Wredundant-decls -Wbad-function-cast -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wextra -Wconversion -pedantic -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -Wno-unused-parameter
DEBUGGING_LINK_FLAGS = -fsanitize=address

PERFORMANCE_COMPILE_FLAGS = -O3 -ffast-math

# Make sure certain rules are not activated by the presence of files
.PHONY: all debug fast set_debug_flags set_fast_flags clean

# Define default target group
all: $(EXECUTABLE_LINK)

# Define optional target groups
debug: set_debug_flags $(EXECUTABLE)
fast: set_fast_flags $(EXECUTABLE)

# Defines appropriate compiler flags for debugging
set_debug_flags:
	$(eval COMPILATION_FLAGS = $(COMPILATION_FLAGS) $(DEBUGGING_COMPILE_FLAGS))
	$(eval LINKING_FLAGS = $(LINKING_FLAGS) $(DEBUGGING_LINK_FLAGS))

# Defines appropriate compiler flags for high performance
set_fast_flags:
	$(eval COMPILATION_FLAGS = $(COMPILATION_FLAGS) $(PERFORMANCE_COMPILE_FLAGS))

$(EXECUTABLE_LINK): $(EXECUTABLE)
	ln -sf $(EXECUTABLE) ${EXECUTABLE_LINK}

# Rule for linking object files
$(EXECUTABLE): $(OBJECT_FILES)
	$(COMPILER) $(EXTRA_FLAGS) $(LINKING_FLAGS) $(OBJECT_FILES) $(LIBRARY_PATH_FLAGS) $(LIBRARY_LINKING_FLAGS) -o $(EXECUTABLE)

# Rule for compiling main.c
${OBJ_PATH}/main.o: ${SRC_PATH}/main.c ${INCLUDE_PATH}/window.h ${INCLUDE_PATH}/renderer.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/main.c -o ${OBJ_PATH}/main.o

# Rule for compiling window.c
${OBJ_PATH}/window.o: ${INCLUDE_PATH}/window.h ${SRC_PATH}/window.c ${INCLUDE_PATH}/renderer.h ${INCLUDE_PATH}/trackball.h ${INCLUDE_PATH}/error.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/window.c -o ${OBJ_PATH}/window.o

# Rule for compiling renderer.c
${OBJ_PATH}/renderer.o: ${INCLUDE_PATH}/renderer.h ${SRC_PATH}/renderer.c ${INCLUDE_PATH}/gl_includes.h ${INCLUDE_PATH}/transform.h ${INCLUDE_PATH}/utils.h ${INCLUDE_PATH}/io.h ${INCLUDE_PATH}/extra_math.h ${INCLUDE_PATH}/error.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/renderer.c -o ${OBJ_PATH}/renderer.o

# Rule for compiling trackball.c
${OBJ_PATH}/trackball.o: ${INCLUDE_PATH}/trackball.h ${SRC_PATH}/trackball.c ${INCLUDE_PATH}/renderer.h ${INCLUDE_PATH}/transform.h ${INCLUDE_PATH}/error.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/trackball.c -o ${OBJ_PATH}/trackball.o

# Rule for compiling transform.c
${OBJ_PATH}/transform.o: ${INCLUDE_PATH}/transform.h ${SRC_PATH}/transform.c ${INCLUDE_PATH}/extra_math.h ${INCLUDE_PATH}/error.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/transform.c -o ${OBJ_PATH}/transform.o

# Rule for compiling utils.c
${OBJ_PATH}/utils.o: ${INCLUDE_PATH}/utils.h ${SRC_PATH}/utils.c ${INCLUDE_PATH}/gl_includes.h ${INCLUDE_PATH}/io.h ${INCLUDE_PATH}/error.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/utils.c -o ${OBJ_PATH}/utils.o

# Rule for compiling io.c
${OBJ_PATH}/io.o: ${INCLUDE_PATH}/io.h ${SRC_PATH}/io.c ${INCLUDE_PATH}/utils.h ${INCLUDE_PATH}/error.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/io.c -o ${OBJ_PATH}/io.o

# Rule for compiling extra_math.c
${OBJ_PATH}/extra_math.o: ${INCLUDE_PATH}/extra_math.h ${SRC_PATH}/extra_math.c ${INCLUDE_PATH}/error.h
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/extra_math.c -o ${OBJ_PATH}/extra_math.o

# Rule for compiling error.c
${OBJ_PATH}/error.o: ${INCLUDE_PATH}/error.h ${SRC_PATH}/error.c
	$(COMPILER) -c $(EXTRA_FLAGS) $(COMPILATION_FLAGS) $(HEADER_PATH_FLAGS) ${SRC_PATH}/error.c -o ${OBJ_PATH}/error.o

# Action for removing all auxiliary files
clean:
	rm -f $(OBJECT_FILES)
