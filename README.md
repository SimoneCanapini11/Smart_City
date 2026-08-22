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

### Test

Contiene gli script utilizzati per garantire la correttezza logica del simulatore e la sua aderenza al modello teorico.

*   **`Convergenza/`**
    *   Script per la raccolta e l'analisi dei dati necessari a valutare la fase di riscaldamento (*Warm-up*). Permette di tracciare il grafico di convergenza per assicurare il raggiungimento delle condizioni di Steady-State.
      
*   **`Event_Tracing/`**
    *   Script dedicato al tracciamento passo-passo (log) degli eventi. Raccoglie e stampa lo stato delle variabili interne del sistema per verificare a mano la corretta evoluzione logica delle code e del server.

*   **`RNG/`**
    *   Test dei Generatori di Numeri Casuali (PRNG). Verifica che le sequenze e le variate generate rispettino fedelmente le distribuzioni statistiche attese per il traffico veicolare e i tempi di servizio.
