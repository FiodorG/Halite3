del /S *.hlt
del /S *.log
call make.bat
call make2.bat 
halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
REM halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
REM halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
REM halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
REM halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
REM halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
REM halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
REM halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"

pause