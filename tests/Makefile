# Host tests for the AmigaOS pieces (compat/): make test
CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O1
test: test_compat
	./test_compat
	sh test-part.sh
	sh test-patches.sh
test_compat: test_compat.cpp ../compat/tarrasch_engine_util.h ../compat/toy_lines.h
	$(CXX) $(CXXFLAGS) -I../compat -o $@ test_compat.cpp
clean:
	rm -f test_compat
.PHONY: test clean
