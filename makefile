# ============================
#  Compiler selection
#  Usage:
#     make            | builds with g++
#     make CC=clang++ | builds with clang++
#  Tracy toggle
#  Usage:
#     make tracy=1    | enable Tracy
#  Sanitize toggle (only for clang++)
#  Usage:
#     make sanitize=1 | enable ThreadSanitizer
# ============================

CXX ?= g++

# ============================
#  Project structure
# ============================

SRC_DIR := src
SRC := $(wildcard $(SRC_DIR)/*.cpp)

MAIN_SRC := $(SRC_DIR)/main.cpp
LIB_SRC := $(filter-out $(MAIN_SRC), $(SRC))

TRACY_DIR := tracy/public
TRACY_SRC := $(TRACY_DIR)/TracyClient.cpp

TARGET := main.exe

ifeq ($(tracy),1)
    TRACY_FLAGS := -DTRACY_ENABLE -I$(TRACY_DIR)
    TRACY_OBJS := $(TRACY_SRC)
else
    TRACY_FLAGS :=
    TRACY_OBJS :=
endif

# ============================
#  Compiler-specific flags
# ============================

CPP_VERSION := -std=c++23
COMMON_FLAGS := 
SANTIZE_FLAGS :=
LDFLAGS := -lpthread

# MinGW-specific libs when Tracy is enabled
ifeq ($(CXX),g++)
    ifeq ($(tracy),1)
        LDFLAGS += -lws2_32 -ldbghelp -g
    endif
endif

# Clang-specific flags
ifeq ($(CXX),clang++)
    ifeq ($(sanitize),1)
        SANTIZE_FLAGS += -fsanitize=thread
    endif
    ifeq ($(tracy),1)
        LDFLAGS += -g
    endif
endif

# ============================
#  GoogleTest
# ============================

GTEST_DIR := third_party/googletest/googletest
GTEST_SRC := $(GTEST_DIR)/src/gtest-all.cc
GTEST_MAIN := $(GTEST_DIR)/src/gtest_main.cc
GTEST_INCLUDE := -I$(GTEST_DIR)/include -I$(GTEST_DIR)

TEST_DIR := $(SRC_DIR)/tests
TEST_SRC := $(wildcard $(TEST_DIR)/*.cpp)

TEST_TARGET := tests.exe
PROJECT_INCLUDE := -I$(SRC_DIR)

# ============================
#  Final build rule
# ============================

all: $(TARGET)

$(TARGET): $(MAIN_SRC) $(LIB_SRC) $(TRACY_OBJS)
	$(CXX) $(CPP_VERSION) $^ -o $@ \
	    $(TRACY_FLAGS) $(COMMON_FLAGS) $(LDFLAGS) $(SANTIZE_FLAGS)

$(TEST_TARGET): $(TEST_SRC) $(LIB_SRC) $(GTEST_SRC) $(GTEST_MAIN)
	$(CXX) $(CPP_VERSION) $^ -o $@ \
	    $(GTEST_INCLUDE) $(PROJECT_INCLUDE) -lpthread

.PHONY: test
test: $(TEST_TARGET)
	./$(TEST_TARGET)
