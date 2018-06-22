# -*- coding: utf-8 -*-

import datetime, json, cv2, numpy as np, fritzconnection
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

#display the missed calls since the last outgoing one max 7 days old
calls = FritzCall(fc=fritz)
lastOutCall = max(calls.get_out_calls(), key=lambda x: x["Date"])["Date"]
sevenDaysAgo = now - datetime.timedelta(7)
maxAge = max(lastOutCall, sevenDaysAgo)
if lastOutCall > sevenDaysAgo:
	maxAge = lastOutCall
else:
	maxAge = sevenDaysAgo

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
#TODO



img.save("output.png")