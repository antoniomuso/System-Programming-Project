# System-Programming-Project

## TODO
- [x] Creare un parser per le opzioni. (Antonio)
- [x] Creare server Multi-Thread Multi-processo (pre thread e pre processo, pre forkato) (Il processo sotto unix deve essere Daemon, sotto windows no).
- [x] Creare parser per i file. (Giacomino)
- [x] Trovare o Creare un decodere Base64 per il punto successivo. (Antonio)
- [x] Creare parser per le richieste HTTP. (Attenzione username e password sono dentro Authorization: )
- [x] Handler per sighup per la versione unix e per windows evento di console.
- [x] Creare creatore dei log secondo lo standard sul testo di Bernaschi.
- [x] Gestione command compreso di funzione per spawnare un processo dandogli come input un comando, ritorna come output l'output del comando Gesitsce anche la sincronizzazione. Progettare con la possibilità futura di passare argomenti.
- [x] Gestire la connessione sulla porta 8081.
- [x] Creare un funzione che cifra un file che mappa in memoria, la cifratura é: xor bitwise usando srand(ip-client).
- [x] Gestire download e upload dei file GET PUT. Se viene passato con la GET un nome di una director deve restituire l'albero della directory (non ricorsivo)
- [x] Gestire l'accesso esclusivo dei file mediante lock.


Gestire per bene gli errori delle fwrite e delle send.

## File Rivisti
- main.c
- signals.c
- server.c
- operations.c 
- windows_process_exe.c
- command_parser.c

## Idee
- [x] Gestire child terminate con un diverso meccanismo di sinctronizzazione
- [x] Controllare che la grandezza dell'header non superi il buffer
- [x] Modificare list_dir e gestirla come send_file per non caricare tutto in memoria
- [x] Ridefinire lo standard di passaggio dei comandi per /command/
- [x] Ridefinire il meccanismo di lock dei file per renderlo universale (Win e Unix)
- [ ] Creare funzione per logging errori interni del server (fprintf -> logerror).

## Controllare 
- [x] ToDo in ctrl_handler.
- [x] Gestire in modo migliore il passaggio della modalità in server.c
- [x] Eccessivo ed esponenziale uso di memoria di exec_command.
- [x] Finire di rivedere command_parser.c
- [x] Doppio controllo (potenzialmente uguale) in server.c: 345, 358
- [x] Gestire, restituendo errore, il caso di PUT e /command/ nel caso in cui la richiesta sia effettuata sulla socket
cypher.


## Test Finali
- [ ] Funzionamento **in remoto e come demone** Unix
- [ ] Funzionamento Windows 
