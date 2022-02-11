: https://stackoverflow.com/questions/346107/creating-a-shortcut-for-a-exe-from-a-batch-file
: thanks to Michael Przybylski
@echo on
set VBS=makeshortcut.vbs
set SRC_LNK="shortcut1.lnk"
set ARG1_APPLCT="C:\Program Files\Google\Chrome\Application\chrome.exe"
set ARG2_APPARG="--profile-directory=QuteQProfile 25QuteQ"
set ARG3_WRKDRC="C:\Program Files\Google\Chrome\Application"
set ARG4_ICOLCT="%USERPROFILE%\Local Settings\Application Data\Google\Chrome\User Data\Profile 25\Google Profile.ico"
cscript %VBS% %SRC_LNK% %ARG1_APPLCT% %ARG2_APPARG% %ARG3_WRKDRC% %ARG4_ICOLCT%
