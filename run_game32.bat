del /S *.hlt
del /S *.log
call make.bat 
halite.exe --replay-directory replays/ -vvv --width 32 --height 32 --seed 1542037006 "MyBot.exe" "MyBot.exe"
%--no-timeout 

pause