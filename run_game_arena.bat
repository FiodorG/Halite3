del /S *.hlt
del /S *.log
call make.bat
call make2.bat 
halite.exe --replay-directory replays/ -vvv --width 32 --height 32 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 32 --height 32 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 32 --height 32 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 40 --height 40 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 40 --height 40 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 40 --height 40 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 48 --height 48 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 48 --height 48 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 48 --height 48 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 56 --height 56 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 64 --height 64 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 64 --height 64 "MyBot.exe" "MyBot2.exe"
halite.exe --replay-directory replays/ -vvv --width 64 --height 64 "MyBot.exe" "MyBot2.exe"
pause