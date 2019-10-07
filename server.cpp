#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

int main(int argc, char* argv[]) {
  int sock_tcp, sock_udp, newsock_tcp, newsock_udp, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv, client;
	int n, i, j, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

  std::unordered_map<int, struct subscriber> sock_id;
	struct msg message;
	char mymsg[2000];

	if (argc < 2) {
		std::cout << "parameters error" << std::endl;
		return -1;
	}

	// se goleste multimea de descriptori de citire
	// (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// deschid socket-ul tcp
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "socket tcp");

	// obtin portul
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(portno);
	serv.sin_addr.s_addr = INADDR_ANY;

	// bind pe tcp
	ret = bind(sock_tcp, (struct sockaddr *) &serv, sizeof(struct sockaddr));
	DIE(ret < 0, "tcp bind");

	ret = listen(sock_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	//deschid socket-ul udp
	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "socket udp");

	// bind pe udp
	ret = bind(sock_udp, (struct sockaddr *) &serv, sizeof(struct sockaddr));
	DIE(ret < 0, "udp bind");

	// se adauga file descriptorii (socketul pe care se asculta conexiuni)
	// in multimea read_fds
	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	fdmax = (sock_tcp > sock_udp) ? sock_tcp : sock_udp;

	while (1) {
		tmp_fds = read_fds;
		memset(buffer, 0, BUFLEN);
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; ++i) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == STDIN_FILENO) {
					// se citeste de la tastatura
					memset(buffer, 0, BUFLEN);
        	fgets(buffer, BUFLEN - 1, stdin);
 
        	if (strncmp(buffer, "exit", 4) == 0) {
						// daca se primeste exit trebuie sa trimit clientilor comanda
						for (j = 0; j <= fdmax; ++j) {
							// verific daca socket-ul este in multimea de descriptori
							if (FD_ISSET(j, &read_fds)) {
								if (j != STDIN_FILENO && j != sock_udp && j != sock_tcp) {
									n = send(j, buffer, strlen(buffer), 0);
									DIE(n < 0, "send exit");
									close(j); // inchid socket-ul
								}
							}
						}
						// inchid socket-ii udp si tcp
						close(sock_tcp);
						close(sock_udp);

						// si ies din program
         		return 0;
        	}
				} else if (i == sock_tcp) {
					// a venit o cerere de conexiune pe socketul tcp,
					// pe care serverul o accepta
					clilen = sizeof(client);
					newsock_tcp = accept(sock_tcp, (struct sockaddr *) &client, &clilen);
					DIE(newsock_tcp < 0, "accept");

					// se adauga noul socket intors de accept()
					// la multimea descriptorilor de citire
					FD_SET(newsock_tcp, &read_fds);

					if (newsock_tcp > fdmax) { 
						fdmax = newsock_tcp;
					}
				} else if (i == sock_udp) {
					// aici trimit mesajele clientilor tcp de la udp
					clilen = sizeof(client);
					// curat datele
					memset(message.value, 0, sizeof(message.value));
					memset(message.topic, 0, sizeof(message.topic));
					// adaug datele in structura
					newsock_udp = recvfrom(i, &message, sizeof(message), 0,
												(struct sockaddr *) &client, &clilen);
					DIE(newsock_udp < 0, "recv udp");

					// adaug portul in structura
					message.port = ntohs(client.sin_port);
					// adaug ip-ul
					memset(message.ip, 0, sizeof(message.ip));
					strcpy(message.ip, inet_ntoa(client.sin_addr));

					memset(mymsg, 0, sizeof(mymsg));
					strcpy(mymsg, pretty_message(message).c_str());

          if (FD_ISSET(i, &tmp_fds)) {
						// trimit tuturor clientilor subscriberi la topic mesajul
						for (auto it = sock_id.begin(); it != sock_id.end(); ++it) {
							// daca nu e conectat o sa ii pastrez mesajele
							// cu fs 1 in container
							if (it->second.connected == 0) {
								int index_one = find_element(it->second.topic_one,
																						 message.topic);
								if (index_one != -1) {
									it->second.container.push_back(mymsg);
								}
							}
							// daca este conectat si ii trimit mesajul oricum
							if (it->second.connected == 1) {
								// caut in ambele liste topic-ul
								int index_one = find_element(it->second.topic_one, message.topic);
								int index_zero = find_element(it->second.topic_zero, message.topic);
								if (index_one != -1 || index_zero != -1) {
									n = send(it->first, mymsg, sizeof(mymsg), 0);
									DIE(n < 0, "send");
								}
							}
						}
        	}
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					// caut socket-ul in map
					auto got_sock = sock_id.find(i);
					// verific daca a fost conectat anterior
					bool was_connected = false;
					if (got_sock != sock_id.end()) {
						if (got_sock->second.connected == 0) {
							was_connected = true;
						}
					}

					if (n == 0) {
						// conexiunea s-a inchis
          	// afisez mesajul
						std::cout<< "Client (" << sock_id[i].id <<  ") disconected.\n";

						// daca nu are topicuri cu fs 1, este in regula sa il sterg
						if (sock_id[i].topic_one.empty()) {
							sock_id.erase(i);
						} else {
							// setez clientul ca deconectat
          		sock_id[i].connected = 0;
							// pot sterge topicurile cu fs 0
							sock_id[i].topic_zero.clear();
						}

						close(i);  // inchid socket-ul
						// scot socket-ul inchis din multimea de citire 
						FD_CLR(i, &read_fds);

					} else if (got_sock == sock_id.end() || was_connected) {
						// daca nu am gasit socket-ul in map atunci acum am primit id-ul
          	// si trebuie sa il adaug
          	struct subscriber sub;
          	sub.id = buffer;
						sub.connected = 1;

						// vreau sa vad daca clientul a mai fost conectat
						bool connected_once = false;
						for (auto it = sock_id.begin(); it != sock_id.end(); ++it) {
							// in caz ca a mai fost conectat, atunci setez variabila
							// care imi spune ca acum este conectat
							if (it->second.id == sub.id) {
								connected_once = true;
								it->second.connected = 1;

								// si ii trimit toate mesajele cu fs 1
								int size = it->second.container.size();
								char tmp_send[2000];
								for (int i = 0; i < size; ++i) {
									memset(tmp_send, 0, sizeof(tmp_send));
									strcpy(tmp_send, it->second.container[i].c_str());
									n = send(it->first, tmp_send, sizeof(tmp_send), 0);
									DIE(n < 0, "send ");
								}
								break;
							}
						}

						// daca nu a fost conectat il adaug in map
						if (!connected_once) {
							sock_id[i] = sub;
						}

						// afisez mesajul ca s-a inscris un client
          	std::cout << "New client (" << buffer << ") connected from "
						<< inet_ntoa(client.sin_addr) << ":" << ntohs(client.sin_port) << ".\n";

					} else if (strncmp(buffer, "subscribe", 9) == 0 ||
										strncmp(buffer, "unsubscribe", 11) == 0) {
						// am primit o cerere de abonare/dezabonare
						change_subscribe_state(sock_id, buffer, i);
					}
				}
			}
		}
	}

	// inchidere socketi
	close(sock_tcp);
	close(sock_udp);

  return 0;
}