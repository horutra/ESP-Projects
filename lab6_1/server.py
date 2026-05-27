from http.server import BaseHTTPRequestHandler, HTTPServer
import requests

class MyServer(BaseHTTPRequestHandler):
    def do_GET(self):
        ##weather = requests.get("http://wttr.in/?format=%t").text
        weather = "this is a test"
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        
        self.wfile.write(weather.encode())

     


server = HTTPServer(('0.0.0.0', 8080), MyServer)
print("Server started on port 8080...")
server.serve_forever()