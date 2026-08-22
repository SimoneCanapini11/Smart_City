/* ------------------------------------------------------------------------- 
 * This program is a next-event simulation of a multi-server, single-queue 
 * service node.  The service node is assumed to be initially idle, no 
 * arrivals are permitted after the terminal time STOP and the node is then 
 * purged by processing any remaining jobs. 
 * 
 * Name              : msq.c (Multi-Server Queue)
 * Author            : Steve Park & Dave Geyer 
 * Language          : ANSI C 
 * Latest Revision   : 10-19-98 
 * ------------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "rngs.h"

#define START        0.0                    /* initial (open the door)        */
#define WARMUP       500000.0             /* Soglia stimata dal grafico di convergenza */
#define BATCH_SIZE   500000.0            /* Ampiezza di ogni singolo blocco */
#define NUM_BATCHES  50                  /* Numero di blocchi da raccogliere */
#define STOP         (WARMUP + (NUM_BATCHES * BATCH_SIZE)) 
#define SERVERS      9                     /* number of servers              */
#define MAX_Q_E      100000                /* Dimensione massima buffer coda E */

#define MAX_LATENZE 2000000
double latenze_E[MAX_LATENZE];
long   num_latenze = 0;

typedef struct {                        /* the next-event list    */
  double t;                             /*   next event time      */
  int    x;                             /*   event status, 0 or 1 */
} event_list[SERVERS + 1];              


   double Exponential(double m)
/* ---------------------------------------------------
 * generate an Exponential random variate, use m > 0.0 
 * ---------------------------------------------------
 */
{
  return (-m * log(1.0 - Random()));
}


   double Uniform(double a, double b)
/* --------------------------------------------
 * generate a Uniform random variate, use a < b 
 * --------------------------------------------
 */
{
  return (a + (b - a) * Random());
}


   double GetArrival(void)
/* ---------------------------------------------
 * generate the next arrival time, with rate 1/2
 * ---------------------------------------------
 */ 
{     
  static double arrival = START;

  SelectStream(0);
  arrival += Exponential(2.0);
  return (arrival);
}

   
double GetService_E(void) {
     /* Emergenza: tempo deterministico veloce */
     return 5.0; 
}

double GetService_N(void) {                 
     /* Navigazione: distribuzione Iperesponenziale a 2 fasi (H2)
      * Media totale attesa = 30 ms, varianza elevata.
      * L'80% delle richieste è veloce (media 10 ms), il 20% è lenta (media 110 ms).
      */
     
     SelectStream(1); // Stream dedicato alla navigazione
     
     double p1 = 0.8;
     double mu_1 = 10.0;
     double mu_2 = 110.0;
     
     // Estrae un numero casuale per decidere la "fase" del job
     double r = Random();
     
     // Genera il tempo di servizio basandosi sulla fase scelta
     if (r <= p1) {
         return Exponential(mu_1); // Fase 1: Richiesta veloce
     } else {
         return Exponential(mu_2); // Fase 2: Richiesta lenta
     }
}

double GetService_T(void) {                 
     /* Telemetria: tempo esponenziale */
     SelectStream(2);                            
     return Exponential(10.0);    
} 


   int NextEvent(event_list event)
/* ---------------------------------------
 * return the index of the next event type
 * ---------------------------------------
 */
{
  int e;                                      
  int i = 0;

  while (event[i].x == 0)       /* find the index of the first 'active' */
    i++;                        /* element in the event list            */ 
  e = i;                        
  while (i < SERVERS) {         /* now, check the others to find which  */
    i++;                        /* event type is most imminent          */
    if ((event[i].x == 1) && (event[i].t < event[e].t))
      e = i;
  }
  return (e);
}


   int FindOne(event_list event)
/* -----------------------------------------------------
 * return the index of the available server idle longest
 * -----------------------------------------------------
 */
{
  int s;
  int i = 1;

  while (event[i].x == 1)       /* find the index of the first available */
    i++;                        /* (idle) server                         */
  s = i;
  while (i < SERVERS) {         /* now, check the others to find which   */ 
    i++;                        /* has been idle longest                 */
    if ((event[i].x == 0) && (event[i].t < event[s].t))
      s = i;
  }
  return (s);
}


  int main(int argc, char *argv[])
{
  // Acquisizione del seed da linea di comando 
  long seed = 0; 
  if (argc == 2) {
      seed = atol(argv[1]);
  } else {
      printf("Errore: devi passare il seed. Uso: %s <seed>\n", argv[0]);
      return 1;
  }
  struct {
    double current;                  /* current time                       */
    double next;                     /* next (most imminent) event time    */
  } t;
  event_list event;
  int        e;                      /* next event index                   */
  int        s;                      /* server index                       */
  long       index  = 0;             /* used to count processed jobs       */

// Variabili di stato per le code e i server
  long       queue_E = 0, queue_N = 0, queue_T = 0; 
  int        busy_servers = 0;       
  
  // Contatori per le statistiche
  long       arrivals_E = 0, arrivals_N = 0, arrivals_T = 0;
  long       offloaded_N = 0;

  // Variabili per il calcolo della latenza (OBIETTIVO 1)
  double     queue_E_times[MAX_Q_E];             /* Array per i timestamp di arrivo in coda */
  long       head_E = 0, tail_E = 0;             /* Indici per gestire l'array come coda FIFO */
  
  double     server_job_arrival[SERVERS + 1];    /* Memorizza quando è arrivato il job attualmente nel server */
  int        server_job_class[SERVERS + 1];      /* Memorizza la classe del job attualmente nel server */
  
  long       completed_E = 0;                    /* Emergenze fisicamente completate */
  long       violations_E = 0;                   /* Emergenze che hanno superato i 15 ms */

  int        warmup_done = 0;                    /* Flag per l'azzeramento statistico */
  double     next_batch_time = WARMUP + BATCH_SIZE; /* Traguardo temporale del 1° blocco */
  int        current_batch = 0;                     /* Contatore dei blocchi completati */

  // Variabili per il calcolo delle medie (aree sotto la curva)
  double     area_qE = 0.0, area_qN = 0.0, area_qT = 0.0;

  struct {                           /* accumulated sums of                */
    double service;                  /*   service times                    */
    long   served;                   /*   number served                    */
  } sum[SERVERS + 1];

  PlantSeeds(seed);
  t.current    = START;
  event[0].t   = GetArrival();
  event[0].x   = 1;
  for (s = 1; s <= SERVERS; s++) {
    event[s].t     = START;          /* this value is arbitrary because */
    event[s].x     = 0;              /* all servers are initially idle  */
    sum[s].service = 0.0;
    sum[s].served  = 0;
    server_job_class[s]   = 0;       // Inizializza la classe a 0 (vuoto)
    server_job_arrival[s] = 0.0;     // Inizializza il timestamp a 0
  }

  while ((event[0].x != 0) || (busy_servers > 0) || (queue_E > 0) || (queue_N > 0) || (queue_T > 0)) {
    e         = NextEvent(event);                  /* next event index */
    t.next    = event[e].t;                        /* next event time  */

    // Aggiornamento degli integrali per calcolare la lunghezza media delle code
    double dt = t.next - t.current;
    area_qE  += dt * queue_E;
    area_qN  += dt * queue_N;
    area_qT  += dt * queue_T;

    t.current = t.next;                            /* advance the clock*/

    // --- GESTIONE WARM-UP (TRONCAMENTO DATI) ---
    if (t.current >= WARMUP && warmup_done == 0) {
        // Il sistema è a regime. Azzeriamo i contatori di latenza accumulati finora.
        completed_E = 0;
        violations_E = 0;
        warmup_done = 1; // Blocchiamo l'if per non azzerare più
    }

    // --- GESTIONE BATCH MEANS (RACCOLTA E RESET STATISTICO) ---
    if (warmup_done == 1 && t.current >= next_batch_time && current_batch < NUM_BATCHES) {
        
        // Calcola la statistica del blocco appena concluso
        double prob_violation = 0.0;
        if (completed_E > 0) {
            prob_violation = ((double)violations_E / completed_E) * 100.0;
        }
        
        // Stampa il dato (separato da virgola per Python)
        if (current_batch == NUM_BATCHES - 1) {
            printf("%f\n", prob_violation); // Ultimo batch va a capo
        } else {
            printf("%f,", prob_violation);  // Gli altri separati da virgola
        }

        // Azzera solo i contatori statistici per il prossimo blocco
        completed_E = 0;
        violations_E = 0;

        // Aggiorna i traguardi per il blocco successivo
        next_batch_time += BATCH_SIZE;
        current_batch++;
    }

    if (e == 0) {                                  /* process an arrival*/
      // Determina la classe del pacchetto con probabilità
      SelectStream(3);
      double r = Random();
      int job_class;

      if (r < 0.10) { 
        job_class = 1; arrivals_E++; 
      }
      else if (r < 0.40) { 
        job_class = 2; arrivals_N++; 
      }
      else { 
        job_class = 3; arrivals_T++; 
      }        

      // Programma il prossimo arrivo globale 
      event[0].t = GetArrival();
      if (event[0].t > STOP) event[0].x = 0;

      // Logica di Offloading Dinamico per la Navigazione
      // Calcola l'attesa media stimata basandosi sui job a priorità uguale o superiore
      double expected_wait = ((queue_E * 5.0) + (queue_N * 30.0)) / SERVERS;
      
      if (job_class == 2 && expected_wait > 50.0) {
          offloaded_N++;
          // Il job è scartato/inviato al nodo adiacente, non entra nel sistema
      }  else {
          // Se c'è un server libero avvia subito il servizio
          if (busy_servers < SERVERS) {
            busy_servers++;
            s = FindOne(event);

            // Registriamo chi è entrato e quando è arrivato nel sistema
            server_job_class[s] = job_class;       
            server_job_arrival[s] = t.current;
            
            double service;
            if (job_class == 1)      service = GetService_E();
            else if (job_class == 2) service = GetService_N();
            else                     service = GetService_T();

            sum[s].service += service;
            sum[s].served++;
            event[s].t      = t.current + service;
            event[s].x      = 1;
          } 
          // Altrimenti metti in coda nella rispettiva classe
          else {
            if (job_class == 1) {
                // Salva il tempo di arrivo esatto di questa Emergenza nell'array
                queue_E_times[tail_E % MAX_Q_E] = t.current; 
                tail_E++;                                    
                queue_E++;
            }
            else if (job_class == 2) queue_N++;
            else                     queue_T++;
          }
      }
    }
    else {                                         /* process a departure */
      index++;                                     /* from server s       */  
      s = e;   

      // Calcolo della latenza del job uscente
      if (server_job_class[s] == 1) {              // Se era un'Emergenza
          completed_E++;
          double w = t.current - server_job_arrival[s]; // Latenza totale = Ora - Arrivo
          if (w > 15.0) {                          // Vincolo traccia: 15 ms
              violations_E++;
          }
          if (warmup_done == 1 && num_latenze < MAX_LATENZE) {
              latenze_E[num_latenze++] = w;   // salva il dato grezzo
          }
      }                    
      
      // Scheduling Prioritario
      if (queue_E > 0) {
        queue_E--;

        // Carica i dati del job prelevato dalla coda nel server
        server_job_class[s] = 1;                                      
        server_job_arrival[s] = queue_E_times[head_E % MAX_Q_E];      
        head_E++;

        double service = GetService_E();
        sum[s].service += service;
        sum[s].served++;
        event[s].t = t.current + service;
      } 
      else if (queue_N > 0) {
        queue_N--;
        server_job_class[s] = 2; // Aggiorna solo la classe

        double service = GetService_N();
        sum[s].service += service;
        sum[s].served++;
        event[s].t = t.current + service;
      } 
      else if (queue_T > 0) {
        queue_T--;
        server_job_class[s] = 3; // Aggiorna solo la classe

        double service = GetService_T();
        sum[s].service += service;
        sum[s].served++;
        event[s].t = t.current + service;
      } 
      else {
        // Se tutte le code sono vuote il server torna idle
        busy_servers--;
        event[s].x = 0;
      }
    }
  }

  FILE *fp = fopen("latenze.csv", "w");
  if (fp == NULL) {
      fprintf(stderr, "Errore: impossibile aprire latenze.csv per la scrittura\n");
  } else {
      for (long i = 0; i < num_latenze; i++) {
          fprintf(fp, "%f\n", latenze_E[i]);
      }
      fclose(fp);
  }

  return (0);
}
