# ============================
#  Compiler selection
#  Usage:
#     make            | builds with g++
#     make CC=clang++ | builds with clang++
#  Tracy toggle
#  Usage:
#     make tracy=1    | enable Tracy
# ============================

CXX ?= g++

# ============================
#  Project structure
# ============================

SRC_DIR := .
TRACY_DIR := tracy/public

SRC := $(SRC_DIR)/main.cpp
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

COMMON_FLAGS := -std=c++23
LDFLAGS := -lpthread

# MinGW-specific libs when Tracy is enabled
ifeq ($(CXX),g++)
    ifeq ($(tracy),1)
        LDFLAGS += -lws2_32 -ldbghelp -g
    endif
endif

# Clang-specific flags
ifeq ($(CXX),clang++)
    COMMON_FLAGS += -fsanitize=thread
    ifeq ($(tracy),1)
        LDFLAGS += -g
    endif
endif

# ============================
#  Final build rule
# ============================

all: $(TARGET)

$(TARGET): $(SRC) $(TRACY_OBJS)
	$(CXX) $(COMMON_FLAGS) $(TRACY_FLAGS) $(LDFLAGS) $^ -o $@ 