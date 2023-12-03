from datetime import datetime # <callout id="code.packagedelivery.py.import"/>
import packagetrack
from packagetrack import Package
import serial
import smtplib
import sqlite3
import time
import os
import sys

# Connect to the serial port
XbeePort = serial.Serial('/dev/tty.YOUR_SERIAL_DEVICE', \
  baudrate = 9600, timeout = 1) # <callout id="code.packagedelivery.py.xbee"/>

def send_email(subject, message): # <callout id="code.packagedelivery.py.send_email"/>
  recipient = 'YOUR_EMAIL_RECIPIENT@DOMAIN.COM'
  gmail_sender = 'YOUR_GMAIL_ACCOUNT_NAME@gmail.com'
  gmail_password = 'YOUR_GMAIL_ACCOUNT_PASSWORD'
  
  # Establish secure TLS connection to Gmail SMTP gateway
  gmail_smtp = smtplib.SMTP('smtp.gmail.com',587)
  gmail_smtp.ehlo()
  gmail_smtp.starttls()
  gmail_smtp.ehlo

  # Log into Gmail
  gmail_smtp.login(gmail_sender, gmail_password)

  # Format message
  mail_header = 'To:' + recipient + '\n' + 'From: ' + gmail_sender + '\n' \
      + 'Subject: ' + subject + '\n'
  message_body = message
  mail_message = mail_header + '\n ' + message_body + ' \n\n'

  # Send formatted message
  gmail_smtp.sendmail(gmail_sender, recipient, mail_message)
  print("Message sent")

  # Close connection
  gmail_smtp.close()

def process_message(msg): # <callout id="code.packagedelivery.py.process_message"/>
  try:
    # Remember to use the full correct path to the 
    # packagedelivery.sqlite file
    connection = sqlite3.connect("packagedelivery.sqlite")
    cursor = connection.cursor()
  
    # Get current date and time and format it accordingly
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
  
    sqlstatement = "INSERT INTO delivery (id, time, event) \
    VALUES(NULL, \"%s\", \"%s\")" % (timestamp, msg)
    cursor.execute(sqlstatement)
    connection.commit()
    cursor.close()
  except:
    print("Problem accessing delivery table in the " \
    + "packagedelivery database")

  if (msg == "Delivery"):
    
    # Wait 5 minutes (300 seconds) before polling the various courier
    time.sleep(300)

    try:
      connection = sqlite3.connect("packagedelivery.sqlite")
      cursor = connection.cursor()
      cursor.execute('SELECT * FROM tracking WHERE '\
      + 'delivery_status=0')
      results = cursor.fetchall()
      message = ""
      for x in results:
        tracking_number = str(x[1])
        description = str(x[2])
        print tracking_number

        package = Package(tracking_number)
        info = package.track()
        delivery_status = info.status
        delivery_date = str(info.delivery_date)

        if (delivery_status.lower() == 'delivered'):
          sql_statement = 'UPDATE tracking SET \
          delivery_status = "1", delivery_date = \
          "' + delivery_date + \ 
          '" WHERE tracking_number = "' \
           + tracking_number + '";'
          cursor.execute(sql_statement)
          connection.commit()
          message = message + description \
          + ' item with tracking number ' \
          + tracking_number \
          + ' was delivered on ' \
          + delivery_date +'\n\n'

      # Close the cursor
      cursor.close()

      # If delivery confirmation has been made, send an email
      if (len(message) > 0):
        print message
        send_email('Package Delivery Confirmation', message)
      else:
        send_email('Package Delivery Detected', 'A ' \
        + 'package delivery event was detected, ' \
        + 'but no packages with un-confirmed ' \
        + 'delivery tracking numbers in the database ' \
        + 'were able to be confirmed delivered by ' \
        + 'the courier at this time.')

    except:
      print("Problem accessing tracking table in the " \
      + "packagedelivery database")
      
  else:
    send_email('Package(s) Removed', 'Package removal detected.')

if sys.platform == "win32": # <callout id="code.packagedelivery.py.main_loop"/>
    os.system("cls")
else:
    os.system("clear")

print("Package Delivery Detector running...\n")
try:
  while 1:
    # listen for inbound characters from the Xbee radio
    xbee_message = XbeePort.readline()

    # Depending on the type of delivery message received, 
    # log and lookup accordingly
    if "Delivery" in xbee_message:
      # Get current date and time and format it accordingly
      timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
      print("Delivery event detected - " + timestamp)
      process_message("Delivery")

    if "Empty" in xbee_message:
      # Get current date and time and format it accordingly
      timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
      print("Parcel removal event detected - " + timestamp)
      process_message("Empty")

except KeyboardInterrupt: # <callout id="code.packagedelivery.py.keypress"/>
  print("\nQuitting the Package Delivery Detector.\n")
  pass
