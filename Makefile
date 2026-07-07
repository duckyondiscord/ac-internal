all:
	gcc -fPIC -shared -o wh.so wh.c -ldl
	gcc -fPIC -shared -o hook.so hook.c -ldl
	gcc -fPIC -shared -o dumptex.so dumptex.c -ldl
wh:
	gcc -fPIC -shared -o wh.so wh.c -ldl
dumptex:
	gcc -fPIC -shared -o dumptex.so dumptex.c -ldl
clean:
	rm *.so
