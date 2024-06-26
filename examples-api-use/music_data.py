import requests
import json
import base64
import os
from time import sleep

# CLIENT CREDENTIALS ENCODED
CLIENT_ID = "90a8671600fe4fdebb0f0124d8f1fe58"
CLIENT_SECRET = "1e613067ace0401cbff1f65fd94176bd"

client_credentials = f"{CLIENT_ID}:{CLIENT_SECRET}"
encoded_credentials = base64.b64encode(client_credentials.encode()).decode()

# TOKEN ACCESSING/FILE READING
TOKEN_URL = "https://accounts.spotify.com/api/token"

def calibrateTokenData():
	with open('/home/justin/rpi-rgb-led-matrix/examples-api-use/token_data.txt') as f:
		token_data = json.load(f)

	with open('/home/justin/rpi-rgb-led-matrix/examples-api-use/refresh_token.txt') as f:
		refresh_token = json.load(f)

	print(f"TOKEN DATA CALIBRATED\n")

	global TOKEN_TYPE
	global ACCESS_TOKEN
	global REFRESH_TOKEN
	global AUTH_HEADER

	TOKEN_TYPE = token_data["token_type"]
	ACCESS_TOKEN = token_data["access_token"]
	REFRESH_TOKEN = refresh_token["refresh_token"]

	auth_string = f"{TOKEN_TYPE} {ACCESS_TOKEN}"

	AUTH_HEADER = {
		"Authorization" : auth_string
	}

def checkTokenExp(status_code):
	if(status_code==401):
		# need to send refresh req for new token
		REFRESH_PARAMS = {
			"grant_type" : "refresh_token",
			"refresh_token" : REFRESH_TOKEN
			}

		REFRESH_HEADERS = {
			"Content-Type" : "application/x-www-form-urlencoded",
			"Authorization" : f"Basic {encoded_credentials}"
			}

		refresh_resp = requests.post(TOKEN_URL, params=REFRESH_PARAMS, headers=REFRESH_HEADERS)
		if(refresh_resp.status_code==200):
			with open('/home/justin/rpi-rgb-led-matrix/examples-api-use/token_data.txt', 'w') as f:
  				json.dump(refresh_resp.json(), f, ensure_ascii=False)
			calibrateTokenData()
		else:
			print(f"ERROR: {refresh_resp.status_code}")
			print(refresh_resp.text)
		return True
	else:
		return False

calibrateTokenData()

#print(f"AUTH STRING = {auth_string}")


# Start user playback
PLAYER_URL = "https://api.spotify.com/v1/me/player"

START_PB_URL = "/play"

#start_pb_resp = requests.put((PLAYER_URL+START_PB_URL), headers=AUTH_HEADER)
#print(start_pb_resp.status_code)

CURR_PLAY_URL = "/currently-playing"

# Open fifos
cmd_fd = open("/home/justin/rpi-rgb-led-matrix/examples-api-use/cmd_cc_to_py", "r")
print("Cmd fifo opened on python")
data_fd = open("/home/justin/rpi-rgb-led-matrix/examples-api-use/data_py_to_cc", "w")
print("Data fifo opened on python")


sleep(1)

while True:

	print("Checking for cmd..")
	# Parse cmd from cmd fifo
	cmd_rx = cmd_fd.read(64).replace('\x00', '')
	print(repr(cmd_rx))
	print(len(cmd_rx))

	if(cmd_rx == "spotify_curr_playing"):

		print("Received cmd for spotify curr playing")

		token_attempt_count = 0
		while token_attempt_count < 10:
			curr_play_resp = requests.get((PLAYER_URL+CURR_PLAY_URL), headers=AUTH_HEADER)
			print(curr_play_resp.status_code)

			if(checkTokenExp(curr_play_resp.status_code)):
			# Try again
				token_attempt_count += 1
			else:
				token_attempt_count = 10

			sleep(2)


		if(token_attempt_count >= 10):
			print("ERROR: Could not get tokens to refresh")

		#print(curr_play_resp.json())
		music_data_dict = curr_play_resp.json()

		song_progress = music_data_dict['progress_ms']
		song_name = music_data_dict['item']['name']
		song_length = music_data_dict['item']['duration_ms']
		artist_name = music_data_dict['item']['album']['artists'][0]['name'].upper()
		is_playing = music_data_dict['is_playing']

		print(music_data_dict['progress_ms'])
		print(music_data_dict['item']['name'])
		print(music_data_dict['item']['duration_ms'])
		print(artist_name)
		print(is_playing)
		# Image downloading
		image_url = music_data_dict['item']['album']['images'][2]['url']
		save_as = '/home/justin/rpi-rgb-led-matrix/examples-api-use/spotify_album.png'

		print(image_url)

		response = requests.get(image_url)
		with open(save_as, 'wb') as file:
			file.write(response.content)

		# Format data to send over fifo
		data_tx = f"{song_progress},{song_length},{song_name},{artist_name},{is_playing},"

		# Send data over the data fifo
		data_fd.write(data_tx)
		data_fd.flush()

		print("Sent currently playing data")

	sleep(0.5)
