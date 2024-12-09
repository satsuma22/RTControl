<script>
	import WebSocketConnect from "./components/WebSocketConnect.svelte";
	import ApplicationList from "./components/ApplicationList.svelte"
    import VideoPlayer from "./components/VideoPlayer.svelte";
	
	let serverReady = false;
	let videoReady = false;
	let isServerRunning = false;

	let list = null;
	//let videoStream = null;
	let vidComp;

	let onServerReady = (status) => {
		serverReady = true;
		console.log(status);
		console.log(status.list);
		isServerRunning = status.isRunning;
		list = status.list;
	}

	let onVideoReady = (stream) => {
		videoReady = true;
		console.log('adding video...')
		console.log(stream);
		//videoStream = stream;
		vidComp.play(stream);
		vidComp.show();
	}

	let onDataChannelReady = (datachannel) =>
	{
		vidComp.setCallbacks(datachannel);
	}

</script>

<main>
	
	<VideoPlayer bind:this={vidComp} ></VideoPlayer>
	{#if !videoReady}
		{#if !serverReady}
			<h1>Welcome </h1>
			<WebSocketConnect on:server-ready={(e) => onServerReady(e.detail)} on:video-ready={(e) => onVideoReady(e.detail)} on:datachannel-ready={(e) => onDataChannelReady(e.detail)}></WebSocketConnect>
		{:else}
			<ApplicationList appList={list} alreadyRunning={isServerRunning}></ApplicationList>
		{/if}
	{/if}
</main>

<style>
	main {
		text-align: center;
		padding: 1em;
		max-width: 240px;
		margin: 0 auto;
	}

	h1 {
		color: #ff3e00;
		text-transform: uppercase;
		font-size: 4em;
		font-weight: 100;
	}

	@media (min-width: 640px) {
		main {
			max-width: none;
		}
	}
</style>