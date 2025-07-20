import http from "http";

const server = http.createServer((req, res) => {
  if (req.url === "/pods") {
    console.log("Received /pods request");
    const body = '[{"name":"test-pod","status":"Running"}]';
    res.writeHead(200, {
      "Content-Type": "application/json",
      "Content-Length": Buffer.byteLength(body),
      "Connection": "close"
    });
    res.end(body);
  }
});

server.listen(3000, '0.0.0.0', () => console.log("Listening on 0.0.0.0:3000"));

