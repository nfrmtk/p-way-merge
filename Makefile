.PHONY: format
format:
	find pmerge -name '*pp' -type f | xargs clang-format --style=file:./.clang-format -i
	find include -name '*pp' -type f | xargs clang-format --style=file:./.clang-format -i


compile:
	g++ $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS)) -std=c++20 -fsanitize=address,undefined -Wsign-compare -Wall -Wextra -O2 -g