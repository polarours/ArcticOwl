import socket, struct

s = socket.create_connection(("127.0.0.1", 8080))

size_bytes = s.recv(4)
size = struct.unpack('<I', size_bytes)[0]

buf = bytearray()
while len(buf) < size:
    chunk = s.recv(size - len(buf))
    if not chunk:
        break
    buf.extend(chunk)
open('frame.jpg', 'wb').write(buf)