import { getWebSocket } from "./websocket";

export class eventManager {
    ws: WebSocket = getWebSocket();
    events: [] = [];

    public listenForEvent(targetEvent: string, run: (args: unknown) => void) {
        this.ws.onmessage = (msg) => {
            const {event, data} = JSON.parse(msg.data);

            if (event === targetEvent) {
                run(data)
            }
        }
    }

    public emitEvent(targetEvent: string, data?: unknown) {
        if (!this.ws || this.ws.readyState !== this.ws.OPEN) {
            console.log("Websockets not ready");
            return;
        }

        this.ws.send(JSON.stringify({
            event: targetEvent,
            data: data,
        }));
    }
};