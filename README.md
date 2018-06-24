# System-Programming-Project

## TODO
- [x] Creare un parser per le opzioni. (Antonio)
- [x] Creare server Multi-Thread Multi-processo (pre thread e pre processo, pre forkato) (Il processo sotto unix deve essere Deamon sotto windows no).
- [x] Creare parser per i file. (Giacomino)
- [x] Trovare o Creare un decodere Base64 per il punto successivo. (Antonio)
- [x] Creare parser per le richieste HTTP. (Attenzione username e password sono dentro Authorization: )
- [x] Handler per sighup per la versione unix e per windows evento di console.
- [x] Creare creatore dei log secondo lo standard sul testo di Bernaschi.
- [x] Gestione command compreso di funzione per spawnare un processo dandogli come input un comando, ritorna come output l'output del comando Gesitsce anche la sincronizzazione. Progettare con la possibilità futura di passare argomenti.
- [x] Gestire la connessione sulla porta 8081.
- [x] Creare un funzione che cifra un file che mappa in memoria, la cifratura é: xor bitwise usando srand(ip-client).
- [x] Gestire download e upload dei file GET PUT. Se viene passato con la GET un nome di una director deve restituire l'albero della directori (non ricorsivo)
- [x] Gestire l'accesso esclusivo dei file mediante lock.


Gestire per bene gli errori delle fwrite e delle send.

## File Rivisti
- main.c
- signal.c
- server.c
- operation.c `Da riguardare list dir per quanto riguarda le realloc`

## Idee
- [x] Gestire child terminate con un diverso meccanismo di sinctronizzazione
- [x] Controllare che la grandezza dell'header non superi il buffer
- [ ] Modificare list_dir e gestirla come send_file per non caricare tutto in memoria
- [ ] Ridefinire lo standard di passaggio dei comandi per /command/
- [ ] Ridefinire il meccanismo di lock dei file per renderlo universale (Win e Unix)

## Controllare 
- [ ] ToDo in ctrl_handler.
- [ ] Eccessivo ed esponenziale uso di memoria di exec_command.
- [ ] Gestire in modo migliore il passaggio della modalità in server.c