Calatoaie Iulia-Adriana - Grupa 325 CA

Tema 2 - Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

Pentru aceasta tema am implementat un server si un client tcp. Clientul
tcp are un id, dupa care va fi recunoscut. Asta inseamna ca daca se
deconecteaza, campul connected va ajunge 0, dar informatiile lui despre id si
topicurile cu flag 1 se vor pastra. Astfel atunci cand vor fi primite de la
clientul udp mesaje, vor exista doua posibilitati ca un client sa il primeasca:
1. clientul este conectat si are topicul in lista
2. clientul nu este conectat, dar are topicul in vectorul cu topicuri 1
In cazul 2 mesajele se vor pastra in campul container si vor fi trimise atunci
cand clientul se va conecta din nou.

In cazul in care se primeste comanda exit in client, in server se va afisa
mesajul corespunzator. La fel in cazul in care se conecteaza un client la
server.

In server se va pastra ierarhia clientilor sub forma unui map, unde cheile
vor fi socketii si valorile clientii. Cand se va deconecta un client, se va
pastra in map doar daca este abonat la topicuri cu flag 1, adica daca vectorul
de topicuri topic_one nu este gol. Daca acesta este abonat la topicuri cu flag
1, atunci se va face update la campul connected care va deveni 0 si se poate
goli vectorul topic_zero, unde se afla topicurile cu flag 0. Daca clientul nu
este abonat la topicuri cu flag 1, poate fi scos cu totul din map. Astfel,
atunci cand se primeste o cerere de conectare a unui client noi, se verifica
daca nu cumva a mai fost conectat, cu ajutorul campului connected = 0 si cand
clientul se afla in map, iar daca raspunsul este pozitiv, i se trimit toate
mesajele din container ca mai apoi acesta sa fie golit.

Cand se va da comanda de subscribe, mesajul va fi trimis serverului. Atunci
cand ajunge acolo numele topicului va fi adaugat la vectorul topic_one sau
topic_zero, daca a fost trimis cu flag 1 sau 0. La comanda de unsubscribe se va
cauta numele topicului in ambii vectori si daca se va gasi, se va elimina din
vectorul cu pricina.

Cand serverul va primi comanda de exit, va trimite pe rand fiecarui client tcp
comanda de exit, il va elimina din multimea de descriptori, se vor inchide
socketii tcp si udp folositi si se va iesi din program. Daca se primeste orice
altceva atat in server cat si in client in afara de comenzile valide enumerate
in enuntul temei, atunci nu se va intampla nimic. Comenzile invalide se ignora.

Mesajele de la clientul udp vor fi receptionate intr-o structura de tip mesaj.
Aceasta va avea campurile: ip, port(pentru usurarea afisajului), value, type,
topic. Dintre acestea value va fi singurul ce va suporta prelucrare in cadrul
functiei ajutatoare pretty_message ce va compune mesajul de trimis clientului
tcp dupa formatul specificat in cerinta. Ajuns la clientul tcp acesta va fi
pur si simplu afisat dupa ce se va intreba daca nu cumva s-a primit comanda
de exit, caz in care se inchide socket-ul si se iese din program.