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

#define START    0.0                    /* initial (open the door)        */
#define STOP     10000000.0                /* terminal (close the door) time */
#define EVALUATION_END 3000000.0        /* Fine del conteggio per le statistiche (Previene la diluizione) */
#define SERVERS  10                      /* number of servers              */
#define MAX_Q_E 100000                 /* Dimensione massima buffer coda E */
#define ACCIDENT_TIME 1000000.0        /* Istante in cui si verifica l'incidente */
#define ACCIDENT_END  1500000.0        /* Fine incidente e ripristino viabilità */
#define WARMUP   500000.0 

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


   double GetArrival(double current_time)
{     
  static double arrival = START;
  SelectStream(0);  // Stream per gli arrivi

/* Il traffico quadruplica durante la finestra dell'incidente */
  if (current_time > ACCIDENT_TIME && current_time <= ACCIDENT_END) {
      arrival += Exponential(0.5); 
  } else {
      arrival += Exponential(2.0);
  }
  return arrival;
}

   
double GetService_E(void) {
     /* Emergenza: tempo deterministico */
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
     SelectStream(2);       // Stream per telemetria              
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


  int main(int argc, char *argv[]) {
// Acquisizione seed da Python
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

  // Variabili per il calcolo della latenza 
  double     queue_E_times[MAX_Q_E];             /* Array per i timestamp di arrivo in coda */
  long       head_E = 0, tail_E = 0;             /* Indici per gestire l'array come coda FIFO */
  
  double     server_job_arrival[SERVERS + 1];    /* Memorizza quando è arrivato il job attualmente nel server */
  int        server_job_class[SERVERS + 1];      /* Memorizza la classe del job attualmente nel server */
  
  long       completed_E = 0;                    /* Emergenze fisicamente completate */
  long       violations_E = 0;                   /* Emergenze che hanno superato i 15 ms */

  
  // Variabili per il calcolo delle medie (aree sotto la curva)
  double     area_qE = 0.0, area_qN = 0.0, area_qT = 0.0;

  struct {                           /* accumulated sums of                */
    double service;                  /*   service times                    */
    long   served;                   /*   number served                    */
  } sum[SERVERS + 1];

  PlantSeeds(seed);
  int accident_triggered = 0; // Variabile per il reset statistiche

  t.current    = START;
  event[0].t   = GetArrival(START);
  event[0].x   = 1;
  for (s = 1; s <= SERVERS; s++) {
    event[s].t     = START;          /* this value is arbitrary because */
    event[s].x     = 0;              /* all servers are initially idle  */
    sum[s].service = 0.0;
    sum[s].served  = 0;
    server_job_class[s]   = 0;       // Inizializza la classe a 0 (vuoto)
    server_job_arrival[s] = 0.0;     // Inizializza il timestamp a 0
  }

  double max_latency_E = 0.0;
  double recovery_time_E = -1.0;     /* tempo di ripristino della coda Emergenza */
  int    is_recovered_E = 0;

  double recovery_time_full = -1.0;   /* tempo di ripristino di tutte le code (E, N, T) */
  int    is_recovered_full  = 0;

  double baseline_E = 0.0, baseline_N = 0.0, baseline_T = 0.0;  /* livello medio pre-incidente (il sistema ha recuperato quando è tornato a questo) */
  int    baseline_computed = 0;

  double area_qE_warmup = 0.0, area_qN_warmup = 0.0, area_qT_warmup = 0.0; /* snapshot integrali a t=WARMUP */
  double warmup_time_actual = 0.0;   /* istante esatto in cui viene superato WARMUP */
  int    warmup_snapshot_done = 0;

  double next_log_time = START; // Per campionare la coda ogni tot ms
  long max_queue_N = 0; 
  long max_queue_T = 0;

  FILE *f_log = fopen("transient_log.csv", "w");
  fprintf(f_log, "Time,Queue_E,Queue_N,Queue_T\n");

  while ((event[0].x != 0) || (busy_servers > 0) || (queue_E > 0) || (queue_N > 0) || (queue_T > 0)) {
    e         = NextEvent(event);                  /* next event index */
    t.next    = event[e].t;                        /* next event time  */

    // Aggiornamento degli integrali per calcolare la lunghezza media delle code
    double dt = t.next - t.current;
    area_qE  += dt * queue_E;
    area_qN  += dt * queue_N;
    area_qT  += dt * queue_T;

    t.current = t.next;                            /* advance the clock*/

    // Scrittura dei dati per il grafico (campionamento ogni 1000 ms)
    if (t.current >= next_log_time) {
        fprintf(f_log, "%f,%ld,%ld,%ld\n", t.current, queue_E, queue_N, queue_T);
        next_log_time += 1000.0;
    }

    // Verifica del Recovery Time per coda E
    if (t.current > ACCIDENT_END && accident_triggered == 1 && is_recovered_E == 0) {
        if (queue_E == 0) {
            recovery_time_E = t.current - ACCIDENT_END;
            is_recovered_E = 1;
        }
    }

    // Verifica del Recovery Time per l'intero sistema: le code tornano al
    // livello medio osservato prima dell'incidente
    if (t.current > ACCIDENT_END && accident_triggered == 1 && is_recovered_full == 0) {
        if (queue_E <= baseline_E && queue_N <= baseline_N && queue_T <= baseline_T) {
            recovery_time_full = t.current - ACCIDENT_END;
            is_recovered_full = 1;
        }
    }

    // --- SNAPSHOT DEGLI INTEGRALI ALL'ISTANTE WARMUP ---
    // Serve come "punto di partenza" per calcolare il livello medio solo sulla
    // finestra [WARMUP, ACCIDENT_TIME], escludendo il transitorio iniziale
    if (t.current >= WARMUP && warmup_snapshot_done == 0) {
        area_qE_warmup    = area_qE;
        area_qN_warmup    = area_qN;
        area_qT_warmup    = area_qT;
        warmup_time_actual = t.current;
        warmup_snapshot_done = 1;
    }

    // --- CALCOLO MEDIE PRE-INCIDENTE (livello medio "normale" delle code) ---
    // Calcolato sulla finestra [WARMUP, ACCIDENT_TIME]
    if (t.current >= ACCIDENT_TIME && baseline_computed == 0) {
        double durata_baseline = t.current - warmup_time_actual;
        baseline_E = (area_qE - area_qE_warmup) / durata_baseline;
        baseline_N = (area_qN - area_qN_warmup) / durata_baseline;
        baseline_T = (area_qT - area_qT_warmup) / durata_baseline;
        baseline_computed = 1;
    }

    // --- RESET DELLE STATISTICHE ALL'ISTANTE DELL'INCIDENTE ---
    if (t.current >= ACCIDENT_TIME && accident_triggered == 0) {
        // Azzera tutti i contatori per misurare solo l'impatto della congestione
        arrivals_E = 0;
        arrivals_N = 0;
        arrivals_T = 0;
        completed_E = 0;
        violations_E = 0;
        offloaded_N = 0;
        max_queue_N = 0; 
        max_queue_T = 0;
        max_latency_E = 0;
        
        accident_triggered = 1; 
    }


    if (e == 0) {                                  /* process an arrival*/
      // Determina la classe del pacchetto con probabilità
      SelectStream(3);  
      double r = Random();
      int job_class;

      // Soglie di probabilità nominali
      double prob_E = 0.10; 
      double prob_N = 0.40; 

      // Soglie di probabilità durante l'incidente
      if (t.current > ACCIDENT_TIME && t.current <= ACCIDENT_END) {
          prob_E = 0.35; // I segnali di Emergenza balzano dal 10% al 35% del traffico totale
          prob_N = 0.85; // La Navigazione occupa il 50% del traffico, alla Telemetria resta il 15%
      }

      if (r < prob_E) { 
        job_class = 1; 
        if (t.current <= EVALUATION_END) arrivals_E++;
      }
      else if (r < prob_N) { 
        job_class = 2; 
        if (t.current <= EVALUATION_END) arrivals_N++; 
      }
      else { 
        job_class = 3; 
        if (t.current <= EVALUATION_END) arrivals_T++; 
      }        


      // Programma il prossimo arrivo globale 
      event[0].t = GetArrival(t.current);
      if (event[0].t > STOP) event[0].x = 0;

      // Logica di Offloading Dinamico per la Navigazione
      // Calcola l'attesa media stimata basandosi sui job a priorità uguale o superiore
      double expected_wait = ((queue_E * 5.0) + (queue_N * 30.0)) / SERVERS;
      
      if (job_class == 2 && expected_wait > 50.0) {
          if (t.current <= EVALUATION_END) offloaded_N++; // Conta l'offload solo nella finestra
          // Il job è inviato al nodo adiacente, non entra nel sistema
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
            else if (job_class == 2) {
                queue_N++;
                if (queue_N > max_queue_N) max_queue_N = queue_N; 
            }
            else {                     
                queue_T++;
                if (queue_T > max_queue_T) max_queue_T = queue_T; 
            }
          }
      }
    }
    else {                                         /* process a departure */
      index++;                                     /* from server s       */  
      s = e;   

      // Calcolo della latenza del job uscente
      if (server_job_class[s] == 1) {              // Se era un'Emergenza
          double w = t.current - server_job_arrival[s]; // Latenza totale = Ora - Arrivo
          

          // Valuta l'SLA solo se l'emergenza termina entro la finestra di valutazione
          if (t.current <= EVALUATION_END) {
              completed_E++;
              if (w > 15.0) {                  // Vincolo traccia: 15 ms
                violations_E++;
              }
              if (w > max_latency_E) {
                max_latency_E = w;    // Registra il picco massimo
              }
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

fclose(f_log);

// Calcolo delle metriche finali
  double tasso_offload = 0.0;
  if (arrivals_N > 0) {
      tasso_offload = ((double)offloaded_N / arrivals_N) * 100.0;
  }
  
  double prob_violation = 0.0;
  if (completed_E > 0) {
      prob_violation = ((double)violations_E / completed_E) * 100.0;
  }

  // Stampa CSV per Python
  // Ordine: Arrivi_N, Offload_N, Tasso%, Viol_E, Prob%, MaxLat_E, RecoveryTime_E, RecoveryTime_Full, MaxQ_N, MaxQ_T
  printf("%ld,%ld,%f,%ld,%f,%f,%f,%f,%ld,%ld\n", 
         arrivals_N, offloaded_N, tasso_offload, violations_E, prob_violation, max_latency_E, 
         recovery_time_E, recovery_time_full, max_queue_N, max_queue_T);
 
  return (0);
}
