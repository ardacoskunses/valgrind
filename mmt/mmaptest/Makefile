all: build test

build: mmaptest32 mmaptest64

mmaptest32: mmaptest.c
	@gcc -m32 -msse -Wall mmaptest.c -o mmaptest32

mmaptest64: mmaptest.c
	@gcc -m64 mmaptest.c -Wall -o mmaptest64

clean:
	@rm -f mmaptest32 mmaptest64 mmaptest32.out1.tmp mmaptest32.out2.tmp mmaptest64.out1.tmp mmaptest64.out2.tmp

test: test32 test64

test32: mmaptest32
	@../../coregrind/valgrind --tool=mmt --mmt-trace-file=/dev/zero ./mmaptest32 >mmaptest32.out1.tmp 2>mmaptest32.out2.tmp || (cat mmaptest32.out1.tmp && cat mmaptest32.out2.tmp && false)
	@cat mmaptest32.out1.tmp | diff -u mmaptest32.out1 -
	@cat mmaptest32.out2.tmp | grep "^--" | sed "s/^--[0-9]*-- //" | diff -u mmaptest32.out2 -

test64: mmaptest64
	@../../coregrind/valgrind --tool=mmt --mmt-trace-file=/dev/zero ./mmaptest64 >mmaptest64.out1.tmp 2>mmaptest64.out2.tmp || (cat mmaptest64.out1.tmp && cat mmaptest64.out2.tmp && false)
	@cat mmaptest64.out1.tmp | diff -u mmaptest64.out1 -
	@cat mmaptest64.out2.tmp | grep "^--" | sed "s/^--[0-9]*-- //" | diff -u mmaptest64.out2 -