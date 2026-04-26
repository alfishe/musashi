# Makefile for Musashi GoogleTests

MUSASHI_ROOT = ../..
GTEST_DIR = .

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -I$(MUSASHI_ROOT) -I$(GTEST_DIR)/framework
LDFLAGS = -lgtest -lgtest_main -lpthread -lm

# Musashi Core Objects (assuming they are built in the root)
# If not, we could build them here, but typically they'd be shared.
MUSASHI_OBJS = $(MUSASHI_ROOT)/m68kcpu.o \
               $(MUSASHI_ROOT)/m68kdasm.o \
               $(MUSASHI_ROOT)/m68kops.o \
               $(MUSASHI_ROOT)/softfloat/softfloat.o

# GTest Source Files
TEST_SRCS = $(GTEST_DIR)/gtest_main.cpp \
            $(GTEST_DIR)/framework/m68k_fixture.cpp \
            $(GTEST_DIR)/instructions/bcd/bcd_tests.cpp

TEST_OBJS = $(TEST_SRCS:.cpp=.o)

TARGET = musashi_unittests

all: $(TARGET)

$(TARGET): $(TEST_OBJS) $(MUSASHI_OBJS)
	$(CXX) -o $@ $(TEST_OBJS) $(MUSASHI_OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TEST_OBJS) $(TARGET)

.PHONY: all clean
