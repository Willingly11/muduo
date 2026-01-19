import socket
import time

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('127.0.0.1', 8000))
    print("Connected")
    s.sendAll(b'Hello')
    data = s.recv(1024)
    print(f"Received: {data}")
    s.close()
except Exception as e:
    print(f"Error: {e}")
