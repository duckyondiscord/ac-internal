all:
	gcc -fPIC -shared -o wh.so wh.c primitives/hook.c -ldl -masm=intel
	gcc -fPIC -shared -o dumptex.so dumptex.c -ldl
wh:
	gcc -fPIC -shared -o wh.so wh.c primitives/hook.c -ldl -masm=intel
dumptex:
	gcc -fPIC -shared -o dumptex.so dumptex.c -ldl
clean:
	rm *.so
