function handleOnReleaseMe() {
    fetch('/release')
        .then(() => console.log('paused'))
        .catch(err => console.error(err));
}

function handleOnReleaseAnother() {
    fetch('/releaseAnother')
        .then(() => console.log('paused other'))
        .catch(err => console.error(err));
}

document.addEventListener("DOMContentLoaded", () => {
   document.title = location.host === "192.168.4.1" ? "LEFT (HOST)" : "RIGHT";
});

let ws = new WebSocket(`ws://${location.host}/ws`);

ws.onmessage = (event) => {
    const color = event.data.trim().toLowerCase();
    document.querySelectorAll('.led').forEach(led => led.classList.remove('its-lit'));
    const activeLED = document.getElementById(color);
    if (activeLED) {
        activeLED.classList.add('its-lit');
    }
};

