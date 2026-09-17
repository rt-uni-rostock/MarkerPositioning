#!/usr/bin/env python3
"""
udp_message_receiver.py - Gegenstelle zu UdpPublisher.cpp

Empfaengt die von UdpPublisher (MarkerPositioning) per UDP gesendeten
MarkerMessage-Pakete, protokolliert sie lesbar (Konsole + Logdatei) und
speichert zusaetzlich alle Rohdaten als JSON-Lines-Datei, damit jede
empfangene Nachricht spaeter exakt nachvollzogen werden kann.

Paket-Layout (siehe UdpPublisher.cpp / UdpPublisher.h):
    UDP_PACKET_SIZE = 22 * sizeof(double) = 176 Bytes

    Offset  Typ     Feld
    0       double  rotX
    8       double  rotZ
    16      double  rotY
    24      double  reserved (immer -1)
    32      double  posX
    40      double  posY
    48      double  posZ
    56      uint8   markerId
    57      double  cameraId
    65..152 double  padding (11 x 0.0)
    153..175        ungenutzt/0 (23 Bytes, da Buffer nullinitialisiert ist)

Verwendung:
    python udp_message_receiver.py [--host 0.0.0.0] [--port 5001] [--logfile udp_receiver.log] [--jsonfile udp_messages.jsonl]
"""

import argparse
import json
import socket
import struct
import sys
import logging
from datetime import datetime, timezone

# Muss exakt UdpPublisher::UDP_PACKET_SIZE (22 * sizeof(double)) entsprechen.
UDP_PACKET_SIZE = 22 * 8  # 176 Bytes

# Little-Endian: 7 doubles (rotX, rotZ, rotY, reserved, posX, posY, posZ)
# + 1 uint8 (markerId) + 1 double (cameraId)
HEADER_FORMAT = "<7dBd"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 65 Bytes


def parse_marker_message(data: bytes) -> dict:
    """Entpackt die ersten Bytes eines UDP-Pakets gemaess UdpPublisher::serialize()."""
    if len(data) < HEADER_SIZE:
        raise ValueError(f"Paket zu kurz: {len(data)} Bytes (erwartet mindestens {HEADER_SIZE})")

    rotX, rotZ, rotY, reserved, posX, posY, posZ, marker_id, camera_id = struct.unpack(
        HEADER_FORMAT, data[:HEADER_SIZE]
    )

    return {
        "markerId": marker_id,
        "cameraId": camera_id,
        "position": {"x": posX, "y": posY, "z": posZ},
        "rotation": {"x": rotX, "y": rotY, "z": rotZ},
        "reserved": reserved,
        "packetSize": len(data),
        "expectedPacketSize": UDP_PACKET_SIZE,
        "sizeMismatch": len(data) != UDP_PACKET_SIZE,
    }


def setup_logging(logfile: str) -> logging.Logger:
    logger = logging.getLogger("UdpReceiver")
    logger.setLevel(logging.DEBUG)

    formatter = logging.Formatter("%(asctime)s.%(msecs)03d | %(message)s", datefmt="%Y-%m-%d %H:%M:%S")

    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)

    file_handler = logging.FileHandler(logfile, mode="a", encoding="utf-8")
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)

    return logger


def main():
    parser = argparse.ArgumentParser(description="UDP-Empfaenger als Gegenstelle zu UdpPublisher.cpp")
    parser.add_argument("--host", default="0.0.0.0", help="Bind-Adresse (Standard: 0.0.0.0, alle Interfaces)")
    parser.add_argument("--port", type=int, default=5001, help="UDP-Port (Standard: 5001, siehe Settings.json)")
    parser.add_argument("--logfile", default="udp_receiver.log", help="Pfad zur lesbaren Log-Datei")
    parser.add_argument("--jsonfile", default="udp_messages.jsonl", help="Pfad zur JSON-Lines-Rohdatendatei")
    args = parser.parse_args()

    logger = setup_logging(args.logfile)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))
    # Timeout, damit recvfrom() regelmaessig zurueckkehrt und Strg+C (KeyboardInterrupt)
    # zeitnah verarbeitet werden kann (unter Windows blockiert recvfrom() sonst Signale).
    sock.settimeout(0.5)

    logger.info(f"UDP-Empfaenger gestartet auf {args.host}:{args.port}")
    logger.info(f"Log-Datei: {args.logfile}")
    logger.info(f"JSON-Rohdaten-Datei: {args.jsonfile}")
    logger.info("Warte auf Nachrichten... (Strg+C zum Beenden)")

    message_count = 0

    try:
        with open(args.jsonfile, "a", encoding="utf-8") as json_out:
            while True:
                try:
                    data, addr = sock.recvfrom(65535)
                except socket.timeout:
                    continue  # kein Paket in diesem Intervall, aber Chance fuer Strg+C

                receive_time = datetime.now(timezone.utc).astimezone()  # lokale Zeit mit Offset
                message_count += 1

                record = {
                    "sequence": message_count,
                    "timestamp": receive_time.isoformat(),
                    "sender": {"ip": addr[0], "port": addr[1]},
                    "rawHex": data.hex(),
                }

                try:
                    parsed = parse_marker_message(data)
                    record["parsed"] = parsed

                    logger.info(
                        "#%d von %s:%d | markerId=%s | cameraId=%s | pos=(%.4f, %.4f, %.4f) | rot=(%.4f, %.4f, %.4f) | %d Bytes%s",
                        message_count,
                        addr[0], addr[1],
                        parsed["markerId"],
                        parsed["cameraId"],
                        parsed["position"]["x"], parsed["position"]["y"], parsed["position"]["z"],
                        parsed["rotation"]["x"], parsed["rotation"]["y"], parsed["rotation"]["z"],
                        len(data),
                        " [GROESSE WEICHT AB!]" if parsed["sizeMismatch"] else "",
                    )
                except ValueError as exc:
                    record["parseError"] = str(exc)
                    logger.warning("#%d von %s:%d konnte nicht geparst werden: %s (%d Bytes, hex=%s)",
                                    message_count, addr[0], addr[1], exc, len(data), data.hex())

                json_out.write(json.dumps(record, ensure_ascii=False) + "\n")
                json_out.flush()

    except KeyboardInterrupt:
        logger.info(f"Beendet. Insgesamt {message_count} Nachrichten empfangen.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
