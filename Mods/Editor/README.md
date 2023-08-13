# Editor Mod

## Installation Instructions

This mod requires the installation of additional content using [SMF](https://github.com/atampy25/simple-mod-framework).

1. Download [SMF](https://github.com/atampy25/simple-mod-framework) if you don't have it already and set it up.
2. Copy the [Smf](/Mods/Editor/Smf) folder and its contents into your SMF `Mods` folder.
3. Rename the `Smf` folder you copied to `Editor` (or anything else).
4. Run SMF, enable the `Editor` mod, and `Apply` your changes.

## Third-party tools

Third party tools can remotely control and communicate with a running editor
instance by connecting to a WebSocket server that the editor hosts locally on
port `46735`. The editor will only accept connections from `localhost` (i.e.
the same machine that the editor is running on).

Clients can send JSON messages to the editor server, and the server will send
JSON events back to the client. After initial connection, the client must send
a `Hello` message to the server, and the server will respond with a `Welcome`
event. After this handshake, the client can start controlling the editor and 
receiving events from it (e.g. when entities get selected, their properties change,
etc.).

For a detailed description of all request and event types, see the [editor.d.ts](/Mods/Editor/editor.d.ts) 
file in this mod's folder. The `EditorRequest` type describes all the requests 
that a client can send to the editor, and the `EditorEvent` type describes all
the events that the editor can send to a client.

You can also find a sample client implementation in the [editor-client.ts](/Mods/Editor/editor-client.ts)
file in this mod's folder. This file is written in TypeScript for use with Deno,
but it can be used for reference when writing clients in other languages or 
environments.

Multiple clients can connect to the editor at the same time, and the editor will
send events to all connected clients. However, events that are tied to a request
that a specific client sent (e.g. a `ListEntities` request) will only be sent to
that client.