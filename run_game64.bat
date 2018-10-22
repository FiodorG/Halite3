del /S *.hlt
del /S *.log
call make.bat
halite.exe --replay-directory replays/ -vvv --width 64 --height 64 --seed 4 "MyBot.exe" "MyBot.exe"

pause