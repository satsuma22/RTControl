const express = require('express')
const http = require('http')
const WebSocket = require('ws')

let clients = {};

const app = express();
const server = http.createServer(app);

//app.use(express.static(__dirname + '/../Client'));
app.use(express.static(__dirname + '/../ClientApp/public'));

const wss = new WebSocket.Server({server});

wss.on('connection', (ws, req) => {
    let clientId = req.url;
    clientId = clientId.replace("/", "");
    console.log(`Client ${clientId} conected!`);

    ws.on('message', (message) => {
        console.log(`message: ${message}`);

        const msg = JSON.parse(message);
        const destId = msg.id;
        const dest = clients[destId];

        if(dest)
        {
            msg.id = clientId;
            const data = JSON.stringify(msg);
            dest.send(data);
        }
        else
        {
            console.error(`Client ${destId} not found`);
        }
    })

    ws.on('close', () => {
        console.log('Client left!');
        delete clients[clientId];
    })

    clients[clientId] = ws;
});

const PORT = process.env.PORT || 8000;

server.listen(PORT, () => {
    console.log(`Signaling Server listening on port ${PORT}`);
});