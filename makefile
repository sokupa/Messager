all:server client
server:
	@g++ -pthread  -std=c++11 server.cpp -o server
client:
	@g++ -pthread  -std=c++11 client.cpp -o client
clean:
	@rm -f *.o server client -r *_*
