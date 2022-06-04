CFLAGS=-W -Wall -g -pedantic -I/usr/local/lib

fsappl: fsappl.o simplefs.o
	gcc -o $@ $^

fsappl.o: fsappl.c simplefs.h
simplefs.o: simplefs.c simplefs.h

clean:
	rm -f *.o fsappl

zip:
	tar -cvzf spalmer2-project4.tgz simplefs.c report.txt
