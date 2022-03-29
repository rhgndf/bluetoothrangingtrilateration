import { Server } from "socket.io";
import isString from "is-string";
import { pushRoomStateToSocket } from "./models/roomdb.js";
export let io;
export function initSocketIO(server) {
  io = new Server(server, {
    cors: {
      origin: "http://localhost:3001",
      credentials: true
    },
  });

  io.on("connection", function (socket) {
    console.log("A user connected");
    // Only allow a session to join one room at one time, leave the previous room if joined
    socket.on("joinRoom", (room) => {
      if (!isString(room)) {
        return;
      }
      socket.data.currentRoom = socket.data.currentRoom || "";
      if (socket.data.currentRoom == "") {
        socket.join("room-" + room);
        pushRoomStateToSocket(socket, room);
      } else if (socket.data.currentRoom != room) {
        socket.leave("room-" + socket.data.currentRoom);
        socket.join("room-" + room);
        pushRoomStateToSocket(socket, room);
      }
      socket.data.currentRoom = room;
    });
  });
}

// Emit new data to a room
export function emitRoom(room, msg) {
  io.to("room-" + room).emit("roomState", msg);
}
