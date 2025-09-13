import serial
import time
import requests




#U controllare che l'id_utente esista
#
url = "http://127.0.0.1/Z-planning/api/checkUtente.php"

ser = serial.Serial('COM3', 9600, timeout=1)
time.sleep(2)  # attesa reset Arduino

#messaggio = "M: Ciao Arduino!\n"
#ser.write(messaggio.encode('utf-8'))

try:
    while True:
        # Invio un comando
        #ser.write(b"ON\n")

        # Lettura eventuale risposta (anche se Arduino non stampa nulla)
        if ser.in_waiting > 0:
            ricevuto = ser.readline().decode().strip()+""
            print(ricevuto)
            if ricevuto[0] == "U":
                ricevuto = ricevuto[1:]
                data = {
                    "id_utente": ricevuto
                }
                print("Mediatore>> chiamata API a ", url)
                response = requests.post(url, json=data)
                response.raise_for_status()
                risultato = response.json()
                risultato = risultato.get("response", "niente")

                print("Mediatore>> Risposta API:", risultato)
                print("Mediatore>> invio ad Arduino: ", risultato)
                ser.write(risultato.encode('utf-8'))

        time.sleep(1)

except KeyboardInterrupt:
    ser.close()