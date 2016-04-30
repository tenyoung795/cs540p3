EXECUTABLES := SharedPtr_test Interpolate_test Function_test
CXXFLAGS ?= -g
CXXFLAGS += -std=c++14 -Wall -Wextra -pedantic -Wno-sized-deallocation -Werror -Wfatal-errors

all: $(EXECUTABLES)

SharedPtr_test: LDFLAGS += -pthread
SharedPtr_test: SharedPtr_test.cpp SharedPtr.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

Interpolate_test: Interpolate_test.cpp Interpolate.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

Function_test: Function_test.cpp Function.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	$(RM) $(EXECUTABLES)

.PHONY: all clean
