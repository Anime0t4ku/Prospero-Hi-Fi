import http.server,sys,time
payload=open(sys.argv[1],'rb').read()
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  if self.path=='/cover':
   from pathlib import Path
   image=(Path(sys.argv[1]).parent/'cover.png').read_bytes();self.send_response(200);self.send_header('Content-Type','image/png');self.end_headers();self.wfile.write(image);return
  if self.path=='/slow':time.sleep(1)
  self.send_response(200);self.send_header('Content-Type','audio/mpeg');self.send_header('icy-metaint','1024');self.end_headers()
  try:
   body=payload*(1 if self.path=='/short' else 100)
   for start in range(0,len(body)-1024,1024):
    self.wfile.write(body[start:start+1024]);metadata=b"StreamTitle='Artist - Song';";size=(len(metadata)+15)//16;self.wfile.write(bytes([size])+metadata.ljust(size*16,b'\0'));self.wfile.flush();time.sleep(.003)
  except (BrokenPipeError,ConnectionResetError):pass
 def log_message(self,*args):pass
http.server.ThreadingHTTPServer(('127.0.0.1',18766),Handler).serve_forever()
