serverM.o: serverM.c
	gcc -c serverM.c -g -Wall

serverM: serverM.o
	gcc -o serverM serverM.o -g -Wall

clientA.o: clientA.c
	gcc -c clientA.c -g -Wall

clientA: clientA.o
	gcc -o clientA clientA.o -g -Wall

clientB.o: clientB.c
	gcc -c clientB.c -g -Wall

clientB: clientB.o
	gcc -o clientB clientB.o -g -Wall

serverA.o: serverA.c
	gcc -c serverA.c -g -Wall

serverA: serverA.o
	gcc -o serverA serverA.o -g -Wall

serverB.o: serverB.c
	gcc -c serverB.c -g -Wall

serverB: serverB.o
	gcc -o serverB serverB.o -g -Wall

serverC.o: serverC.c
	gcc -c serverC.c -g -Wall

serverC: serverC.o
	gcc -o serverC serverC.o -g -Wall

all: clientA clientB serverM serverA serverB serverC

clean:
	rm -rf *.o serverM serverA clientA clientB serverB serverC alichain.txt

submit: clean
	tar cvf ee450_xiakezhu_30828.tar *.c *.h Makefile readme.md
	gzip ee450_xiakezhu_30828.tar