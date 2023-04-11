import serial
import sys
import json
import requests






com="com21"

if(len(sys.argv) > 2):
	com = sys.argv[1]

ser = serial.Serial(com)



while (1):
	line = ser.readline()
	print(line)
	data = str(line).split(',')
	print(data)

	pack = {

		"board_id"	:  int(data[1]),
		"packet_nr"	:  int(data[2]),
		"hop_cnt"	:  int(data[3]),
		"bat_level"	:  int(data[4])/10.0,
		"latitude"	:  float(data[5]),
		"longitude"	:  float(data[6]),
		"hdop"		:  float(data[7]),
		"altitude"  :  float(data[8]),
		"bearing"   :  float(data[9]),
		"velocity"  :  float(data[10])
	}

	y = json.dumps(pack)
	print(y)
	headers = {'Content-type': 'application/json'}
	r = requests.post('http://localhost/upload_track.php', data=y, headers=headers)
	print(f"Status Code: {r.status_code}, Response: {r.text}")


