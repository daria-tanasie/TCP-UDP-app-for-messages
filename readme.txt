
    In fisierul helpers.h se afla 2 structuri:
    - tcp_client care reprezinta detaliile ce le pastram pt fiecare client tcp care s a conectat
la server
    - udp_message care ne va ajuta sa stocam si sa afisam mesajele primite de la clientul udp
    - protocol_message care reprezinta structura care va fi primita si trimisa de la/ la clientii tcp.
In cadrul acesteia avem 2 campuri: type ce reprezinta ce tip de mesaj vrem sa trimitem (exit, udp message,
subscribe, unsubscribe) si buf care reprezinta mesajul propriu-zis

    In fisierul server.c:
    Vom avea 2 socketuri (unul pt tcp si unul pt udp) pe care le vom lega cu bind la server(serv_addr) si
vom asculta cu listen pe socketul tcp.
    Vom declara o structura de tip pollfd pt a utiliza poll(). Pe prima pozitie din vectorul de structuri
pollfd vom avea fd pentru intrarea standard input, pe a 2a fd ul pt socketul udp si pe a 3a pt cel tcp.
    Printr un while(1) pornim serverul si incepem sa iteram prin vectorul de pollfd sa vedem ce trebuie sa
facem in continuare.
    Verficam daca avem un event pe fd ul curent si apoi verificam pt ce tip de fd este. Daca este cel de 
tcp, inseamna ca un nou client vrea sa se conecteze la server. Acceptam incepem sa primim informatia. Daca
este prima conectare la server, clientul va trimite ID ul sau printr un send (acest lucru este facut in
subscriber.c dupa ce s a realizat conectarea la server). Serverul va cauta atunci daca exista un client cu
acelasi ID (daca exista si este on, se afiseaza mesaj si se inchide conexiunea pe acel socket, iar daca
nu este on, inseamna ca acest client s a mai conectat anterior, deci vom schimba doar campul is_on). Daca
nu exista un client cu acelasi ID, il adaugam in vectorul de clientii, setand toate campurile si afisam
mesaj.

    Daca in schimb avem un event pe socketul de stdin si primim comanda de exit, serverul va trimite catre
toti clientii o structura message_to_send cu type ul 0, ce indica inchiderea conexiunii cu serverul, iar apoi
inchidem socketul clientilor, iar apoi serverul. In cadrul subscriberului, daca se primeste aceasta structura
cu type ul 0, inchidem conexiunea cu serverul si apoi inchidem si subscriberul.
    Daca in cadrul serverului avem event pe socketul udp, va trebuie sa vedem carui client trebuie sa trimitem
mesaj. Vom primi mesajul, iar apoil il parsam pentru a l pune intr o structura de tip udp_message. Setam apoi
si campul type din cadrul structurii Protocol_message si anume message_to_send pt a l trimite catre subscribe.
Trebuie acum sa iteram prin toti clientii care sunt on si prin topicurile lor pentru a afla cui sa trimitem
mesajul primit. Daca clientul este abonat la un topic ce contine '*', vom folosi functia search_star() pt a 
afla daca este un match intre topicul clientului si cel primit de la clientul udp.
    In cadrul functieii search_star(): 
    Daca avem stea pe prima pozitie si este singurul lucru din topic, trimitem mesajul.
    Parsam fiecare topic (cel al clietului si cel de la udp) dupa '/' si pastram cuvintele intr un vector
de cuvinte cu functia strtok(). Cautam apoi steaua in topic. Daca este ultimul segment din topic, verificam daca
pana la acel segemnt, toate celelalte sunt aceleasi, iar daca da, trimitem mesajul. Daca stteaua este in mijloc,
trebuie sa verificam daca avem dupa match de cuvinte, iar daca avem, trimitem mesajul. Daca nu ne aflam in
cazurile de mai sus, nu trimitem mesajul.
    Daca avem '+' in topicul clientului, folosim functia search_plus(). Parsam cuvintele dupa '/' si le
punem in vectorii de cuvinte. Daca nu avem acelasi nr de cuvinte, automat topicul nu este match si nu trimitem
mesajul. Altfel, verificam ca toate celelalte cuvinte sa fie match, daca nu este plus. In caz pozitiv, trimitem
mesajul. Daca avem si plus si stea, folosim functia search_both(). Parsam cuvintele in acelasi mod si 
cautam steaua. Daca steaua se afla pe ultima pozitie, verificam daca pana in acel punct, toate celelalte
cuvinte sunt match. Daca nu, nu vom trimite mesajul, iar in sens contrar il trimitem. Daca steaua nu se afla
pe prima pozitie, verificam ca dupa ea sa avem match de cuvinte, ca la functia search_star(). Daca nu avem
match, returnam 0 si nu trimtem mesajul. Daca avem totusi, verificam sa avem acelasi nr de cuvinte ramase,
iar apoi vom itera prin restul de cuvinte ramase sa vedem daca in rest avem match, in afara cazului in
care este plus. Daca se respecta aceste conditii, avem match si trimitem mesajul. Daca nu avem nici stea
nici plus, verificam doar ca topicul sa fie un match si trimitem mesajul.
    In cadrul subscriberului:
    Daca avem event pe socketul serverului si este de tip 1, avem de afisat un mesaj de la clientul udp.
Parsam mesajul de la server si apoi apelam functia print_message(). In cadrul acesteia vom verifica ce tip de
date trebuie sa afisam:
    Pt 0, avem INT si vom parsa mesajul conform regulilor indicate. Daca byte ul de semn e 0, avem un nr
pozitiv pe care il transfomam din network byte order, iar daca e 1, este negativ si il transformam.
    Pt 1, avem SHORT_REAL si vom imparti nr transformat din network byte order la 100.
    Pt 2, avem FLOAT si aflam numarul, puterea la care trebuie inmultit nr si ce fel de nr este(negtiv sau
pozitiv). Dupa toti acesti pasi, afisam mesajul.
    Pt 3, avem STRING si punem doar null la final si afisam mesajul. 

    Ne intoarcem in server.c. Daca nu avem event pe niciunul din socketurile mentionate mai sus, inseamna
ca ne aflam in cazul in care primim mesaj de la clientii tcp (subscriberi). Acestia pot trimite mesaje de
3 feluri: exit (type 0), subscribe (type 2) si unsubscribe (type 3).
    Daca este type 0, vom seta clientul ca fiind deconectat de la server (is_on = 0) si printam mesaj. Pentru
aceasta operatie, subscriberul primeste de la tastatura comanda exit de la stdin. In cadrul fisierului
subscriber.c, citim ce s a scris la stdin, iar daca este "exit", vom seta type pe 0 si trimitem message_to_send,
iar apoi inchidem socketul catre server.
    In server.c: Daca avem type 2, inseamna ca trebuie sa adaugam un nou topic la lista de topicuri ale clientului
care a dat subscribe. Pentru asta, cautam fd-ul clientului, iar apoi folosim strtok() pt a parsa mesajul,
si apoi adaugam topicul in vector. In subscriber.c: Daca la stdin primim subscribe, setam type pe 2 si trimitem
message_to_send catre server pt a se adauga topicul, iar apoi printam mesaj la stdout.
    In server.c: Daca avem type 3, va trebui sa stergem din vectorul de topic al clientului in cauza, topicul
primit la stdin din client. Cautam clientul in vectorul de clienti, iar apoi cautam topicul si il eliminam.
In subscriber.c: Daca primim unsubscribe la stdin, setam type ul pe 3, iar apoi trimitem message_to_send catre
server. Parsam apoi mesajul si afisam la stdout mesaj. La final inchidem socketul cu serverul.


