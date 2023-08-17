# WEB CLIENT

322CA - Bianca Ștefania Dumitru
Protocoale de comunicatii

Mai 2023
----------------------------------------------------------------------------------------------------
## Introducere

* Web client
  * programul simuleaza o biblioteca online, unde userii
        se pot conecta si pot vedea, adauga sau sterge carti
  * clientul comunica cu serverul prin protocolul HTTP,
        interactionand cu REST API-ul acestuia
  * pentru stabilirea accesului la resurse este folosit
        conceptul de cookie si token jwt
  * datele transmise intre client si server sunt in format
        JSON, parsate cu ajutorul bibliotecii parson    

## Ce contine proiectul?

* client.c - Functionalitatea principala a clientului. Acesta
    se conecteaza la server si asteapta input de la utilizator.
    Prin intermediul multiplexarii cu poll se detecteaza atunci
    cand conexiunea cu serverul se inchide din cauza inactivitatii
    (5s) si redeschide conexiunea. Clientul poate face cereri de tip
    register, login, logout, acces la biblioteca, dar si afisari,
    adaugari si stergeri de carti in biblioteca.

* requests.h - Functii ce creeaza cele 3 tipuri de requesturi,
    POST, GET, DELETE, pe baza inputului primit.

* parson.h - Biblioteca externa de parsare si creare a obiectelor JSON.

* helpers.h - Functii ajutatoare precum: deschiderea si inchiderea conexiunii,
    citirea si scrierea catre server, parsarea raspunsului primit de la server, etc.

* buffer.h - Functii ce prelucreaza un buffer, folosite in special pentru
    receptionarea raspunsului de la server.

## Cum am implementat?

Am inceput prin a folosi **Postman** pentru a testa toate rutele, cu diverse
payload-uri, ca sa imi dau seama cum functioneaza si cum ar trebui sa arate
cererile de la client, dar si raspunsurile serverului.

Am luat rezolvarea facuta de mine a **laboratorului 9**, care a reprezentat scheletul
temei. Am modificat structura requesturilor pentru a acomoda prezenta **cookie**-ului
si a **token**-ului. Pentru parsarea si crearea obiectelor **JSON** am folosit biblioteca
recomandata pentru C, **parson**. 

Initial nu faceam **multiplexare** in client, ci doar citeam requesturile de la
input si cream si trimiteam cererile catre server. Mi-am dat apoi seama, citind si
discutiile de pe forum, de faptul ca serverul se inchide dupa 5s. La sugestia de acolo
a lui Florin-Alexandru STANCU, am hotarat sa folosesc **poll** pentru a detecta inchiderea
si a reconecta. In plus, in cazul in care dureaza mai mult de 5s ca userul sa trimita
parametrii unei comezi (de exemplu username), atunci conexiunea se va inchide din nou,
asa ca daca vad ca primesc un raspuns gol de la server dupa ce am trimis un request,
redeschid conexiunea si retrimit mesajul.

Fiecare comanda primita de la stdin e verificata de client, folosind functia check_input
(requests.c). La detectarea unui input gresit, **requestul se incheie** si se asteapta un altul.

Daca incercam sa facem o operatiune la momentul nepotrivit (ex. logout fara sa
fim logati, get book cand nu avem acces la biblioteca), atunci clientul afiseaza
imediat un **mesaj de eroare** fara sa mai accepte inputul ulterior
corespunzator requestului esuat (de ex. nu mai asteapta id-ul cartii).

Codurile de raspuns de la server sunt tratate: pentru **succes** (200/201) este
afisat un mesaj de confirmare, iar in caz de **eroare** (4xx/5xx) este afisat codul si
eroarea primita de la server.

## Resurse
* Enuntul temei
* Laboratorul 9
* https://github.com/kgabis/parson
* https://web.postman.co