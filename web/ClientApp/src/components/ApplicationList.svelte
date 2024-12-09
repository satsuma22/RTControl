<script>
    import ApplicationItem from "./ApplicationItem.svelte";
    export let appList = null;
    export let alreadyRunning = false;
</script>

<div class="button-container">
    {#each appList as app}
        <ApplicationItem appTitle={app} isRunning={alreadyRunning}></ApplicationItem>
    {/each}
</div>

<style>
    .button-container {
        display: flex;
        gap: 10px;
        justify-content: center;
        margin-top: 100px;
    }
</style>