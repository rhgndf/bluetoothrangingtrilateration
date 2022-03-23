import mqtt from "mqtt";
import { getRoomAndMacState } from "./models/roomdb.js";
import { emitRoom } from "./socket.js";

const client = mqtt.connect("mqtt://bbalpha.ragulbalaji.com");

client.on("connect", function () {
  client.subscribe("room/update", function (err) {
    if (err) {
      console.log("Error subscribing to update");
    }
  });
});

client.on("message", async function (topic, message) {
  // message is Buffer
  if (topic === "room/update") {
    const data = JSON.parse(message.toString());
    const doc = await getRoomAndMacState(data["room"], data["mac"]);
    console.log(data, doc);
    emitRoom(data["room"], doc);
  }
});

export default client;
