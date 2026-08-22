# Simulatore Nodo Edge V2X - Analisi Prestazionale

Questo repository contiene il codice sorgente e gli script di automazione per la simulazione ad eventi discreti di un nodo Edge in un ecosistema Smart City. Il progetto modella le prestazioni del sistema a regime, valuta la resilienza durante una congestione transitoria (incidente stradale) e implementa una soluzione architetturale evoluta per garantire i vincoli SLA.

## Struttura del Repository

Il modello simulato, e le sue differenti configurazioni, sono situati all'interno della directory `Modello/`. Per ogni scenario analizzato, la rispettiva cartella contiene il motore di simulazione scritto in **C** e gli script **Python** utilizzati per automatizzare le esecuzioni, gestire i seed e raccogliere i dati.

### Modello

*   **`Obiettivo1/`**
    *   Modello base utilizzato per l'analisi a regime (Steady-State). 
    *   Implementa la logica per la simulazione a orizzonte infinito utilizzando il metodo **Batch Means**.

*   **`Obiettivo2/`**
    *   Modello configurato per lo *Stress Test* su orizzonte temporale finito.
    *   Simula l'onda d'urto dell'incidente stradale. Lo script Python automatizza l'esecuzione di **Repliche Indipendenti** passando seed incrementali.
      
*   **`Migliorato/`**
    *   Evoluzione del modello.
    *   Implementa un Controller di Stato con logica di *Adaptive Trunk Reservation*. Include il codice per il limite dinamico dei server, il controllo a doppia soglia e il *Soft Recovery* post-crisi.

