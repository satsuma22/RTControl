import { writable } from "svelte/store";

export let socket = writable(null);
export let datachannel = writable(null);
export let trackReceived = writable(0);