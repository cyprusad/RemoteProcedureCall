default: librpc.a binder

binder.o:
	g++ -c rpc.h sck_stream.h utility.hpp message_protol.h response_codes.h binder.cpp

sck_stream.o:
	g++ -c sck_stream.h message_protocol.h response_codes.h sck_stream.cpp

utility.o:
	g++ -c message_protocol.h utility.hpp utility.cpp

binder: sck_stream.o utility.o binder.o libpthread.a
	g++ -L. sck_stream.o utility.o binder.o -lpthread -o binder

client_proc.o: 
	g++ -c rpc.h sck_stream.h message_protocol.h utility.hpp response_codes.h client_proc.cpp

server_proc.o: 
	g++ -c rpc.h sck_stream.h message_protocol.h utility.hpp response_codes.h server_proc.cpp

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