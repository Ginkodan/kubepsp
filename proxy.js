import http from "http";

const server = http.createServer((req, res) => {
  if (req.url === "/pods") {
    const body = '[{"name":"test-pod","status":"Running"}]';
    res.writeHead(200, {
      "Content-Type": "application/json",
      "Content-Length": Buffer.byteLength(body),
      "Connection": "close"
    });
    res.end(body);
  }
});

server.listen(3000, () => console.log("K8s proxy listening on http://0.0.0.0:3000"));
