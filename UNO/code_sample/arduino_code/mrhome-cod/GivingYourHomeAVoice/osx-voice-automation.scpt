with timeout of 2629743 seconds
  set exitApp to "no"
  repeat while exitApp is "no"
    -- <callout id="code.osx-voice-automation.scpt.speechrecognizer"/>
    tell application "SpeechRecognitionServer"
      activate
      try
        set voiceResponse to listen for {"light on", "light off", ¬
          "unlock door", "play music", "pause music", ¬
          "unpause music", "stop music", "next track", ¬
          "raise volume", "lower volume", ¬
          "previous track", "check email", "time", "make a call", ¬
          "hang up", "quit app"} giving up after 2629743
      on error -- time out
        return
      end try
    end tell
    
    -- <callout id="code.osx-voice-automation.scpt.lighton"/>
    if voiceResponse is "light on" then
      -- open URL to turn on Light Switch  
      open location "http://192.168.1.100:3344/command/on"
      say "The light is now on."
      
    else if voiceResponse is "light off" then
      -- open URL to turn off Light Switch
      open location "http://192.168.1.100:3344/command/off"
      say "The light is now off."
      
    else if voiceResponse is "unlock door" then
      -- open URL to unlock Android Door Lock
      open location "http://192.168.1.230:8000"
      say "Unlocking the door."
      
      -- <callout id="code.osx-voice-automation.scpt.playmusic"/>
    else if voiceResponse is "play music" then
      tell application "iTunes"
        set musicList to {"Cancel"} as list
        set myList to (get artist of every track ¬
          of playlist 1) as list
        
        repeat with myItem in myList
          if musicList does not contain myItem then
            set musicList to musicList & myItem
          end if
        end repeat
      end tell
      
      say "Which artist would you like to listen to?"
      tell application "SpeechRecognitionServer"
        set theArtistListing to (listen for musicList with prompt musicList)
      end tell
      
      if theArtistListing is not "Cancel" then
        say "Which of " & theArtistListing & ¬
          "'s albums would you like to listen to?"
        tell application "iTunes"
          tell source "Library"
            tell library playlist 1
              set uniqueAlbumList to {}
              set albumList to album of tracks ¬
                where artist is equal to theArtistListing
              
              repeat until albumList = {}
                if uniqueAlbumList does not contain (first item of albumList) then
                  copy (first item of albumList) to end of uniqueAlbumList
                end if
                set albumList to rest of albumList
              end repeat
              -- set response to (choose from list uniqueAlbumList) as string
              
              set theUniqueAlbumList to {"Cancel"} & uniqueAlbumList
              tell application "SpeechRecognitionServer"
                set theAlbum to (listen for the theUniqueAlbumList with prompt ¬
                  theUniqueAlbumList)
              end tell
            end tell
            if theAlbum is not "Cancel" then
              if not ((name of playlists) contains "Current Album") then
                set theAlbumPlaylist to make new playlist with properties ¬
                  {name:"Current Album"}
              else
                set theAlbumPlaylist to playlist "Current Album"
                delete every track of theAlbumPlaylist
              end if
              tell library playlist 1 to duplicate ¬
                (every track whose album is theAlbum) to theAlbumPlaylist
              play theAlbumPlaylist
            else
              say "Cancelling music selection"
            end if
          end tell
        end tell
      else
        say "Cancelling music selection"
      end if
      
      -- <callout id="code.osx-voice-automation.scpt.miscmusic"/>
    else if voiceResponse is "pause music" or voiceResponse is "unpause music" then
      tell application "iTunes"
        playpause
      end tell
      
    else if voiceResponse is "stop music" then
      tell application "iTunes"
        stop
      end tell
      
    else if voiceResponse is "next track" then
      tell application "iTunes"
        next track
      end tell
      
    else if voiceResponse is "previous track" then
      tell application "iTunes"
        previous track
      end tell
      
      -- <callout id="code.osx-voice-automation.scpt.volume"/>      
      -- Raise and lower volume routines courtesy of HexMonkey's post at  
      -- http://forums.macrumors.com/showthread.php?t=144749  
    else if voiceResponse is "raise volume" then
      set currentVolume to output volume of (get volume settings)
      set scaledVolume to round (currentVolume / (100 / 16))
      set scaledVolume to scaledVolume + 1
      if (scaledVolume > 16) then
        set scaledVolume to 16
      end if
      set newVolume to round (scaledVolume / 16 * 100)
      set volume output volume newVolume
      
    else if voiceResponse is "lower volume" then
      set currentVolume to output volume of (get volume settings)
      set scaledVolume to round (currentVolume / (100 / 16))
      
      set scaledVolume to scaledVolume - 1
      if (scaledVolume < 0) then
        set scaledVolume to 0
      end if
      
      set newVolume to round (scaledVolume / 16 * 100)
      set volume output volume newVolume
      
      -- <callout id="code.osx-voice-automation.scpt.checkmail"/>
    else if voiceResponse is "check email" then
      tell application "Mail"
        activate
        check for new mail
        set unreadEmailCount to unread count in inbox
        if unreadEmailCount is equal to 0 then
          say "You have no unread messages in your Inbox."
        else if unreadEmailCount is equal to 1 then
          say "You have 1 unread message in your Inbox."
        else
          say "You have " & unreadEmailCount & ¬
            " unread messages in your Inbox."
        end if
        if unreadEmailCount is greater than 0 then
          say "Would you like me to read your unread email to you?"
          tell application "SpeechRecognitionServer"
            activate
            set voiceResponse to listen for {"yes", "no"} ¬
              giving up after 1 * minutes
          end tell
          if voiceResponse is "yes" then
            set allMessages to every message in inbox
            repeat with aMessage in allMessages
              if read status of aMessage is false then
                set theSender to sender of aMessage
                set {savedDelimiters, AppleScript's text item delimiters} ¬
                  to {AppleScript's text item delimiters, "<"}
                set senderName to first text item of theSender
                set AppleScript's text item delimiters to savedDelimiters
                say "From " & senderName
                say "Subject: " & subject of aMessage
                delay 1
              end if
            end repeat
          end if
        end if
      end tell
      
      -- <callout id="code.osx-voice-automation.scpt.time"/>
    else if voiceResponse is "time" then
      set current_time to (time string of (current date))
      set {savedDelimiters, AppleScript's text item delimiters} to ¬
        {AppleScript's text item delimiters, ":"}
      set hours to first text item of current_time
      set minutes to the second text item of current_time
      set AMPM to third text item of current_time
      set AMPM to text 3 thru 5 of AMPM
      set AppleScript's text item delimiters to savedDelimiters
      say "The time is " & hours & " " & minutes & AMPM
      
      -- <callout id="code.osx-voice-automation.scpt.skype"/>
      --else if voiceResponse is "make a call" then
      --  tell application "Skype"
      -- -- A Skype API Security dialog will pop up first 
      -- -- time accessing Skype with this script.
      -- -- Select "Allow this application to use Skype" for ¬
      -- -- uninterrupted Skype API access.
      --    activate
      --   -- replace echo123 Skype Call Testing Service ID with ¬
      --   -- phone number or your contact's Skype ID
      --      send command "CALL echo123" script name "Place Skype Call"
      --    end tell
      
      --  else if voiceResponse is "hang up" then
      --    tell application "Skype"
      --      quit
      --  end tell
      
      -- <callout id="code.osx-voice-automation.scpt.quit"/>
    else if voiceResponse is "quit app" then
      set exitApp to "yes"
      say "Listening deactivated. Exiting application."
      delay 1
      do shell script "killall SpeechRecognitionServer"
    end if
  end repeat
end timeout