################################################################################
# Declare some Makefile variables
################################################################################
CC = clang -g -O0
LANG_STD = -std=c11 -fms-extensions 
COMPILER_FLAGS = -Wall -Wfatal-errors -Wno-gnu -Wno-microsoft
INCLUDE_PATH = -I"./libs/"
SRC_FILES = ./src/*.c ./src/containers/*.c ./src/renderer/*.c ./src/math/*.c \
./src/asset_store/*.c ./src/memory/*.c ./src/game/*.c ./src/cJSON/*.c 
LINKER_FLAGS = -lSDL2 -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lm
OBJ_NAME = gameengine

################################################################################
# Declare some Makefile rules
################################################################################
build:
	$(CC) $(COMPILER_FLAGS) $(LANG_STD) $(INCLUDE_PATH) $(SRC_FILES) $(LINKER_FLAGS) -o $(OBJ_NAME)

run:
	./$(OBJ_NAME)

clean:
	rm $(OBJ_NAME)