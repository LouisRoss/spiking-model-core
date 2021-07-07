#!/usr/bin/python3

import sys
import json
import socket

size_packet = bytearray()
data_packet = bytearray()
struct_count = 0
struct_size = 8

def pack2Bytes(data, packed_data):
    packed_data.append(data & 255)
    packed_data.append((data >> 8) & 255)

def pack4Bytes(data, packed_data):
    packed_data.append(data & 255)
    packed_data.append((data >> 8) & 255)
    packed_data.append((data >> 16) & 255)
    packed_data.append((data >> 24) & 255)

def packStructCount():
    global struct_count
    global size_packet

    pack2Bytes(struct_count, size_packet)

def packStruct(tick, index):
    global data_packet

    pack4Bytes(tick, data_packet)
    pack4Bytes(index, data_packet)

def addStruct(tick, index):
    global struct_count

    packStruct(tick, index)
    struct_count += 1


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("localhost", 8001))

tick = 0
print(len(sys.argv))
if len(sys.argv) > 1:
    tick = int(sys.argv[1])

#signal = [[tick, 0], [tick, 1], [tick, 2], [tick, 3], [tick, 4]]
#msg = json.dumps(signal).encode('utf-8')

addStruct(tick, 0)
addStruct(tick, 1)
addStruct(tick, 2)
addStruct(tick, 3)
addStruct(tick, 4)
packStructCount()

s.sendall(size_packet)
s.sendall(data_packet)
s.close()
