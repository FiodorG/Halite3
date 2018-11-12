del /S *.hlt
del /S *.log
call make.bat
halite.exe --replay-directory replays/ -vvv --width 56 --height 56 --seed 1541480703 "MyBot.exe" "MyBot.exe"

pause