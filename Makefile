all:
	@python3 challenger.py ${CHALL} --no-gdb --fail-fast --no-cleanup
fj: forj.c vect.c
	@gcc forj.c -g -o fj  -Werror -D_DEFAULT_SOURCE -D_DEFAULT_SOURCE -I. -I/home/iron/projects/forj/raylib/src -I/home/iron/projects/forj/raylib/src/external  -I/usr/local/include -I/home/iron/projects/forj/raylib/src/external/glfw/include -L. -L/home/iron/projects/forj/raylib/src -L/home/iron/projects/forj/raylib/src -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -latomic -DPLATFORM_DESKTOP -DPLATFORM_DESKTOP_GLFW
int: forj.c vect.c
	@gcc forj.c -DINTERACTIVE -o fj && fj
allval: fj
	@python3 challenger.py ${CHALL} --no-gdb --fail-fast --no-cleanup --valgrind
val: fj
	@valgrind --errors-for-leak-kinds=all --error-exitcode=1 --leak-check=full --show-leak-kinds=all ./fj 2> val.log || \
	if [ $$? -ne 0 ]; then \
		echo "\033[31;1mValgrind: Errors or leaks found. Check val.log\033[0m" >&2; \
		exit 1; \
	fi
gdb: fj
	@gdb -q -x `pwd`/pyg.py -ex "py connect(rv=False)" --args ./fj challenge
