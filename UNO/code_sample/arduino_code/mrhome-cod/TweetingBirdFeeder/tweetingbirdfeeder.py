# import DateTime, Serial port, SQLite3 and Twitter python libraries
from datetime import datetime
import serial
import sqlite3
import twitter

# import the os module to clear the terminal window at start of the program
# windows uses "cls" while Linux and OSX use the "clear" command
import os
if sys.platform == "win32":
    os.system("cls")
else:
    os.system("clear")

# Connect to the serial port
XbeePort = serial.Serial('/dev/tty.YOUR_SERIAL_DEVICE', \
						baudrate = 9600, timeout = 1)

# Connect to SQLite database file
sqlconnection = sqlite3.connect("tweetingbirdfeeder.sqlite3")

# create database cursor
sqlcursor = sqlconnection.cursor()

# Initialize Twitter API object
api = twitter.Api('Your_OAuth_Consumer_Key', 'Your_OAuth_Consumer_Secret', \
		'Your_OAuth_Access_Token', 'Your_OAuth_Access_Token_Secret')

def transmit(msg):
	# Get current date and time and format it accordingly
	timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
	
	# Determine message and assign response parameters
	if msg == "arrived":
		tweet = "A bird has landed on the perch!"
		table = "birdfeeding"

	if msg == "departed":
		tweet = "A bird has left the perch!"
		table = "birdfeeding"

	if msg == "refill":
		tweet = "The feeder is empty."
		table = "seedstatus"

	if msg == "seedOK":
		tweet = "The feeder has been refilled with seed."
		table = "seedstatus"
	
	print "%s - %s" % (timestamp.strftime("%Y-%m-%d %H:%M:%S"), tweet)
	
	# Store the event in the SQLite database file
	try:
        sqlstatement = "INSERT INTO %s (id, time, event) \
		VALUES(NULL, \"%s\", \"%s\")" % (table, timestamp, msg)
		sqlcursor.execute(sqlstatement)
		sqlconnection.commit()
    except:
        print "Could not store event to the database."
        pass
	
	# Post message to Twitter
	try:
		status = api.PostUpdate(msg)
	except:
		print "Could not post Tweet to Twitter"
		pass

# Main program loop
try:
	while 1:
		# listen for inbound characters from the feeder-mounted Xbee radio
		message = XbeePort.readline()
		
		# Depending on the type of message is received, 
		# log and tweet it accordingly
		if "arrived" in message:
			transmit("arrived")

		if "departed" in message:
			transmit("departed")
			
		if "refill" in message:
			transmit("refill")
			
		if "seedOK" in message:
			transmit("seedOK")
		
except KeyboardInterrupt:
	# Exit the program when the Control-C keyboard interrupt been detected
	print("\nQuitting the Tweeting Bird Feeder Listener Program.\n")
	sqlcursor.close()
	pass