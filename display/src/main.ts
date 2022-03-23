import { createApp } from "vue";
import { createPinia } from "pinia";

import App from "./App.vue";
import router from "./router";

import VueSocketIO from "vue-3-socket.io";
import SocketIO from "socket.io-client";

const app = createApp(App);
const store = createPinia();
app.use(
  new VueSocketIO({
    debug: true,
    connection: SocketIO("http://localhost:3000"),
    vuex: {
      store,
      actionPrefix: "SOCKET_",
      mutationPrefix: "SOCKET_",
    },
    //options: { path: "/" }, //Optional options
  })
);

app.use(store);
app.use(router);

app.mount("#app");
