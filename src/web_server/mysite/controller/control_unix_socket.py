import sys
import traceback
import json
import socket
import struct


def ctrl_create_unix_socket():
  sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
  
  server_address = "/tmp/m_unix_path"
  print "connecting to %s" % server_address
  
  try:
    sock.connect(server_address);
  except socket.error, msg:
    print msg
    return None

  return sock

def __ctrl_get_server_data(vreq):
  sock = ctrl_create_unix_socket()
  if sock == None:
    return "{\"Resul\"t:-1, \"ErrMsg\":\"Cannot connect to the server\"}"
 
  print vreq
  cmd = json.dumps(vreq)
  iLen = len(cmd)
  tmp_str = struct.pack("i", socket.htonl(iLen))
  send_data = tmp_str + cmd
  sock.sendall(send_data)

  data = sock.recv(4)
  iLen = struct.unpack("I", data)
  iLen = socket.ntohl(iLen[0])
  print "recv len = %d" % iLen
  data = sock.recv(iLen)
  print "recv data = %s" % data
  sock.close()
  return data

def ctrl_get_server_data(method):
  vreq = {}  

  vreq["RPCMethod"] = method

  return __ctrl_get_server_data(vreq)
   
def ctrl_get_server_detail_data(method, ip, port ,sid):
  print "param = ", ip
  print "port = ", port
  print "id = ", sid
  vreq = {}

  vreq["RPCMethod"] = method
  vreq["ip"] = ip
  vreq["port"] = port
  vreq["id"] = sid

  return __ctrl_get_server_data(vreq)
