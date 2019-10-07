#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>

// un mesaj are :
struct __attribute__((packed)) msg {
	char topic[50];    // un topic,
	char type;         // un tip,
	char value[1500];  // date,
	char ip[16];       // ip-ul de pe care a fost trimis,
	int port;          // portul de pe care a fost trimis
};

// un subscriber are :
struct subscriber {
  // o stare de a fi conectat sau nu,
  int connected;
  //  un id,
  std::string id;
  // o lista de mesaje ale caror sf e 1,
  std::vector<std::string> topic_one;
  // o lista de mesaje ale caror sf e 0,
  std::vector<std::string> topic_zero;
  // un container pentru a tine mesajele cu sf 1 cat e offline
  std::vector<std::string> container;
};

/*
 * Macro de verificare a erorilor din laborator
 * Thankies :D
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1550	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	100		// numarul maxim de clienti in asteptare


// parseaza cele doua/trei argumente pentru comanda de un/subscribe
// le pune intr-un vector
std::vector<std::string> parse(std::string s) {
	std::vector<std::string> result;
	std::string delimiter = " ";
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
    	token = s.substr(0, pos);
    	result.push_back(token);
    	s.erase(0, pos + delimiter.length());
	}
	if (result[0] == "subscribe" && !s.empty()) {
		result.push_back({s[0]});
	}

	return result;
}

// gasirea elementului intr-un vector
// daca s-a gasit, se returneaza indicele, altfel -1
int find_element(std::vector<std::string> v, std::string str) {
	if (v.size() == 0) return -1;
	for (unsigned i = 0; i < v.size(); ++i) {
		if (v[i] == str) {
			return i;
		}
	}
	return -1;
}


// schimbarea starii de subscribe
void change_subscribe_state (std::unordered_map<int, struct subscriber> &si,
							std::string msg, int socket) {
	std::vector<std::string> tokens = parse(msg);
	std::string topic = tokens[1];

	if (tokens[0] == "subscribe") {
		if (tokens.size() < 3) {
			return;
		}
		// adaug topicul la lista
		if (tokens[2] == "1") {
			si[socket].topic_one.push_back(topic);
		} else {
			si[socket].topic_zero.push_back(topic);
		}
	} else if (tokens[0] == "unsubscribe") {
		// la comanda de unsubscribe pur si simplu il scot din
		// lista de topicuri corespunzatoare
		if (tokens.size() < 2) {
			return;
		}

		// caut in ambele liste si acolo unde gasesc, sterg
		int index = find_element(si[socket].topic_one, topic);
		if (index != -1) {
			si[socket].topic_one.erase(si[socket].topic_one.begin() + index,
									si[socket].topic_one.begin() + index + 1);
		}

		index = find_element(si[socket].topic_zero, topic);
		if (index != -1) {
			si[socket].topic_zero.erase(si[socket].topic_zero.begin() + index,
									si[socket].topic_zero.begin() + index + 1);
		}
	}
}

// functie helper pentru construirea mesajului
u_int16_t parse_short(struct msg message) {
	// ia parametri din structura
	u_int16_t fst = message.value[0];
	u_int16_t snd = message.value[1];
	// compune cu ei short unsigned
	return (((short)fst) << 8) | (0x00ff & snd);
}

// functie helper pentru construirea mesajului
int parse_int(struct msg message) {
	char val[4];
	// extrag octetii 1-4
	for (int i = 0; i < 4; ++i) {
		val[i] = message.value[4 - i];
	}
	// cast la int
	int result = *((int*) val);

	// daca este negativ, atunci trebuie sa ii fac complementul afar de 2
	if ((int)message.value[0] == 1) {
		return (~result + 1);
	}

	return result;
}

// functie helper pentru construirea mesajului
double parse_float(struct msg message) {
	char my_int[4];
	// extrag octetii 1-4 pe care se afla int
	for (int i = 0; i < 4; ++i) {
		my_int[i] = message.value[4 - i];
	}
	int res_int = *((int*) my_int);

	// extrag puterea
	uint8_t power = (uint8_t) message.value[5];

	// verific daca numarul e negativ
	if ((int)message.value[0] == 1) {
		res_int = ~res_int;
		++res_int;
	}

	// impart la puterea lui 10
	double result = (double) res_int / pow(10, power);

	return result;
}

// formeaza mesajul de afisat
std::string pretty_message(struct msg message) {
	std::string result = message.ip;
	result += ':';
	result.append(std::to_string(message.port));
	result.append(" - ");
	result.append(message.topic);

	if ((int)message.type == 0) {
		result.append(" - INT - ");
		std::string myint = std::to_string(parse_int(message));
		result.append(myint);

	} else if ((int)message.type == 1) {
		result.append(" - SHORT_REAL - ");
		u_int16_t nr = parse_short(message);
		// extrag ultimele doua cifre
		u_int16_t fst = nr % 10;
		nr /= 10;
		u_int16_t snd = nr % 10;
		nr /= 10;
		std::string myreal = std::to_string(nr);
		// formez string-ul
		myreal += '.';
		myreal.append(std::to_string(snd));
		myreal.append(std::to_string(fst));
		result.append(myreal);

	} else if ((int)message.type == 2){
		result.append(" - FLOAT - ");
		std::string mydouble = std::to_string(parse_float(message));
		result.append(mydouble);

	} else {
		result.append(" - STRING - ");
		result.append(message.value);
	}

	result += '\n';

	return result;
}


#endif