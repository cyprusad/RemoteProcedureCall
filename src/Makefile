default: librpc.a binder

binder:
	g++ -x c sck_stream.c -x c++ utility.cpp -x c++ binder.cpp  -lpthread -o binder

sck_stream.o:
	g++ -c sck_stream.c

utility.o:
	g++ -c utility.cpp

client_proc.o: sck_stream.o utility.o
	g++ -c client_proc.cpp 

server_proc.o: sck_stream.o utility.o
	g++ -c server_proc.cpp

librpc.a: sck_stream.o utility.o client_proc.o server_proc.o
	ar -cvq librpc.a sck_stream.o
	ar -cvq librpc.a utility.o
	ar -cvq librpc.a client_proc.o
	ar -cvq librpc.a server_proc.o 

clean:
	rm -f *.o
	rm -f *.a
	rm -f binder
	rm -f *.gch