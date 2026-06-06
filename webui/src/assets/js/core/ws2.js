// core/ws.js

const handlers = new Map();

const ws = new WebSocket("wss://example.com/ws");

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);

  handlers.get(msg.type)?.forEach((fn) => fn(msg));
};

export function subscribe(type, handler) {
  if (!handlers.has(type)) {
    handlers.set(type, []);
  }

  handlers.get(type).push(handler);
}

// stores/chat.js

import { subscribe } from "../core/ws.js";

export default {
  messages: [],

  init() {
    subscribe("chat_message", (msg) => {
      this.messages.push(msg);
    });
  },
};


// stores/users.js

import { subscribe } from '../core/ws.js';

export default {
    users: [],

    init() {
        subscribe('user_update', (msg) => {
            // update users
        });
    }
};