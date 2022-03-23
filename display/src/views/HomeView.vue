<script setup lang="ts">
import TheWelcome from "@/components/TheWelcome.vue";
</script>

<script lang="ts">
export default {
  data() {
    return {
      detections: {},
    };
  },
  sockets: {
    connect: function () {
      console.log("socket connected");
    },
    roomState: function (msg) {
      this.addDetection(msg);
    },
  },
  methods: {
    addDetection(data) {
      this.detections[data["mac"]] = data;
    },
    removeOldDetections() {
      let currentDate = new Date();
      let toRemove = [];
      for (let key in this.detections) {
        if (currentDate - new Date(this.detections[key].lastseen) > 60000) {
          toRemove.push(key);
        }
      }
      for (let key of toRemove) {
        delete this.detections[key];
      }
    },
  },
  created() {
    this.$socket.emit("joinRoom", "default");
    setInterval(this.removeOldDetections, 1000);
  },
};
</script>

<template>
  <main>
    <!--<TheWelcome />-->
    <div id="room">
      <div
        class="detection"
        v-for="(item, key) in detections"
        :key="key"
        :style="'left: ' + item.x * 100 + 'px; bottom: ' + item.y * 100 + 'px;'"
      ></div>
    </div>
  </main>
</template>

<style>
#room {
  width: 500px;
  height: 500px;
  background-color: #e0e0e0;
  position: relative;
}
.detection {
  width: 10px;
  height: 10px;
  background-color: blue;
  border-radius: 5px;
  position: absolute;
}
</style>
