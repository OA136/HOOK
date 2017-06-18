objects =  nlusr.o

nlusr : $(objects)
	gcc -o nlusr $(objects) -lz -lmysqlclient
nlusr.o : nlusr.c
	gcc -c nlusr.c

clean:
	rm nlusr $(objects)

url_start:
	gcc -o url_start url_start.c -lz -lmysqlclient
content_start:
	gcc -o content_start content.c -lz -lmysqlclient
