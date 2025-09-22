import threading

import requests
import time
import serial
import sched
from datetime import datetime

ser = serial.Serial("COM3", 9600, timeout=1)
time.sleep(2)
url = "http://127.0.0.1/SMART-planning_db/api/"

matrice_led = [
    ["1A1", "30"], ["2A1", "31"], ["3A1", "32"], ["4A1", "33"], ["5A1", "34"], ["6A1", "35"], ["7A1", "36"], ["8A1", "37"],
    ["6A2", "60"], ["7A2", "61"], ["8A2", "62"], ["9A2", "63"], ["10A2", "64"], ["11A2", "65"], ["12A2", "66"], ["13A2", "67"],
    ["14A2", "50"], ["15A2", "51"], ["16A2", "52"], ["17A2", "53"], ["18A2", "54"], ["19A2", "55"], ["20A2", "56"], ["21A2", "57"],
    ["9A1", "40"], ["10A1", "41"], ["11A1", "42"], ["12A1", "43"], ["22A2", "44"], ["23A2", "45"], ["24A2", "46"], ["25A2", "47"],
    ["13A1", "70"], ["14A1", "71"], ["15A1", "72"], ["16A1", "73"], ["17A1", "74"], ["18A1", "75"], ["19A1", "76"], ["20A1", "77"],
    ["1A2", "20"], ["2A2", "21"], ["3A2", "22"], ["4A2", "23"], ["5A2", "24"],
    ["26A2", "10"], ["27A2", "11"], ["28A2", "12"], ["29A2", "13"], ["30A2", "14"],
]

def postazioni(id_badge: str) -> None:
    oggi = datetime.today()
    data_formattata = oggi.strftime("%Y-%m-%d")
    print(data_formattata)
    data = {
        "id_badge": id_badge,
        "data": data_formattata
    }
    try:
        response = requests.get(url+"getPrenotazioniPostazioniFromIdBadge.php", json = data)
        response.raise_for_status()
        risposta = response.json()
        print(risposta)

        if risposta['errore'] == "errore":
            msg_to_display = "D:ERRORE/badge non valido :("
            ser.write(msg_to_display.encode("utf-8"))
            print(msg_to_display)
            return

        msg_to_display = ""
        msg_to_display = "benvenuto " + risposta['username']
        if risposta['postazioni'] == "NONE":
            msg_to_display += "/    non hai nessuna/prenotazione oggi"
        else:
            msg_to_display += "/oggi hai prenotato/"
            postazioni = risposta['postazioni'].split(';')
            postazioni.pop()
            print("postazioni:  ", postazioni)

            for postazione in postazioni:
                msg_to_display += postazione + "  "

        msg_to_display = 'D:' + msg_to_display + '\n'
        print(msg_to_display)
        ser.write(msg_to_display.encode("utf-8"))
        ser.write("SI\n".encode("utf-8"))
        print(msg_to_display)
        time.sleep(0.2)

    except requests.RequestException as e:
        print("Errore nella richiesta:", e)
    return

def parcheggio(id_badge: str) -> None:
    oggi = datetime.today()
    data_formattata = oggi.strftime("%Y-%m-%d")
    data = {
        "id_badge" : id_badge,
        "data" : data_formattata
    }
    try:
        response = requests.get(url+"getPrenotazioniParcheggioFromIdBadge.php", json = data)
        response.raise_for_status()
        risposta = response.json()
        print(risposta)

        if risposta['errore'] == "errore":
            return

        if risposta['msg'] == "errore":
            return

        msg = ""
        if risposta['msg'] == "OK":
            msg = "SP"

            ser.write(msg.encode("utf-8"))

    except requests.RequestException as e:
        print("Errore nella richiesta:", e)
    return

def postazioniAttive() -> None:


    oggi = datetime.today()
    data_formattata = oggi.strftime("%Y-%m-%d")
    data = {
        "data":data_formattata
    }
    try:
        response = requests.get(url+"getPostazioniAttive.php", json = data)
        response.raise_for_status()
        risposta = response.json()
        print(risposta)

        if risposta['errore'] == "errore":
            return
        postazioni = risposta['postazioni'].split(';')
        print(postazioni)

        msg = "M"
        i = 0
        index_matrice = 0
        for postazione in postazioni:
            for coordinate in matrice_led:
                if coordinate[0] == postazione:
                    msg+=""+coordinate[1]


        print(msg)
        ser.write(msg.encode("utf-8"))

    except requests.RequestException as e:
        print("Errore nella richiesta:", e)
    return

def cmdDetector(msg: str) -> None: #gli viene passata la stringa ricevuta in seriale, riconosce e esegue il comando
    cmd = msg.split(':')
    print(msg)
    if cmd[0] == "POS":
        postazioni(cmd[1])
        return
    if cmd[0] == "PAR":
        parcheggio(cmd[1])
        return
    return

def esegui_periodicamente(intervallo=60):
    postazioniAttive()
    threading.Timer(intervallo, esegui_periodicamente, [intervallo]).start()

 
time.sleep(1)
esegui_periodicamente(60)
while True:

    if ser.in_waiting > 0:  # controlla se ci sono dati disponibili
        serial_msg = ser.readline().decode("utf-8").rstrip()
        cmdDetector(serial_msg)

