import subprocess
import numpy as np
import scipy.stats as st
import matplotlib.pyplot as plt
import csv

# Configurazione (Incidente)
eseguibile_c = "./msq_ev_py.exe"  
repliche = 50

print(f"Avvio di {repliche} repliche per lo Scenario di Congestione Dinamica...")

# Liste per raccogliere i dati
tassi_offload = []
prob_violazioni = []
totali_offloaded = []
picchi_latenza = []
tempi_recupero_E = []
tempi_recupero_full = []
picchi_coda_N = []
picchi_coda_T = []

# Esecuzione delle simulazioni
for seed in range(1, repliche + 1):
    comando = [eseguibile_c, str(seed)]
    processo = subprocess.run(comando, capture_output=True, text=True)
    
    if processo.returncode != 0:
        print(f"Errore al seed {seed}")
        continue
        
    output = processo.stdout.strip()
    
    try:
        # Parsing dell'output CSV stampato dal C
        dati = output.split(',')
        totali_offloaded.append(int(dati[1]))
        tassi_offload.append(float(dati[2]))
        prob_violazioni.append(float(dati[4]))
        picchi_latenza.append(float(dati[5]))
        
        # Recovery time della sola coda Emergenza
        rec_time_E = float(dati[6])
        if rec_time_E >= 0:
            tempi_recupero_E.append(rec_time_E)

        # Recovery time dell'intero sistema (E, N e T tutte vuote)
        rec_time_full = float(dati[7])
        if rec_time_full >= 0:
            tempi_recupero_full.append(rec_time_full)

        picchi_coda_N.append(int(dati[8]))
        picchi_coda_T.append(int(dati[9]))
            
    except Exception as e:
        print(f"Errore parsing output {seed}: {output}")

# ANALISI STATISTICA DELL'IMPATTO
if len(tassi_offload) > 0:
    # Calcolo Medie
    media_offload = np.mean(tassi_offload)
    media_tot_off = np.mean(totali_offloaded)
    media_viol = np.mean(prob_violazioni)
    media_picco = np.mean(picchi_latenza)
    media_recupero_E    = np.mean(tempi_recupero_E) if len(tempi_recupero_E) > 0 else -1
    media_recupero_full = np.mean(tempi_recupero_full) if len(tempi_recupero_full) > 0 else -1

    # Intervalli di Confidenza (95%)
    ic_offload = st.t.interval(0.95, df=len(tassi_offload)-1, loc=media_offload, scale=st.sem(tassi_offload))
    ic_viol = st.t.interval(0.95, df=len(prob_violazioni)-1, loc=media_viol, scale=st.sem(prob_violazioni))
    ic_picco = st.t.interval(0.95, df=len(picchi_latenza)-1, loc=media_picco, scale=st.sem(picchi_latenza))
    
    print("\n" + "="*65)
    print(" IMPATTO DELL'INCIDENTE STRADALE (ANALISI TRANSITORIA)")
    print("="*65)
    
    print("1. EFFICACIA POLITICA DI OFFLOADING:")
    print(f"   Totale medio job deviati:  {media_tot_off:.0f} richieste")
    print(f"   Tasso Medio di Deviazione: {media_offload:.2f}%")
    print(f"   Intervallo Conf. (95%):    [{ic_offload[0]:.2f}%, {ic_offload[1]:.2f}%]")
    print("-" * 65)
    
    print("2. TENUTA DEL VINCOLO DI EMERGENZA:")
    print(f"   Prob. Media Violazione:    {media_viol:.4f}%")
    print(f"   Intervallo Conf. (95%):    [{ic_viol[0]:.4f}%, {ic_viol[1]:.4f}%]")
    print(f"   Picco massimo Latenza:     {media_picco:.2f} ms")
    print(f"   Intervallo Conf. (95%):    [{ic_picco[0]:.2f} ms, {ic_picco[1]:.2f} ms]")
    print("-" * 65)
    
    print("3. RESILIENZA E RECUPERO (RECOVERY TIME):")
    print("   a) Solo vincolo critico (coda Emergenza):")
    if media_recupero_E >= 0:
        ic_recupero_E = st.t.interval(0.95, df=len(tempi_recupero_E)-1, loc=media_recupero_E, scale=st.sem(tempi_recupero_E))
        print(f"      Tempo medio di recupero:   {media_recupero_E:.2f} ms")
        print(f"      Intervallo Conf. (95%):    [{ic_recupero_E[0]:.2f} ms, {ic_recupero_E[1]:.2f} ms]")
    else:
        print("      ATTENZIONE: la coda Emergenza non è tornata a regime stazionario entro lo STOP.")

    print("   b) Intero sistema (code E, N e T):")
    if media_recupero_full >= 0:
        ic_recupero_full = st.t.interval(0.95, df=len(tempi_recupero_full)-1, loc=media_recupero_full, scale=st.sem(tempi_recupero_full))
        print(f"      Tempo medio di recupero:   {media_recupero_full:.2f} ms")
        print(f"      Intervallo Conf. (95%):    [{ic_recupero_full[0]:.2f} ms, {ic_recupero_full[1]:.2f} ms]")
    else:
        print("      ATTENZIONE: il sistema non è mai tornato completamente alla normalità entro lo STOP.")
    print("-"*65)

    # Calcoli per le code secondarie
    media_picco_N = np.mean(picchi_coda_N)
    media_picco_T = np.mean(picchi_coda_T)
    # Se lo standard error è 0 (tutti i valori sono uguali), l'intervallo è il valore stesso
    err_N = st.sem(picchi_coda_N)
    if err_N == 0:
        ic_picco_N = (media_picco_N, media_picco_N)
    else:
        ic_picco_N = st.t.interval(0.95, df=len(picchi_coda_N)-1, loc=media_picco_N, scale=err_N)
    ic_picco_T = st.t.interval(0.95, df=len(picchi_coda_T)-1, loc=media_picco_T, scale=st.sem(picchi_coda_T))
    
    print("4. IMPATTO SULLE CODE SECONDARIE (STARVATION):")
    print(f"   Picco medio Coda Navigazione: {media_picco_N:.0f} pacchetti")
    print(f"   Intervallo Conf. (95%):       [{ic_picco_N[0]:.0f}, {ic_picco_N[1]:.0f}]")
    print(f"   Picco medio Coda Telemetria:  {media_picco_T:.0f} pacchetti")
    print(f"   Intervallo Conf. (95%):       [{ic_picco_T[0]:.0f}, {ic_picco_T[1]:.0f}]")
    print("="*65)

# GENERAZIONE GRAFICO DEL TRANSITORIO
print("\nGenerazione del grafico in corso...")
tempi, code_E, code_N, code_T, limiti_non_E = [], [], [], [], []

try:
    with open('transient_log.csv', mode='r') as file:
        lettore = csv.reader(file)
        next(lettore) # Salta l'intestazione
        for riga in lettore:
            tempi.append(float(riga[0]))
            code_E.append(int(riga[1]))
            code_N.append(int(riga[2]))
            code_T.append(int(riga[3]))
            limiti_non_E.append(int(riga[4]))

    fig, ax1 = plt.subplots(figsize=(12, 6))
    
    # Plot delle tre code (Asse Y Principale - Logaritmico)
    ax1.plot(tempi, code_E, color='red', label="Emergenza (Alta Prio)", linewidth=2)
    ax1.plot(tempi, code_N, color='blue', label="Navigazione (Media Prio)", alpha=0.8)
    ax1.plot(tempi, code_T, color='orange', label="Telemetria (Bassa Prio)", alpha=0.8)
    ax1.set_yscale('symlog')
    ax1.set_xlabel("Tempo di Simulazione (ms)", fontsize=12)
    ax1.set_ylabel("Numero di Job in Coda (Log)", fontsize=12, color='black')
    ax1.grid(True, linestyle=':', alpha=0.7)

    # Plot del Limite Hardware (Asse Y Secondario - Lineare)
    ax2 = ax1.twinx()  # Crea un secondo asse Y che condivide lo stesso asse X
    ax2.plot(tempi, limiti_non_E, color='purple', linestyle='-.', linewidth=2.5, label="Limite Server Non-Critici (K=9)")
    ax2.set_ylabel("Server Disponibili (Navigazione + Telemetria)", fontsize=12, color='purple')
    ax2.set_ylim(0, 14) # Da 0 a 13 server massimi
    ax2.tick_params(axis='y', labelcolor='purple')
    
    # Linee degli eventi
    ax1.axvline(x=1000000.0, color='black', linestyle='--', label="Inizio Incidente")
    ax1.axvline(x=1500000.0, color='green', linestyle='--', label="Fine Incidente")
    
    plt.title("Evoluzione Transitoria e Controllo a Isteresi (Trunk Reservation)", fontsize=14)
    
    # Unione delle legende dei due assi
    lines_1, labels_1 = ax1.get_legend_handles_labels()
    lines_2, labels_2 = ax2.get_legend_handles_labels()
    ax1.legend(lines_1 + lines_2, labels_1 + labels_2, loc='upper right')
    
    plt.savefig("grafico_incidente_trunk_reservation.png", dpi=300, bbox_inches='tight')
    plt.show()
    print("Grafico salvato come 'grafico_incidente_trunk_reservation.png'.")

except FileNotFoundError:
    print("Errore: Impossibile trovare il file transient_log.csv.")