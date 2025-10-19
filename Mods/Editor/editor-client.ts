/// <reference path="./editor.d.ts" />

import {StandardWebSocketClient} from "https://deno.land/x/websocket@v0.1.4/mod.ts";

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
            console.log("The editor has welcomed us. Trying to spawn an entity.");

            const qnData = {
                "tempHash": "00644fe9eb9feff5",
                "tbluHash": "005474211f99b411",
                "rootEntity": "fffffffffffffffe",
                "entities": {
                    "fffffffffffffffe": {
                        "parent": null,
                        "name": "Entity",
                        "factory": "[assembly:/_pro/design/setpieces/unique/setpiece_miami_unique.template?/setpiece_miami_unique_android.entitytemplate].pc_entitytype",
                        "blueprint": "[assembly:/_pro/design/setpieces/unique/setpiece_miami_unique.template?/setpiece_miami_unique_android.entitytemplate].pc_entityblueprint",
                        "properties": {
                            "m_mTransform": {
                                "type": "SMatrix43",
                                "value": {
                                    "rotation": {
                                        "x": -0,
                                        "y": 0,
                                        "z": -101.52882
                                    },
                                    "position": {
                                        "x": -40.105434,
                                        "y": -29.001667,
                                        "z": 2.3575625
                                    }
                                }
                            },
                            "m_eRoomBehaviour": {
                                "type": "ZSpatialEntity.ERoomBehaviour",
                                "value": "ROOM_DYNAMIC"
                            }
                        }
                    }
                },
                "propertyOverrides": [],
                "overrideDeletes": [],
                "pinConnectionOverrides": [],
                "pinConnectionOverrideDeletes": [],
                "externalScenes": [],
                "subType": "brick",
                "quickEntityVersion": 3.1,
                "extraFactoryDependencies": [],
                "extraBlueprintDependencies": [],
                "comments": []
            };

            sendMessage({
                type: "spawnQnEntity",
                entityId: "0069420000000000",
                name: "Custom spawned entity",
                qnJson: JSON.stringify(qnData),
            });

            /*sendMessage({
                type: "selectEntity",
                entity: {
                    id: "6a561b6be27290db",
                    tblu: "0093BA77DCA54AD5",
                },
            });

            console.log("Requesting a list of all entities.");

            sendMessage({
                type: "listEntities",
            });*/

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
                    source: "game",
                },
                property: "m_bVisible",
                value: false,
            });

            return;
        }

        if (msg.type === "entitySpawned") {
            console.log("Entity spawned:", msg.entity);
            return;
        }

        if (msg.type === "entityDetails") {
            console.log("Received details for an entity:", msg.entity);

            /*if (msg.entity.properties.m_eidParent.data !== null) {
                console.log("Setting parent property to null and getting details again.");

                sendMessage({
                    type: "setEntityProperty",
                    entity: {
                        id: msg.entity.id,
                        tblu: msg.entity.tblu,
                    },
                    property: "m_eidParent",
                    value: null,
                });

                sendMessage({
                    type: "getEntityDetails",
                    entity: {
                        id: msg.entity.id,
                        tblu: msg.entity.tblu,
                    },
                });
            }*/

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
