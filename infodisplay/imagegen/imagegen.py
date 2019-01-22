# -*- coding: utf-8 -*-

import datetime, json, cv2, numpy as np, urllib2
from PIL import ImageFont, ImageDraw, Image  
from fritzconnection.fritzconnection import FritzConnection
from fritzconnection.fritzhosts import FritzHosts
from fritzconnection.fritzcall import FritzCall

with open("./config.json", "r") as fd:
	config = json.load(fd)

width = config["width"]
height = config["height"]
padding = config["padding"]
font = ImageFont.truetype(config["font"], config["font-size"])

img = Image.new("L", (width, height), 255)
draw = ImageDraw.Draw(img)

#
# draw the clock in the middle
#
now = datetime.datetime.now()
clock = config["clock"]
radius = clock["radius"]
draw.pieslice([width / 2 - radius, height / 2 - radius, width / 2 + radius, height / 2 + radius],
	-90, now.minute * 6 - 90, #startAngle, endAngle
	fill=0 #colour
)

clockFont = ImageFont.truetype(config["font"], clock["font-size"])
text = str(now.hour)
w, h = draw.textsize(text, clockFont)

minX = width / 2 - w / 2
maxX = width / 2 + w / 2
minY = height / 2 - h / 2
maxY = height / 2 + h / 2
draw.rectangle([minX - padding, minY - padding, maxX + padding, maxY + padding], 255)
draw.text((minX, minY), text, 0, clockFont)


#
# draw information retrieved from the fritz!box
#
fritz = FritzConnection(user=config["fritz"]["username"], password=config["fritz"]["password"])

#display the amount of connected devices
hosts = FritzHosts(fc=fritz).get_hosts_info()
hosts = len(filter(lambda x: x["status"] == "1", hosts))
text = str(hosts) + " devices online"
w, h = draw.textsize(text, font)
draw.text((width - w - padding, height - h - padding), text, 0, font)

#display the missed calls max 7 days old
calls = FritzCall(fc=fritz)
maxAge = now - datetime.timedelta(7)
missedCalls = filter(lambda x: x["Date"] > maxAge and x["Device"] == "Anrufbeantworter",
		calls.get_received_calls())
missedCalls = missedCalls + filter(lambda x: x["Date"] > maxAge, calls.get_missed_calls())
missedCalls = missedCalls[0 : config["fritz"]["max-missed-calls"]]

if len(missedCalls) > 0:
	text = "Missed calls"
	for i, call in enumerate(missedCalls):
		caller = call["Caller"] or "Unknown"
		text = text + "\n" + call["Date"].strftime("%a %H:%M") + ":\n  " + caller

	w, h = draw.textsize(text, font)
	draw.text((width - w - padding, height / 2 - h / 2), text, 0, font)

#
# draw weather information
#
weatherIcons = ImageFont.truetype("owfont-regular.ttf", 60)
weatherConfig = config["weather"]

url = "http://api.openweathermap.org/data/2.5/forecast?cnt=3&id={}&units={}&appid={}" \
	.format(weatherConfig["city"], weatherConfig["units"], weatherConfig["api-key"])
try:
	data = json.load(urllib2.urlopen(url))
except: #go pikachu
	data = None

if data != None:
	weatherWidth = width / 2 - clock["radius"] - 2 * padding

	def drawEntry(entry, poscb):
		text1 = datetime.datetime.fromtimestamp(entry["dt"]).strftime("%H:%M")
		text2 = unichr(0xea60 + entry["weather"][0]["id"])
		text3 = str(int(entry["main"]["temp"])) + u"°C"

		w1, h1 = draw.textsize(text1, font)
		w2, h2 = draw.textsize(text2, weatherIcons)
		w3, h3 = draw.textsize(text3, font)

		posY1 = height / 2 - h1 - h2 - h3 - 4 * padding
		posY2 = height / 2 - h2 - h3 - 4 * padding
		posY3 = height / 2 - h3 - 3 * padding

		posX = poscb(w2, h2)
		iconCenterX = posX + w2 / 2

		draw.text((posX, posY2), text2, 0, weatherIcons)
		draw.text((iconCenterX - w1 / 2, posY1), text1, 0, font)
		draw.text((iconCenterX - w3 / 2, posY3), text3, 0, font)

	drawEntry(data["list"][0], lambda w, h: padding)
	drawEntry(data["list"][1], lambda w, h: padding + weatherWidth / 2 - w / 2)
	drawEntry(data["list"][2], lambda w, h: padding + weatherWidth - w)


#
# draw bus information
#
with open("./bus.json", "r") as fd:
	busInfo = json.load(fd)

def findNextDepartures(departures):
	result = []
	now = datetime.datetime.now()
	now = now.hour * 60 + now.minute

	for time in departures:
		split = time.split(":")
		time = int(split[0]) * 60 + int(split[1])

		if time > now:
			result.append(time)

		# add the departure time for tomorrow
		result.append(24 * 60 + time)

	return sorted(result)

def departureTimeToStr(time):
	if time > 24 * 60:
		time = time - 24 * 60

	return "%d:%d" % (time / 60, time % 60)

def drawBusDepartures(name, times, posXCb):
	text1 = name
	text2 = "\n".join(map(departureTimeToStr, times[0 : 2]))

	w1, h1 = draw.textsize(text1, font)
	w2, h2 = draw.textsize(text2, font)

	posX = posXCb(max(w1, w2), h1 + h2 + padding)
	posY1 = height / 2 + 3 * padding
	posY2 = height / 2 + h1 + 4 * padding

	draw.text((posX, posY1), text1, 0, font)
	draw.text((posX, posY2), text2, 0, font)



#this is currently hard coded to two lines and a bus icon in the middle
busInfoWidth = width / 2 - clock["radius"] - 2 * padding
#fontAwesome = ImageFont.truetype("fontawesome-regular.ttf", 20)

# font awesome is somehow broken, see python-pillow/Pillow issue 3601
# for now we use a placeholder
text = "XXXX\nXXXX\nXXXX"
w, h = draw.textsize(text, font)
x = padding + busInfoWidth / 2 - w / 2
y = height / 2 + 3 * padding
draw.text((x, y), text, 0, font)

iconW = w
times1 = findNextDepartures(busInfo[0]['departures'])
times2 = findNextDepartures(busInfo[1]['departures'])
drawBusDepartures(busInfo[0]['name'], times1, lambda w, h: x - padding - w)
drawBusDepartures(busInfo[1]['name'], times2, lambda w, h: x + padding + iconW)



img.save("output.png")