<script>
    import {createEventDispatcher} from "svelte";
    import {socket, trackReceived} from '../stores/stores'
    
    const dispatch = createEventDispatcher();

    let websocket = null;
    let id = null;
    let showWarning = false;
    let pc = null;
    let dc = null;

    function randomId(length) {
        const characters = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
        const pickRandom = () => characters.charAt(Math.floor(Math.random() * characters.length));
        return [...Array(length) ].map(pickRandom).join('');
    }

    id = randomId(10);

    let requestStatus = () => {
        let msg = {
            "id" : "server",
            "type" : "requestStatus"
        };

        websocket.send(JSON.stringify(msg));
    }

    async function handleOffer(offer) {
        pc = createPeerConnection();
        await pc.setRemoteDescription(offer);
        await sendAnswer(pc);
    }

    async function sendAnswer(pc) {
        await pc.setLocalDescription(await pc.createAnswer());
        await waitGatheringComplete();

        const answer = pc.localDescription;
        console.log("Sending Answer...")

        websocket.send(JSON.stringify({
            id: "server",
            type: answer.type,
            sdp: answer.sdp,
        }));
    }

    async function waitGatheringComplete() {
        return new Promise((resolve) => {
            if (pc.iceGatheringState === 'complete') {
                resolve();
            } else {
                pc.addEventListener('icegatheringstatechange', () => {
                    if (pc.iceGatheringState === 'complete') {
                        resolve();
                    }
                });
            }
        });
    }

    function createPeerConnection() {
        const config = {
            bundlePolicy: "max-bundle",
        };

        config.iceServers = [{urls: ['stun:stun.l.google.com:19302']}];
        let pc = new RTCPeerConnection(config);

        // Register some listeners to help debugging
        pc.addEventListener('iceconnectionstatechange', () =>
            console.log(pc.iceConnectionState));

        pc.addEventListener('icegatheringstatechange', () =>
            console.log(pc.iceGatheringState));

        pc.addEventListener('signalingstatechange', () =>
            console.log(pc.signalingState));

        // Receive audio/video track
        pc.ontrack = (evt) => {
            /*
            document.getElementById('media').style.display = 'block';
            const video = document.getElementById('video');
            // always overrite the last stream - you may want to do something more clever in practice
            video.srcObject = evt.streams[0]; // The stream groups audio and video tracks
            video.play();
            */
            dispatch('video-ready', evt.streams[0]);
            trackReceived.update((value) => value = value + 1)
        };
        // Receive data channel
        pc.ondatachannel = (evt) => {
            dc = evt.channel;

            dc.onopen = () => {
                console.log("Data Channel Opened")
            };

            dc.onmessage = (evt) => {
                if (typeof evt.data !== 'string') {
                    return;
                }

                //console.log(evt.data);
                dc.send(evt.data);
            }

            dc.onclose = () => {
                console.log("Data Channel Closed")
            };

            //datachannel.set(dc);
            dispatch('datachannel-ready', dc);
        }

        return pc;
    }

    let ConnectToWebSocket = () => {
        let ip = document.getElementById("ipaddress").value;
        
        if (ip)
        {
            console.log(`Connecting to ${ip}...`);
            websocket = new WebSocket('ws://' + ip + '/' + id);
        }
        else 
        {
            console.log('Connecting to localhost...');
            websocket = new WebSocket('ws://127.0.0.1:8000/' + id);
        }

        websocket.onopen = () => {
            console.log('Connected to the Signaling Server!');
            // Request the current status of the server
            requestStatus();
        }

        websocket.onclose = () => {
            console.log('WebSocket Closed!');
        }

        websocket.onerror = () => {
            const inputElem = document.getElementById("ipaddress");
            inputElem.style.borderColor = "#ff8888";
            showWarning = true;
        }

        websocket.onmessage = async (evt) => {
            if (typeof evt.data !== 'string'){
                return;
            }

            const message = JSON.parse(evt.data);
            if (message.type == "Status"){
                console.log(message);
                let status;
                if (message.isRunning){
                    status = {
                        "isRunning" : message.isRunning,
                        "list" : [message.list]
                    }
                }
                else{
                    status = {
                        "isRunning" : message.isRunning,
                        "list" : message.list
                    }
                }
                dispatch('server-ready', status);
            }
            else if (message.type == "offer") {
                //document.getElementById('offer-sdp').textContent = message.sdp;
                await handleOffer(message);
            }
        }
        socket.set(websocket);
    }

</script>

<input type="text" id="ipaddress">
<button on:click={ConnectToWebSocket}>Connect</button>

{#if showWarning}
    <p id="warning">Couldn't connect to the websocket. Try again...</p>
{/if}


<style>
    #warning {
        color: #ff8888;
        font-size: 0.8em;
    }
</style>