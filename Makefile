build:
	g++ -Wall -g -std=c++11 server.cpp -o server
	g++ -Wall -g -std=c++11 subscriber.cpp -o subscriber

clean:
	rm -f server subscriber

