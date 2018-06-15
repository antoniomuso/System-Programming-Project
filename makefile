CC=gcc -std=gnu11

OPTIONS=
ifeq ($(OS),Windows_NT)
OPTIONS += -lws2_32
else
OPTIONS += -lpthread
endif


ifeq ($(OS),Windows_NT)
RM = del
TARGET += *.exe
TARGET += *.o
else
RM = rm
TARGET += ./main
TARGET += ./*.o
endif




main: b64.c/decode.o b64.c/encode.o command_parser.o server.o signals.o windows_process_exe main.o
	${CC} server.o b64.c/decode.o command_parser.o main.o signals.o -o main ${OPTIONS}

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

windows_process_exe: windows_process_exe.c
	${CC} b64.c/decode.o command_parser.o server.o signals.o windows_process_exe.c  -o windows_process_exe.o ${OPTIONS}

signals.o: signals.c
	${CC} -c signals.c -o signals.o ${OPTIONS}
clean:
	$(RM) $(TARGET)
