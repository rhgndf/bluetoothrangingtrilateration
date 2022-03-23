import { MongoClient } from "mongodb";
const uri = "mongodb://bbalpha:sigmagrindset@localhost:27017/";
const client = new MongoClient(uri);

let roomCollection = client.db("test").collection("detections");

export async function connect() {
  // Connect the client to the server
  await client.connect();
  // Establish and verify connection
  await client.db("admin").command({ ping: 1 });
  console.log("Connected successfully to MongoDB server");
}

export async function getRoomState(room) {
  roomCollection.find({ room: room });
}

export async function pushRoomStateToSocket(socket, room) {
  await roomCollection.find({ room: room }).forEach((doc) => {
    socket.emit("roomState", doc);
  });
}

export async function getRoomAndMacState(room, mac) {
  return await roomCollection.findOne({ room: room, mac: mac });
}
