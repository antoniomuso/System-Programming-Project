# Relazione Progetto Programmazione di Sistema (A.A 2017-2018)
Antonio Musolino, Giacomo Priamo  

## Introduzione
Il progetto è stato sviluppato e testato contemporaneamente su un sistema Unix e uno Windows, il che ci ha permesso di 
poter gestire da subito gli errori riscontrati sulle diverse piattaforme, di non dover tornare 
sulle stesse funzioni in un secondo momento per re-implementare le stesse funzionalità 
e di massimizzare il riutilizzo del codice.

Le funzionalità fondamentali e critiche, come la creazione e gestione del server pre-forkato, la gestione delle 
richieste HTTP in accordo con le specifiche e la di gestione segnali ed eventi, sono state realizzate in presenza di 
entrambi in modo  simile alla tecnica di sviluppo "[Pair Programming](https://en.wikipedia.org/wiki/Pair_programming)"; 
mentre quelle secondarie, come i vari parser, sono state sviluppate individualmente.  

## Implementazione
Per aumentarne la leggibilità e la modularità, è stato deciso di organizzare il codice in file separati, 
ognuno con un proprio header, che vengono compilati assieme automaticamente tramite il _makefile_.

### Compilazione
Per compilare l'intero programma è sufficiente posizionarsi nella directory del progetto e usare il _makefile_:

- Nel caso di Unix, è sufficiente chiamare `make`.
- In Windows, è possibile invocare il compilatore digitando `mingw32-make.exe` 

### Lancio
È possibile avviare il programma eseguendo il programma _main_ (`./main` su Unix e `main.exe` su Windows). Senza parametri,
verranno caricate le impostazioni specificate nel file di configurazione _config.txt_, oppure è possibile specificare 
manualmente i parametri, tutti o in parte (in quest'ultimo caso, i restanti valori saranno estratti dal file di 
configurazione), secondo la seguente sintassi di lancio  da linea di comando:
- `-server_ip <ip>` per specificare l'indirizzo IP su cui aprire la socket.
- `-port <numero_porta>` per specificare la porta a cui legarsi, il server si legherà automaticamente alla porta 
successiva allo scopo di poter scaricare i file richiesti dopo che essi siano stati cifrati come da specifiche.
- `-mode <modalità>` per specificare la modalità di funzionamento del server: **MT** per multi thread e **MP** per
multi processo.
- `-n_proc <numero>` per specificare quanti thread/processi (a seconda della modalità scelta) figli creare.
### Server
Su Unix, il codice è scritto in modo tale che il server nasca come **_processo demone_** (la daemonizzazione è 
effettuata nel file `main.c`), mentre su Windows esso nascerà come un normale processo.
##### Avvio del server
Dopo aver caricato i vari parametri, si procede con la configurazione del server partendo con la creazione di 2 socket: 
una "normale" e l'altra per le sole operazioni di _GET_ per solo file preventivamente cifrati.
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
Tutti i processi/thread figli eseguono la stessa funzione: `process_routine`, che riceve in input le due socket create in
precedenza dal processo padre e in cui è specificata la modalità di gestione delle richieste in arrivo da
parte di client esterni. Al fine di evitare che i figli si blocchino su una sola `accept`, 
si è deciso utilizzare la funzione `select` per risvegliare i processi appena si verifica un cambiamento su una delle 
due socket, ciò permette di evitare un'attesa attiva dei figli, che sarebbe troppo dispendiosa.  
Una volta ricevuta una richiesta, si verificano che le credenziali sottomesse siano corrette confrontandole con quelle
presenti nel file `passwordFile.txt` e si procede con la sua gestione. Per questioni di efficienza si è deciso di 
leggere tale file solo una volta, ossia all'avvio dei thread/processi.
### Operazioni
Per relizzare l'accesso esclusivo ai file sono state usate la funzione `flock` per Unix e `[Un]LockFileEx` per Windows, 
le operazioni di file locking e unlocking sono state universalizzate nelle funzioni `lock_file` e `unlock_file`.
Per le richieste di GET si è deciso di rendere le richieste di lock sui file bloccanti, mentre per quelle di PUT verrà 
restituito un errore al client nel caso in cui la risorsa sia già bloccata.

**NOTA**: L'attuale implementazione del sistema dà la possibilità ad entrambe le richieste di GET e PUT di avere accesso
completo all'intero filesystem del sistema operativo, ossia non è stata inserita alcuna sorta di restrizione al campo 
di azione del programma all'interno di una zona specifica e debitamente limitata (come una _root directory_ 
"fittizia" dentro cui confinare le operazioni messe a disposizione dal server stesso) come normalmente avviene nei 
server HTTP. È stato deciso di non prendere provvedimenti al riguardo sia perché il "problema" è stato discusso 
solamente il giorno precedente alla consegna del progetto, ma soprattutto perché, consultando le specifiche, non si è 
evinta la necessità di gestire la sicurezza del programma in termini di permessi e controllo degli accessi. 
Infatti, durante la realizzazione del progetto, abbiamo deciso di focalizzare la nostra attenzione sul funzionamento e
la gestione corretti, coerenti ed efficienti del server e delle funzionalità richieste, piuttosto che concentrarsi su 
aspetti che ci sono risultati secondari o non rilevanti dopo un'accurata analisi delle specifiche.
##### GET
Le richieste di GET sulla porta principale (quella specificata in input o dal file di configurazione) sono gestitite 
dalla funzione `send_file`.
###### File
Se il path richiesto corrisponde a un file, la funzione `send_file` lo apre, acquisisce il lock su di esso e, dopo 
averlo letto, lo invia al client richiedente. La lettura del file è eseguita a blocchi di dimensione massima prefissata,
i quali vengono inviati una volta raggiunta tale dimensione; il procedimento continua finquando non è stato letto e 
inviato l'intero contenuto del file.
###### Directory
Nel caso in cui il path richiesto corrisponde a una directory, viene invocata la funzione `list_dir` (che restituisce il 
**contenuto della directory** indicata), il cui risultato viene inviato al client come risposta.
##### GET con cifratura
Le richieste di GET per i file sulla porta che prevede la cifratura preventiva del contenuto del file stesso sono gestite dalla funzione
`send_file_cipher`. Dopo essere stato aperto, il file è mappato in memoria grazie alle apposite funzioni dei due sistemi
operativi, successivamente si procede con la cifratura tramite la funzione `encrypt`, che manipola blocchi di dimensione 
pari a 4 byte del file mappato  e li cifra mediante lo _XOR bitwise_ con un intero random avente come seme l'indirizzo IP del 
client. 
Nel caso in cui la dimensione del file non fosse divisibile per 4, è stato deciso di aggiungere un **padding** di zeri 
(0) all'ultimo blocco al fine di effettuare correttamente la cifratura, tale **padding** tuttavia non è né mappato in 
memoria né tantomeno inviato inviato al client. Il risultato di tali operazioni viene inviato al client al termine delle
stesse.
##### Esecuzione comandi
Come da specifiche, nel caso in cui il primo elemento del path contenga la stringa **command** viene eseguita la 
funzione `exec_command`, la quale crea un thread che andrà a generare il processo che eseguirà il comando passato in 
input. Come meccanismi di sincronizzazione, sono stati usati una **_condition variable_** per Unix e un **_Evento_** per
Windows. Per entrambe le piattaforme si è deciso di usare il meccanismo delle **_pipe_** per redirezionare l'output dei
processi che eseguono il comando, al fine di inviarne il contenuto al client una volta completata l'esecuzione del 
thread. Nonostante fossimo a conoscenza di `popen`, per rimanare coerenti con il passaggio dei comandi e 
per evitare il collegamento con una shell comportato dall'uso di tale funzione, abbiamo deciso di gestire manualmente
e separatamente nei due sistemi operativi la creazione dei processi e la redirezione delle pipe.  

L'implementazione di `exec_command` supporta anche il **passaggio di parametri**, secondo la seguente sintassi:
`[...]/command/arg1?arg2?[...]?argN`, dove nel caso di passaggio di comandi legati ai terminali 
(es: _cat_, _ipconfig_, _echo_) il primo argomento deve corrispondere, sia per Unix che per Windows, allo stesso valore
di _command_. La scelta di non inserire automaticamente il nome del comando come primo argomento è stata presa
al fine di poter eseguire anche comandi che non seguono lo standard di lancio dei due sistemi operativi.

Esempi:
- Unix: `/command/date?date`, `/command/cat?cat?config.txt` 
- Windows: `/command/C:/Windows/System32/cmd.exe?C:\Windows\System32\cmd.exe?/k?ipconfig` 

##### PUT
Le richieste di PUT vengono gestite dalla funzione `put_file`, che dopo aver creato un file con lo stesso nome di quello
che il client sta caricando ed ottenuto il lock su di esso, legge e trascrive localmente il contenuto che gli viene 
inviato. Nel caso in cui esistesse già un file con lo stesso nome di quello che il client sta caricando, il file 
esistente viene sovrascritto. 
È anche possibile specificare il nome che il file che verrà caricato scrivendolo nel _path_ dell'_url_. 
##### Logging
Come da specifiche, tutte le richieste sono loggate all'interno del file _log.txt_ secondo il formato stabilito dal 
_common log format_. Nel caso di fallimento della funzione di logging, è stato deciso di non riportare gli errori.

### Segnali ed Eventi da console
Come da specifiche, il server è in grado di rileggere il file di configurazione dopo l'avvenimento di uno specifico 
evento:
- Su Unix, ciò avviene quando il server (processo padre) riceve il segnale `SIGHUP`, per il quale è stato quindi necessario 
installare un **_signal handler_**. Una volta ricevuto tale segnale, viene inviato ai thread/processi figli il segnale
`SIGUSR1` (che ha quindi richiesto l'implementazione di un ulteriore **_signal handler_**), che li spinge a rilasciare
le risorse acquisite (tra cui le socket) e terminare la propria esecuzione.

- Su Windows, ciò avviene quando nella console è premuta la combinazione di tasti `ctrl + break` (`ctrl + interr` per
le tastiere italiane), perciò è stato necessario installare un **_ctrl handler_**. Al  fine di 
permettere ai thread/processi "figli" di rilasciare le proprie risorse in modo corretto (come nel caso di Unix), 
è stato inserito come meccanismo di segnalazione un **_Evento_**, che viene segnalato nella routine associata 
al **_ctrl handler_**  e resettato ogni volta che viene rilanciato il server stesso.
 
Al verificarsi dell'evento specificato sopra, il comportamento per entrambe le piattaforme è lo stesso: si attende il
termine  dell'esecuzione di tutti i processi/thread figli, si chiudono le 2 socket create dal processo padre,
e si torna al _main_, dove vengono riletti il file  configurazione e quello delle password, e infine vine avviato il 
server con le nuove impostazioni.
### Parser
Sono stati creati molteplici parser per soddisfare varie necessità: per le richieste HTTP (che include la lettura e la
decodifica delle credenziali cifrate in _base64_, per cui è stato deciso di usare un modulo esterno riperito in rete), 
per creare risposte HTTP, per interpretare il file di configurazione, quello delle credenziali (che per comodità seguono 
lo stesso formato: `nome=valore` \ `username=password`) e i parametri passati da linea di comando.
### Terminazione
La terminazione del programma avviene in modi diversi a seconda della piattaforma:
- Nel caso di Windows, è sufficiente usare la combinazione di tasti `ctrl + C`.
- In Unix è necessario inviare il segnale `SIGTERM` al processo **padre**, l'handler installato si occuperà di terminare
correttamente tutti i figli.