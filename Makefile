.PHONY: format
format:
	find pmerge -name '*pp' -type f | xargs clang-format --style=file:./.clang-format -i
	find include -name '*pp' -type f | xargs clang-format --style=file:./.clang-format -i
