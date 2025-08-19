CC ?=clang
SRC_DIR := ./src/
SRC_FILES :=  $(wildcard $(SRC_DIR)*.c)
SRC_INCLUDES :=  $(wildcard $(SRC_DIR)*.h)
BUILD_DIR:=./bin/
EXAMPLES_DIR:=./examples/
EXAMPLES := $(wildcard ${EXAMPLES_DIR}*.c)
EXECUTABLES := $(patsubst ${EXAMPLES_DIR}%.c, ${BUILD_DIR}%, ${EXAMPLES})

all: build_dir $(EXECUTABLES)

./bin/%: ./examples/%.c $(SRC_FILES) $(SRC_INCLUDES)
	@echo "Building: " $@
	@$(CC) $< $(SRC_FILES) -I $(SRC_DIR) -o $@ 


build_dir:  
	@mkdir -p ${BUILD_DIR}

clean: 
	@rm -r ${BUILD_DIR}
.PHONY: all build_dir

