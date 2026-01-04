import { globals } from "./env";

let ws: WebSocket | null = null;

export function getWebSocket(): WebSocket {
  if (!ws || ws.readyState === WebSocket.CLOSED) {
    ws = new WebSocket(`${globals.websockets.scheme}://${globals.url_string.subdomain}${globals.url_string.port !== undefined ? ':' + globals.url_string.port : ''}/ws/`);

    ws.onopen = () => console.log("[WebSocket] Connected");
    ws.onclose = () => console.log("[WebSocket] Disconnected");
    ws.onerror = (err) => console.error("[WebSocket] Error:", err);
  }

  return ws;
}