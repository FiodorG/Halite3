del /S *.hlt
del /S *.log
call make.bat
halite.exe --replay-directory replays/ -vvv --width 48 --height 48 --seed 1541332424 "MyBot.exe" "MyBot.exe"

pause