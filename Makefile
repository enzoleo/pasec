CXX := /usr/bin/g++

CXXFLAGS := -g -fPIC -std=c++0x -O2 -Wall
CXXFLAGS += -I./
OBJ_DIR := obj
SRCS := $(wildcard *.cpp)
OBJS := $(addprefix $(OBJ_DIR)/, ${SRCS:.cpp=.o})
EXEC := query

.PHONY: all
all:$(OBJ_DIR) $(EXEC)

$(OBJ_DIR):
	@ mkdir -p $@;

$(OBJ_DIR)/%.o:%.cpp
	@ $(CXX) $< -c -o $@ $(CXXFLAGS); \
	echo "CXX "$^;

$(EXEC):$(OBJS)
	@ $(CXX) $(OBJS) -o $(EXEC) $(CXXFLAGS); \
	echo "EXEC "$@;

clean:
	@rm -rf $(OBJ_DIR) $(EXEC)


