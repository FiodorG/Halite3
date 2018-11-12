del /S *.hlt
del /S *.log
call make.bat 
call make2.bat 
halite.exe --replay-directory replays/ -vvv --width 32 --height 32 --seed 1540699194 "MyBot.exe" "MyBot2.exe"
%--no-timeout 

pause