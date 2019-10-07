#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

int main(int argc, char *argv[]) {
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 3) {
		std::cout << "parameter problem" << std::endl;
		return -1;
	}

	fd_set read_fds, tmp_fds;
	char message[2000];
   
  FD_ZERO(&read_fds); // initializare fdset
  FD_ZERO(&tmp_fds);  // initializare fdset

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// adaug in read_fds standard inputul
	FD_SET(STDIN_FILENO, &read_fds);
	// adaug in read_fds socketul folosit pentru comunicatia cu serverul
  FD_SET(sockfd, &read_fds);


  // trimit serverului id-ul clientului
  send(sockfd, argv[1], strlen(argv[1]), 0);

	while (1) {
		tmp_fds = read_fds; // restore fds

    // multiplexez cu select
    ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		
    // incep verificarile pe file descriptori
		// daca am primit ceva de la stdin
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {

    	// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
      fgets(buffer, BUFLEN - 1, stdin);
 
      if (strncmp(buffer, "exit", 4) == 0) {
        break;
      }
 
      // se trimite mesaj la server
      n = send(sockfd, buffer, strlen(buffer), 0);
      DIE(n < 0, "send");
    } else if (FD_ISSET(sockfd, &tmp_fds)) {
			// am primit mesaj de la server
			memset(message, 0, sizeof(message));
      n = recv(sockfd, message, sizeof(message), 0);
      DIE(n < 0, "recv subscriber");

			// daca am primit exit inseamna ca serverul s-a inchis
  		if (strncmp(message, "exit", 4) == 0) {
				break;
			}

			// daca nu am primit exit, afisez datele primite
			std::cout << message;
    }
	}

	close(sockfd);

	return 0;
}
