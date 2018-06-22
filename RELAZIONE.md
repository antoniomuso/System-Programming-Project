# Relazione Progetto Programmazione di Sistema (A.A 2017-2018)
Antonio Musolino, Giacomo Priamo

## Introduzione
Il progetto è stato sviluppato e testato contemporaneamente su un sistema Unix e uno Windows, il che ci ha permesso di 
poter gestire da subito gli errori riscontrati sulle diverse piattaforme, di non dover tornare 
sulle stesse funzioni un secondo momento per re-implementare le stesse funzionalità 
e anche per fare massimizzare il riutilizzo del codice.

Le funzionalità fondamentali e critiche (come la creazione e gestione del server pre-forkato, la gestione delle 
richieste HTTP in accordo con le specifiche, la di gestione segnali ed eventi) sono state realizzate in presenza di 
entrambi in modo  simile alla tecnica di sviluppo "[Pair Programming](https://en.wikipedia.org/wiki/Pair_programming)", 
mentre quelle secondarie come i vari parser sono state sviluppate individualmente.  

## Implementazione
Per aumentare la leggibilità e la modularità del codice è stato deciso di organizzare il codice in file separati, 
ognuno con un proprio header, che vengono compilati assieme automaticamente tramite il makefile.

### Compilazione
Per compilare l'intero programma è sufficiente posizionarsi nella directory del progetto e usare il makefile:

- Nel caso di Unix, è sufficiente chiamare `make`.
- Nel per Windows, è possibile invocare il compilatore digitando `mingw32-make.exe` 

### Server
##### Avvio del server
##### Creazione Thread o Processi
##### Routine 


### Operazioni

##### GET
###### File
###### Directory
##### GET con cifratura
##### PUT
##### Esecuzione comandi
(argomenti)
##### Logging

### Parser

### Segnali ed Eventi
(stessa funzione di terminazione)
##### Unix
##### Windows

