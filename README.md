# The In-C-redible Web Server
A web server made in C which handles GET request, whether they're good or bad ones. <br>
<b>Develoement time:</b> 11:33 hours.

## Setting Up:
1. Install dependencies:<b> libbsd-dev</b>
2. Run <b>make</b>

If using Linux system, run
```
./build
```

## Running:
Run ./webserver on a terminal

### Options:
- -h  set server host address 
- -p  set server port
- -r  set server root directory
- -w  set maximum available clients


## Connecting:
got to http://127.0.0.1:5000/index.html 
on a wev browser running HTTP/1.1 or HTTP/1.0

Or run
```
nc -C localhost 5000
```
on a different terminal and manually type in the request.

### Example request:
```
GET /index.html HTTP/1.1
```

## Shutting Down:
Ctrl-c on terminl runnning the server
