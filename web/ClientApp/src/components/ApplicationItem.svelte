<script>
    import {socket} from '../stores/stores'

    export let appTitle;
    export let isRunning;

    let websocket;

    websocket = $socket;

    function sendRequestStartStream()
    {
        let req = {
            id: "server",
            type: "requestStartStream",
            app: appTitle
        };
        console.log(req);
        websocket.send(JSON.stringify(req));
    }

    function sendRequestJoinStream()
    {
        let req = {
            id: "server",
            type: "requestJoinStream"
        };
        console.log(req);
        websocket.send(JSON.stringify(req));
    }

    let onButtonClick = () => {
        if (isRunning)
        {
            sendRequestJoinStream();
        }
        else{
            sendRequestStartStream();
        }
    }
</script>

<div class="list">
    <button on:click={onButtonClick}>{isRunning === 1 ? "Join" : "Launch"}<br>{appTitle}</button>
</div>

<style>
    button {
        
        padding: 40px 20px;
        background-color: #ee833c;
        color: #ffffff;
        border: none;
        border-radius: 5px;
        transition: background-color 0.3s ease;
    }

    button:hover {
        background-color: #e0792a;
    }

</style>