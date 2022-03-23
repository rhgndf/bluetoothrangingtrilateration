import paho.mqtt.client as mqtt
from pymongo import MongoClient
import datetime
import json

mqtt_server = "165.22.249.102"
mqtt_port = 1883
mongo_url = "mongodb://bbalpha:sigmagrindset@localhost:27017/"
mongo_db = 'test'

# connect to MongoDB, change the << MONGODB URL >> to reflect your own connection string
client = MongoClient(mongo_url)
db=client[mongo_db]


#serverStatusResult=db.command("serverStatus")
#print(serverStatusResult)

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("room/bt")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload)
        current_time = datetime.datetime.utcnow()
        print('Received:',data['room'],data['index'], len(data['devices']))
        print(data)
        for device in data['devices']:
            # Fetch the
            
            stale_time = current_time - datetime.timedelta(seconds=60)
            db.detections.delete_many({'lastseen': {'$lt': stale_time}})
            entry = db.detections.find_one_and_update(
                {
                    'mac': device['mac'],
                    'room': data['room']
                },
                {
                    '$set': {
                        'type': data['type'],
                        'name': device['name'],
                        'lastseen': current_time
                    }
                },
                upsert=True, return_document=True
            )
            new_device_list = []
            if 'detections' in entry:
                for detection in entry['detections']:
                    if (current_time - detection['lastseen']).seconds > 60:
                        continue
                    if detection['index'] == data['index']:
                        continue
                    new_device_list.append(detection)
            new_device_list.append({
                'lastseen': current_time,
                'index': data['index'],
                'rssi': device['rssi']
            })
            new_device_list = sorted(new_device_list, key=lambda x:x['index'])
            db.detections.update_one(
                {
                    'mac': device['mac'],
                    'room': data['room']
                },
                {
                    '$set': {
                        'detections': new_device_list,
                        'num_detections': len(new_device_list)
                    }
                }
            )

            notify_json = json.dumps({
                    'mac': device['mac'],
                    'room': data['room']
            })
            client.publish("room/update", notify_json)
    except e:
        print(e)
        pass

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(mqtt_server, mqtt_port, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()