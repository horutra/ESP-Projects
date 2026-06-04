from http.server import BaseHTTPRequestHandler, HTTPServer

class MyServer(BaseHTTPRequestHandler):

    def do_GET(self):

        print("GET request received")
        print("Path:", self.path)

        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()

        self.wfile.write(b"GET request received")

    def do_POST(self):

        print("POST request received")
        print("Path:", self.path)

        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)

        print("Data:", post_data.decode())

        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()

        self.wfile.write(b"POST request received")

server = HTTPServer(("0.0.0.0", 8000), MyServer)

print("Server running on port 8000...")

server.serve_forever()
