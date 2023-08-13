/// <reference path="./editor.d.ts" />

import { StandardWebSocketClient } from "https://deno.land/x/websocket@v0.1.4/mod.ts";

const ws = new StandardWebSocketClient("ws://localhost:46735");

ws.on("error", (e: Error) => {
	console.error("Encountered error in editor connection:", e);
});

ws.on("close", () => {
	console.log("Connection to editor closed.");
});

function sendMessage(msg: EditorRequest) {
	const jsonMsg = JSON.stringify(msg);
	console.log("Sending message to editor:", jsonMsg);
	ws.send(jsonMsg);
}

ws.on("message", (message) => {
	try {
		const msg: EditorEvent = JSON.parse(message.data);
		console.log("Received message from editor:", msg);

		if (msg.type === "welcome") {
			console.log("The editor has welcomed us. Trying to select an entity.");

			sendMessage({
				type: "selectEntity",
				entity: {
					id: "6a561b6be27290db",
					tblu: "0093BA77DCA54AD5",
				},
			});

			console.log("Requesting a list of all entities.");

			sendMessage({
				type: "listEntities",
			});

			return;
		}

		if (msg.type === "entityList") {
			console.log("Received a list of entities:", msg.entities);

			console.log("Getting details for the 9th entity in the list for no particular reason.");

			sendMessage({
				type: "getEntityDetails",
				entity: msg.entities[9],
			});

			console.log("Making hitman invisible.");

			sendMessage({
				type: "setEntityProperty",
				entity: {
					id: "6a561b6be27290db",
					tblu: "0093BA77DCA54AD5",
				},
				property: "m_bVisible",
				value: false,
			});

			return;
		}
	} catch (e) {
		console.error("Encountered error while parsing message from editor:", e, message.data);
	}
});

ws.on("open", () => {
    console.log("Connected to SDK Editor.");

	sendMessage({
		type: "hello",
		identifier: "Editor Test Client"
	});
});
