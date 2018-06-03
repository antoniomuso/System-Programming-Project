
main: b64.c/decode.o b64.c/encode.o command_parser.o server.o main.o
	gcc server.o command_parser.o main.o -o main

main.o: main.c
	gcc -c main.c -o main.o

server.o: server.c
	gcc -c server.c -o server.o

command_parser.o: command_parser.c
	gcc -c command_parser.c -o command_parser.o

b64.c/decode.o: ./b64.c/decode.c
	gcc -c b64.c/decode.c -o b64.c/decode.o

b64.c/encode.o: ./b64.c/encode.c
	gcc -c b64.c/encode.c -o b64.c/encode.o

clean:
	rm ./*.o |  rm ./main