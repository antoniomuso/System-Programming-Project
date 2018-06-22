# Relazione Progetto Programmazione di Sistema (A.A 2017-2018)
Antonio Musolino, Giacomo Priamo

## Introduzione
Il progetto è stato sviluppato e testato contemporaneamente su un sistema Unix e uno Windows, il che ci ha permesso di 
poter gestire da subito gli errori riscontrati sulle diverse piattaforme, di non dover tornare 
sulle stesse funzioni in un secondo momento per re-implementare le stesse funzionalità 
e anche per fare massimizzare il riutilizzo del codice.

Le funzionalità fondamentali e critiche, come la creazione e gestione del server pre-forkato, la gestione delle 
richieste HTTP in accordo con le specifiche, la di gestione segnali ed eventi, sono state realizzate in presenza di 
entrambi in modo  simile alla tecnica di sviluppo "[Pair Programming](https://en.wikipedia.org/wiki/Pair_programming)"; 
mentre quelle secondarie, come i vari parser, sono state sviluppate individualmente.  

## Implementazione
Per aumentarne la leggibilità e la modularità, è stato deciso di organizzare il codice in file separati, 
ognuno con un proprio header, che vengono compilati assieme automaticamente tramite il _makefile_.

### Compilazione
Per compilare l'intero programma è sufficiente posizionarsi nella directory del progetto e usare il makefile:

- Nel caso di Unix, è sufficiente chiamare `make`.
- In Windows, è possibile invocare il compilatore digitando `mingw32-make.exe` 

### Lancio
È possibile avviare il programma eseguendo il file _main_ (`./main` su Unix e `main.exe` su Windows). Senza parametri,
verranno caricate le impostazioni specificate nel file di configurazione _config.txt_, oppure è possibile specificare  
manualmente i parametri, tutti o in parte (in quest'ultimo caso, i restanti valori saranno estratti dal file di 
configurazione), secondo la seguente sintassi di lancio  da linea di comando:
- `-server_ip <ip>` per specificare l'indirizzo IP su cui aprire la socket.
- `-port <numero_porta>` per specificare la porta a cui legarsi, il server si legherà automaticamente alla porta 
successiva allo scopo di poter scaricare i file richiesti dopo che essi siano stati cifrati come da specifiche.
- `-mode <mode>` per specificare la modalità di funzionamento del server: **MT** per multi thread e **MP** per
multi processo.
- `-n_proc <numero>` per specificare quanti thread/processi (a seconda della modalità scelta) "figli" creare.
### Server
##### Avvio del server
Dopo aver caricato i vari parametri, si procede con la configurazione del server con la creazione delle socket: 
una "normale" e l'altra per le sole operazioni di _GET_ precedute da cifratura.
##### Thread e Processi
Successivamente vengono creati, attraverso le apposite chiamate a funzione, i thread/processi "figli". 

Particolarmente per il caso Windows con modalità multi processo, poiché la funzione `Create_Process` richiede che venga 
specificato un file eseguibile che deve essere lanciato dal nuovo processo, è stato creato il file 
_windows_process_exe.c_. Lo scopo di tale programma è quello di ricevere ed inizializzare le socket che gli vengono 
inviate dal processo che lo genera, il quale prima chiama la funzione `WSADuplicateSocket` per duplicare le socket, e
in seguito le trasmette al processo figlio usando come meccanismo di comunicazione inter-processo una _Named Pipe_
(una separata per ogni processo creato, visto che la duplicazione è specifica per un singolo processo). Da lì, il 
comportamento del processo appena creato si uniforma con quello degli altri thread/processi creati con altre
modalità o sotto piattaforma Unix.    
##### Routine 
Tutti i processi/thread figli eseguono la stessa funzione: `process_routine` che riceve in input le due socket create in
precedenza dal processo padre e in cui è specificata la modalità di gestione delle richieste in arrivo da
parte di client esterni. Al fine di evitare che i figli si blocchino su una sola `accept`, 
si utilizza la funzione `select` per risvegliare i processi appena si verifica un cambiamento su una delle due socket, 
ciò permette di evitare un'attesa attiva dei figli, che sarebbe troppo dispendiosa.  
Una volta ricevuta una richiesta, si verificano che le credenziali sottomesse siano corrette e si procede con la sua
gestione.
### Operazioni
Per relizzare l'accesso esclusivo ai file sono state usate la funzione `flock` per Unix e ...
##### GET
###### File
###### Directory
Nel caso in cui l'url richiesta corrisponde a una directory, viene invocata la funzione `list_dir` che restituisce il 
contenuto della directory indicata.
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

