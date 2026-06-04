from http.server import BaseHTTPRequestHandler, HTTPServer
import requests

SERVER_LOCATION = "Santa Cruz"
class MyServer(BaseHTTPRequestHandler):

    def do_GET(self):

        if self.path == "/location": 
            print("Location request received")
            print("Location:", SERVER_LOCATION)

            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.end_headers()

            self.wfile.write(SERVER_LOCATION.encode())
            return



        print("GET request received")
        print("Path:", self.path)
        print("Client Address:", self.client_address[0])

        try: 
            weather = requests.get("http://wttr.in/?format=3").text
            if weather:
             print("Weather:", weather)
            else: 
                print("Failed to retrieve weather information")
        except Exception as e:
            print("Error fetching weather information:", e)
            weather = "Unable to fetch weather information"

        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()


        self.wfile.write(weather.encode())

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
