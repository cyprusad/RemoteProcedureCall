default: librpc.a binder

binder:
	g++ -x c sck_stream.c -x c++ utility.cpp -x c++ binder.cpp -o binder

client_proc.o:
	g++ -x c sck_stream.c -x c++ utility.cpp -x c++ client_proc.cpp -o client_proc.o

server_proc.o:
	g++ -x c sck_stream.c -x c++ utility.cpp -x c++ server_proc.cpp -o server_proc.o

librpc.a: client_proc.o server_proc.o
	ar -cvq librpc.a client_proc.o server_proc.o

clean:
	rm -f *.o
	rm -f *.a
	rm -f binder
	rm -f *.gch