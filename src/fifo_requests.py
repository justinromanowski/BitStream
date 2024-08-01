import requests
import json
import base64
import os
from time import sleep

class SpotifyRequests:
	###################################################### CONSTANTS
	# CLIENT CREDENTIALS ENCODED
	CLIENT_ID = " "
	CLIENT_SECRET = " "

	client_credentials = " "
	encoded_credentials = " "

	# TOKEN ACCESSING/FILE READING
	TOKEN_URL = "https://accounts.spotify.com/api/token"

	# REQUEST URLS
	PLAYER_URL = "https://api.spotify.com/v1/me/player"
	START_PB_URL = "/play"
	CURR_PLAY_URL = "/currently-playing"

	TOKEN_TYPE = " "
	ACCESS_TOKEN = " "
	REFRESH_TOKEN = " "
	AUTH_HEADER = " "

	###################################################### FUNCTIONS

	def calibrateTokenData(self):
	# Opens the text files w/ token data, and updates the files w/ new data
		with open('key_data/client_data.txt') as f:
                        client_data = json.load(f)

		with open('key_data/token_data.txt') as f:
			token_data = json.load(f)

		with open('key_data/refresh_token.txt') as f:
			refresh_token = json.load(f)

		print(f"TOKEN DATA CALIBRATED\n")

		self.CLIENT_ID = client_data['CLIENT_ID']
		self.CLIENT_SECRET = client_data['CLIENT_SECRET']
		self.client_credentials = f"{self.CLIENT_ID}:{self.CLIENT_SECRET}"
		self.encoded_credentials = base64.b64encode(self.client_credentials.encode()).decode()

		self.TOKEN_TYPE = token_data["token_type"]
		self.ACCESS_TOKEN = token_data["access_token"]
		self.REFRESH_TOKEN = refresh_token["refresh_token"]

		auth_string = f"{self.TOKEN_TYPE} {self.ACCESS_TOKEN}"

		self.AUTH_HEADER = {
			"Authorization" : auth_string
			}


	def checkTokenExp(self,status_code):
	# Checks the status code of the REST API response
	# RETURNS: TRUE if status code is invalid, FALSE if status code is valid
	# Note: status code 401 is retured when refresh token has expired
		if(status_code==401):
			# need to send refresh req for new token
			self.REFRESH_PARAMS = {
				"grant_type" : "refresh_token",
				"refresh_token" : self.REFRESH_TOKEN
				}

			self.REFRESH_HEADERS = {
				"Content-Type" : "application/x-www-form-urlencoded",
				"Authorization" : f"Basic {self.encoded_credentials}"
				}

			refresh_resp = requests.post(self.TOKEN_URL, params=self.REFRESH_PARAMS, headers=self.REFRESH_HEADERS)
			if(refresh_resp.status_code==200):
				with open('key_data/token_data.txt', 'w') as f:
					json.dump(refresh_resp.json(), f, ensure_ascii=False)
				self.calibrateTokenData()
			else:
				print(f"ERROR: {refresh_resp.status_code}")
				print(refresh_resp.text)
			return True
		else:
			return False

	def requestData(self):
	# Sends the request for the data, parses the data, and organizes data into a string
	# RETURNS: string of data to send over the FIFO
		token_attempt_count = 0
		while token_attempt_count < 10:
			curr_play_resp = requests.get((self.PLAYER_URL+self.CURR_PLAY_URL), headers=self.AUTH_HEADER)
			print(curr_play_resp.status_code)

			if(self.checkTokenExp(curr_play_resp.status_code)):
			# Try again
				token_attempt_count += 1
			else:
				token_attempt_count = 10
			sleep(2)

		if(token_attempt_count >= 10):
			print("ERROR: Could not get tokens to refresh")

		if(curr_play_resp.status_code == 200):
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
			data_tx = f"200,{song_progress},{song_length},{song_name},{artist_name},{is_playing},"
			return data_tx

		elif(curr_play_resp.status_code == 204):
			data_tx = f"204"
			return data_tx
		else:
			return "error"

	def __init__(self):
		self.calibrateTokenData()
		print("Spotify class initialized")

class MLBRequests:
	######################################################## CONSTANTS
	url = "https://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard"
	yankee_game_num = -1
	yankee_game = {}
	mlb_game_data = {}

	##################################################################################
	## GAME DATA FORMATTING AND FUNDTIONS
	## Pregame = pre,away abrv,away color,away alt,home abrv,home color, home alt,first pitch time
	##
	## Ingame = in,{same team data as above},balls,strikes,outs,onFirst,onSecond,onThird,away runs,home runs,inning
	##
	## Postgame = post,{same team data as above},away runs,home runs
	##################################################################################

	def appendTeamData(self,yankee_game,str):
		away_abrv = yankee_game['competitions'][0]['competitors'][1]['team']['abbreviation']
		away_color = yankee_game['competitions'][0]['competitors'][1]['team']['color']
		away_alt = yankee_game['competitions'][0]['competitors'][1]['team']['alternateColor']

		home_abrv = yankee_game['competitions'][0]['competitors'][0]['team']['abbreviation']
		home_color = yankee_game['competitions'][0]['competitors'][0]['team']['color']
		home_alt = yankee_game['competitions'][0]['competitors'][0]['team']['alternateColor']

		str = str + "," + away_abrv + "," + away_color + "," + away_alt
		str = str + "," + home_abrv + "," + home_color + "," + home_alt

		return str


	def pregameData(self, yankee_game):
		# Get time related data and prep the string for sending
		gametime = yankee_game['status']['type']['shortDetail']
		gametime = gametime.split(" ")
		gametime = gametime[2] + "," + gametime[3]

		pre_str = "pre"
		pre_str = self.appendTeamData(yankee_game,pre_str)
		pre_str = pre_str + "," + gametime + ","

		return pre_str


	def ingameData(self, yankee_game):
		# Obtain game related data
		balls = yankee_game['competitions'][0]['situation']['balls']
		strikes = yankee_game['competitions'][0]['situation']['strikes']
		outs = yankee_game['competitions'][0]['situation']['outs']

		on_first = int(yankee_game['competitions'][0]['situation']['onFirst'])
		on_second = int(yankee_game['competitions'][0]['situation']['onSecond'])
		on_third = int(yankee_game['competitions'][0]['situation']['onThird'])

		away_runs = yankee_game['competitions'][0]['competitors'][1]['score']
		home_runs = yankee_game['competitions'][0]['competitors'][0]['score']

		inning = yankee_game['status']['period']
		top_bot = yankee_game['status']['type']['shortDetail']
		top_bot = top_bot[0]

		# Prep to transmit
		in_str = "in"
		in_str = self.appendTeamData(yankee_game, in_str)
		in_str = in_str + f",{balls},{strikes},{outs},{on_first},{on_second},{on_third},{away_runs},{home_runs},{inning},{top_bot},"

		return in_str


	def postgameData(self, yankee_game):
		# Get the final score
		away_runs = yankee_game['competitions'][0]['competitors'][1]['score']
		home_runs = yankee_game['competitions'][0]['competitors'][0]['score']

		# Prep to transmit
		post_str = "post"
		post_str = self.appendTeamData(yankee_game, post_str)
		post_str = post_str + f",{away_runs},{home_runs},"

		return post_str


	def findGame(self):
	# Finds the yankees game among the games in the schedule, if they are playing, and updates
	# the data to the "yankee_game" variable within the class
	# RETURNS: True if a Yankees game is today
	# False if there is no Yankees game today
		# Search through the list of MLB games today
		for game in range(0, 20):
			try:
				game_state = self.mlb_game_data['events'][game]['status']['type']['state']
				print(game_state)

				home_short = self.mlb_game_data['events'][game]['competitions'][0]['competitors'][0]['team']['abbreviation']
				visitor_short = self.mlb_game_data['events'][game]['competitions'][0]['competitors'][1]['team']['abbreviation']

				if(home_short == "NYY"):
					self.yankee_game_num = game
					self.yankee_game = self.mlb_game_data['events'][game]
					print(f"Yankees found at game {game} home")
					return True
				elif(visitor_short == "NYY"):
					self.yankee_game_num = game
					self.yankee_game = self.mlb_game_data['events'][game]
					print(f"Yankees found at game {game} away")
					return True

			except IndexError:
				continue

		self.yankee_game_num = -1
		self.yankee_game = {}
		return False

	def requestData(self):
	# Gets data from the ESPN API, parses the data, and formats it to be sent over the FIFO to
	# the C++ process.
	# RETURNS: string data depending on the game status, or if there is a game today
		# Initialize some variables related to the game
		balls = -1
		strikes = -1
		outs = -1
		on_first = -1
		on_second = -1
		on_third = -1

		# Send the REST Get request for the MLB schedule today
		response = requests.get(self.url)
		self.mlb_game_data = response.json()

		if(not self.findGame()):
			# No game today, return string that says no
			str = "nogame"
			return str

		if self.yankee_game_num != -1:
			#print(yankee_game)

			#print(yankee_game['competitions'][0]['competitors'][0]['team'].keys())

			yankee_game_state = self.yankee_game['status']['type']['state']
			if yankee_game_state == "pre":
				print("pre")
				gamedata = self.pregameData(self.yankee_game)
				print(gamedata)

			elif yankee_game_state == "in":
				print("in")
				gamedata = self.ingameData(self.yankee_game)
				print(gamedata)

			elif yankee_game_state == "post":
				print("post")
				gamedata = self.postgameData(self.yankee_game)
			else:
				print("error")

		return gamedata

	def __init__(self):
		print("MLB class initialized")


class WeatherRequests:
	######################################################
	## WEATHER DATA FORMATTING
	## STATUS, TEMP_0, IMG_NUM_0, PRECIP_PCT_0, TIME_0, ... , TIME_3, CITY_NAME, STATE_NAME,
	##
	## Note: Temp, img number, etc. are repeated four times in the data, first is the
	## current forecast, second is 1h forecast, third is 2h, and fourth is 3h.
	##
	###################################################### CONSTANTS
	API_KEY = " "
	LOCATION_KEY = " "

	POSTAL_CODE = "34113"
	CITY_NAME = " "
	STATE_NAME = " "

	LOCATION_URL = "http://dataservice.accuweather.com/locations/v1/postalcodes/search"
	FORECASE_URL = " "
	FORECAST_BASE = "http://dataservice.accuweather.com/forecasts/v1/hourly/12hour/"

	LOCATION_PARAMS = " "
	FORECAST_PARAMS = " "

	###################################################### FUNCTIONS

	def requestData(self):

		# Initialize variables for weather data
		temp = -1
		icon_number = -1
		precip_perc = -1
		time = -1

		weather_str = ""

		# Send GET request
		self.FORECAST_PARAMS = {
			"apikey" : self.API_KEY
			}
		weather_resp = requests.get(self.FORECAST_URL, params=self.FORECAST_PARAMS)

		if(weather_resp.status_code==200):
			weather_str = weather_str + "GOOD,"

			weather_data = weather_resp.json()

			for i in range(0,4):
				temp = int(weather_data[i]['Temperature']['Value'])
				icon_number = weather_data[i]['WeatherIcon']
				precip_perc = weather_data[i]['PrecipitationProbability']

				# Parse datetime to get AM/PM time
				datetime = weather_data[i]['DateTime']
				time = datetime[11:13]

				# Append onto weather_str to send over FIFO
				weather_str = weather_str + f"{temp}°,{icon_number},{precip_perc}%,{time},"

			# Append location
			weather_str = weather_str + f"{self.CITY_NAME},{self.STATE_NAME},"
		else:
			weather_str = "ERROR"

		return weather_str


	def requestLocationKey(self):
	# Fetches the custom AccuWeather location key for the postal code
		self.LOCATION_PARAMS = {
			"apikey" : self.API_KEY,
			"q" : self.POSTAL_CODE
			}

		location_resp = requests.get(self.LOCATION_URL, params=self.LOCATION_PARAMS)
		print(location_resp.status_code)

		if(location_resp.status_code==200):
			location_data = location_resp.json()

			location_data = location_data[0]
			self.LOCATION_KEY = location_data['Key']
			print(self.LOCATION_KEY)

			self.CITY_NAME = location_data['EnglishName']
			self.STATE_NAME = location_data['AdministrativeArea']['ID']

			self.FORECAST_URL = self.FORECAST_BASE + self.LOCATION_KEY

		else:
			printf("ERROR: Error obtaining location")


	def __init__(self):
		# Load the API key
		with open('key_data/weather_token.txt') as f:
			api_key = json.load(f)

		self.API_KEY = api_key['api_key']
		print(f"api key: {api_key}")

		self.requestLocationKey()


################################################################### MAIN LOOP
# All the main loop does is open the FIFO, read the command, send
# the appropriate request, parse and format the data, and write
# the data onto the FIFO for the C++ process to use for the display

spotify_data = SpotifyRequests()
mlb_data = MLBRequests()
weather_data = WeatherRequests()

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
	# Get string from spotify class
		print("Received cmd for spotify curr playing")

		data_tx = spotify_data.requestData()

		if(data_tx != "error"):
			data_fd.write(data_tx)
			data_fd.flush()
			print("Sent currently playing data")

	elif(cmd_rx == "mlb"):
		print("Received cmd for mlb")

		data_tx = mlb_data.requestData()

		data_fd.write(data_tx)
		data_fd.flush()
		print("Sent mlb data")

	elif(cmd_rx == "weather"):
		print("Received cmd for weather")

		data_tx = weather_data.requestData()
		data_fd.write(data_tx)
		data_fd.flush()
		print("Sent data weather")

	sleep(0.5)
