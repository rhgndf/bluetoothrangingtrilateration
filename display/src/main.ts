import Vue, { createApp } from "vue";
import { createPinia } from "pinia";

import App from "./App.vue";
import router from "./router";

import VueSocketIO from "vue-3-socket.io";

Vue.use(
  new VueSocketIO({
    debug: true,
    connection: "http://localhost:8000",
    vuex: {
      store,
      actionPrefix: "SOCKET_",
      mutationPrefix: "SOCKET_",
    },
    options: { path: "/my-app/" }, //Optional options
  })
);

const app = createApp(App);

app.use(createPinia());
app.use(router);

app.mount("#app");
