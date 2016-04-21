CXXFLAGS ?= -g
CXXFLAGS += -std=c++14 -Wall -Wextra -pedantic -Wno-sized-deallocation -Werror -Wfatal-errors
LDFLAGS += -pthread

SharedPtr_test: SharedPtr_test.cpp SharedPtr.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	$(RM) SharedPtr_test

.PHONY: clean
