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
		text = unichr(0xeb28 - 200 + entry["weather"][0]["id"])

		w, h = draw.textsize(text, weatherIcons)
		posX = poscb(w, h)
		draw.text((posX, height / 2 - h / 2), text, 0, weatherIcons)

		iconCenterX = posX + w / 2
		iconH = h

		text = str(int(entry["main"]["temp"])) + u"°C"
		w, h = draw.textsize(text, font)
		draw.text((iconCenterX - w / 2, height / 2 + iconH / 2), text, 0, font)

		text = datetime.datetime.fromtimestamp(entry["dt"]).strftime("%H:%M")
		w, h = draw.textsize(text, font)
		draw.text((iconCenterX - w / 2, height / 2 - iconH / 2 - h - padding), text, 0, font)

	drawEntry(data["list"][0], lambda w, h: padding)
	drawEntry(data["list"][1], lambda w, h: padding + weatherWidth / 2 - w / 2)
	drawEntry(data["list"][2], lambda w, h: weatherWidth - w)

img.save("output.png")