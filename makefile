CC=gcc -std=gnu11

OPTIONS=
ifeq ($(OS),Windows_NT)
OPTIONS += -lws2_32
endif

main: b64.c/decode.o b64.c/encode.o command_parser.o server.o main.o
	${CC} server.o command_parser.o main.o -o main ${OPTIONS}

main.o: main.c
	${CC} -c main.c -o main.o ${OPTIONS}

server.o: server.c
	${CC} -c server.c -o server.o ${OPTIONS}

command_parser.o: command_parser.c
	${CC} -c command_parser.c -o command_parser.o ${OPTIONS}

b64.c/decode.o: ./b64.c/decode.c
	${CC} -c b64.c/decode.c -o b64.c/decode.o ${OPTIONS}

b64.c/encode.o: ./b64.c/encode.c
	${CC} -c b64.c/encode.c -o b64.c/encode.o ${OPTIONS}

clean:
	rm ./*.o |  rm ./main