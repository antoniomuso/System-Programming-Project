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
Su Unix, il codice è scritto in modo tale che il server nasca come **_processo demone_** (la daemonizzazione è 
effettuata nel file `main.c`), mentre su Windows esso nascerà come un normale processo.
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
Le richieste di GET sulla porta principale (quella specificata in input o dal file di configurazione) sono gestitite 
dalla funzione `send_file`.
###### File
Se il path richiesto corrisponde a un file, la funzione `send_file` lo apre, acquisisce il lock su di esso e, dopo 
averlo letto, lo invia al client richiedente. La lettura del file è eseguita a blocchi di dimensione massima prefissata,
i quali vengono inviati una volta raggiunta tale dimensione; il procedimento continua finquando non è stato letto e 
inviato l'intero contenuto del file.
###### Directory
Nel caso in cui il path richiesto corrisponde a una directory, viene invocata la funzione `list_dir` che restituisce il 
**contenuto della directory** indicata, che viene inviato come risposta.
##### GET con cifratura
Le richieste di GET per i file sulla porta che prevede cifratura preventiva del file stesso sono gestite dalla funzione
`send_file_cipher`. Dopo essere stato aperto, il file è mappato in memoria grazie alle apposite funzioni dei due sistemi
operativi, successivamente si procede con la cifratura tramite la funzione `encrypt`, che manipola blocchi di dimensione 
pari a 4 byte del file mappato  e li cifra mediante lo _XOR_ con un intero random avente come seme l'indirizzo IP del 
client. 
Nel caso in cui la dimensione del file non fosse divisibile per 4, è stato deciso di aggiungere un **padding** di zeri 
(0) all'ultimo blocco al fine di effettuare correttamente la cifratura, tale **padding** tuttavia non è né mappato in 
memoria né tantomeno inviato inviato al client. 
##### Esecuzione comandi
Come da specifiche, nel caso in cui il primo elemento del path contenga la la stringa **command** viene eseguita la 
funzione `exec_command`, la quale crea un thread che andrà a generare il processo che eseguirà il comando passato in 
input. Come meccanismi di sincronizzazione, sono stati usati una **_condition variable_** per Unix e un **_Evento_** per
Windows. Per entrambe le piattaforme si è deciso di usare il meccanismo delle **_pipe_** per redirezionare l'output dei
processi che eseguono il comando, al fine di inviarne il contenuto al client una volta completata l'esecuzione del 
thread.

L'implementazione di `exec_command` supporta anche il **passaggio di parametri**, secondo la seguente sintassi:
`[...]/command/arg1?arg2?[...]?argN`, dove nel caso di passaggio di comandi legati ai terminali 
(es: _cat_, _ipconfig_, _echo_) il primo argomento deve corrispondere, sia per Unix che per Windows, allo stesso valore
di _command_. 

Esempi:
- Unix: `/command/date?date` 
- Windows: `/command/C:/Windows/System32/cmd.exe?C:\Windows\System32\cmd.exe?/k?ipconfig` 

##### PUT
Le richieste di PUT vengono gestite dalla funzione `put_file`, che dopo aver creato un file con lo stesso nome di quello
che il client sta caricando ed ottenuto il lock su di esso, legge e trascrive il contenuto che gli viene inviato. Nel 
caso in cui esistesse già un file con lo stesso nome di quello che il client sta caricando, il file esistente viene 
sovrascritto.
##### Logging
Come da specifiche, tutte le richieste sono loggate all'interno del file _log.txt_ secondo il formato stabilito dal 
_common log format_.

### Segnali ed Eventi da console
Come da specifiche, il server è in grado di rileggere il file di configurazione dopo l'avvenimento di uno specifico 
evento:
- Su Unix, ciò avviene quando il server (processo padre) riceve il segnale `sighup`. È stato quindi necessario 
installare un **_signal handler_**.
- Su Windows, ciò avviene quando nella console è premuta la combinazione di tasti `ctrl + break` (`ctrl + interr` per
le tastiere italiane). In questo caso, invece, è stato necessario installare un **_ctrl handler_**.

Al verificarsi dell'evento specificato sopra, il comportamento per entrambe le piattaforme è lo stesso: si termina 
l'esecuzione di tutti i processi/thread figli, si chiudono le 2 socket create dal processo padre,
e si torna al _main_, dove viene riletto il file  configurazione e avviato il server con le nuove configurazioni.
### Parser
Sono stati creati molteplici parser per soddisfare varie necessità: per le richieste HTTP (che include la lettura e la
decodifica delle credenziali cifrate in base64, per cui è stato deciso di usare un modulo esterno riperito in rete), 
per creare risposte, per interpretare il file di configurazione e i parametri passati da linea di comando. 