del /S *.hlt
del /S *.log
call make.bat
halite.exe --replay-directory replays/ -vvv --width 40 --height 40 --seed 1541383630 "MyBot.exe" "MyBot.exe"

pause