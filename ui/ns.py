#!/usr/bin/python3

import sys
import json
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("localhost", 8001))

tick = 0
print(len(sys.argv))
if len(sys.argv) > 1:
    tick = int(sys.argv[1])

signal = [[tick, 0], [tick, 1], [tick, 2], [tick, 3], [tick, 4]]
msg = json.dumps(signal).encode('utf-8')

s.sendall(msg)
s.close()
