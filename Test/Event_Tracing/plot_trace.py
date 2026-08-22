import pandas as pd
import matplotlib.pyplot as plt

# Carica i dati generati dalla simulazione C
try:
    df = pd.read_csv("trace.csv")
except FileNotFoundError:
    print("Errore: File trace.csv non trovato. Esegui prima il programma C.")
    exit()

# Simensione del grafico
plt.figure(figsize=(14, 7))

# Step Plot
# Totale dei job nel sistema (in nero)
plt.step(df['Time'], df['Total_In_System'], where='post', 
         label='Totale nel Sistema (l(t))', color='black', linewidth=2)

# Server occupati
plt.step(df['Time'], df['Busy_Servers'], where='post', 
         label='Server Occupati', color='green', alpha=0.7, linestyle='--')

# Coda di Navigazione (soggetta a offloading)
plt.step(df['Time'], df['Queue_N'], where='post', 
         label='Coda Navigazione (q_N(t))', color='blue', alpha=0.7)

# Le altre code
plt.step(df['Time'], df['Queue_E'], where='post', label='Coda Emergenza', color='red', alpha=0.7)
plt.step(df['Time'], df['Queue_T'], where='post', label='Coda Telemetria', color='orange', alpha=0.7)

plt.title('Traiettoria di Stato - Simulazione Multi-Server con Priorità', fontsize=14, fontweight='bold')
plt.xlabel('Tempo di Simulazione (t)', fontsize=12)
plt.ylabel('Numero di Job', fontsize=12)

plt.legend(loc='upper right')
plt.grid(True, linestyle='--', alpha=0.5)
plt.tight_layout()

plt.show()