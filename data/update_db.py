import paho.mqtt.client as mqtt
from pymongo import MongoClient
from sklearn.neighbors import KNeighborsRegressor
import datetime
import json
import numpy as np

mqtt_server = "165.22.249.102"
mqtt_port = 1883
mongo_url = "mongodb://bbalpha:sigmagrindset@localhost:27017/"
mongo_db = 'test'

client = MongoClient(mongo_url)
db = client[mongo_db]

# Setup the machine learning model
data = np.array([[-50, -58, -62, 0, 5],
 [-71, -65, -74, 0, 5],
 [-65, -64, -68, 5, 5],
 [-67, -87, -79, 5, 5],
 [-69, -87, -81, 5, 0],
 [-53, -72, -80, 5, 0],
 [-56, -60, -73, 2.5, 2.5],
 [-63, -68, -92, 2.5, 2.5],
 [-64, -96, -84, 0, 0],
 [-62, -70, -66, 0, 0]])

 X = data[:, :3]
 y1 = data[:, -2]
 y2 = data[:, -1]

model1 = KNeighborsRegressor()
model2 = KNeighborsRegressor()

model1.fit(X, y1)
model2.fit(X, y2)

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribe to the message
    client.subscribe("room/bt")

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload)
        current_time = datetime.datetime.utcnow()
        print('Received:',data['room'],data['index'], len(data['devices']))
        print(data)
        # Delete stale data more than 60 seconds old
        stale_time = current_time - datetime.timedelta(seconds=60)
        db.detections.delete_many({'lastseen': {'$lt': stale_time}})
        for device in data['devices']:
            # Fetch the data for the mac address in the room, create if not present
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
            # Update the detections
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
            
            # If we have enough RSSI data to create an x and y prediction, insert it in
            rssis = [device['rssi'] for device in new_device_list]
            if len(rssis) == 3:
                x = model1.predict([rssis])[0]
                y = model1.predict([rssis])[0]
                db.detections.update_one(
                    {
                        'mac': device['mac'],
                        'room': data['room']
                    },
                    {
                        '$set': {
                            'x': x,
                            'y': y
                        }
                    }
                )
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