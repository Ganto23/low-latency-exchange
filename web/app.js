// Connect to the Python Bridge on the same machine
// If you open this from a different computer, change 'localhost' to the Pi's IP address
const ws = new WebSocket('ws://192.168.0.13:8080');

const statusEl = document.getElementById('status');
const bestBidEl = document.getElementById('best-bid');
const bestAskEl = document.getElementById('best-ask');
const tradeTape = document.getElementById('trade-tape');

const NULL_NODE = 4294967295; // Matches C++ UINT32_MAX

ws.onopen = () => {
    statusEl.textContent = 'Connected';
    statusEl.className = 'status connected';
};

ws.onclose = () => {
    statusEl.textContent = 'Disconnected';
    statusEl.className = 'status disconnected';
};

ws.onmessage = (event) => {
    const msg = JSON.parse(event.data);

    if (msg.type === 'BBO_UPDATE') {
        updateBook(msg);
    } else if (msg.type === 'TRADE') {
        appendTrade(msg);
    }
};

function updateBook(msg) {
    // If the price is NULL_NODE, it means that side of the book is completely empty
    const isEmpty = (msg.price === NULL_NODE || msg.quantity === 0);
    const displayStr = isEmpty ? 'EMPTY' : `${msg.quantity} @ $${msg.price}`;

    if (msg.side === 'BUY') {
        bestBidEl.textContent = displayStr;
        if (isEmpty) {
            bestBidEl.classList.add('empty');
            bestBidEl.style.color = '';
        } else {
            bestBidEl.classList.remove('empty');
            bestBidEl.style.color = '#3fb950'; // Green
        }
    } else {
        bestAskEl.textContent = displayStr;
        if (isEmpty) {
            bestAskEl.classList.add('empty');
            bestAskEl.style.color = '';
        } else {
            bestAskEl.classList.remove('empty');
            bestAskEl.style.color = '#ff7b72'; // Red
        }
    }
}

function appendTrade(msg) {
    const li = document.createElement('li');
    li.className = msg.side === 'BUY' ? 'buy-trade' : 'sell-trade';
    
    // Convert the nanosecond timestamp to a readable time
    const date = new Date(msg.timestamp / 1000000);
    const timeStr = date.toISOString().substring(11, 23); // HH:MM:SS.mmm

    li.innerHTML = `
        <span>${timeStr}</span>
        <span>$${msg.price}</span>
        <span>${msg.quantity}</span>
    `;

    // Add to the top of the list
    tradeTape.prepend(li);

    // Keep the tape from growing infinitely
    if (tradeTape.children.length > 50) {
        tradeTape.removeChild(tradeTape.lastChild);
    }
}