del /S *.hlt
del /S *.log
call make.bat 
halite.exe --replay-directory replays/ -vvv --width 48 --height 48 --seed 1540693439 "MyBot.exe" "MyBot.exe"
%--no-timeout 

pause