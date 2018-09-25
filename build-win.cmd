@echo "making shv" %WORKSPACE%

set path=c:/Qt/Tools/mingw530_32/bin
C:/Qt/5.11.1/mingw53_32/bin/qmake.exe DEFINES+=GIT_COMMIT=%CI_COMMIT_SHA% DEFINES+=GIT_BRANCH=%CI_COMMIT_REF_SLUG% -r CONFIG+=release shv.pro || exit /b 2
C:/Qt\Tools\mingw530_32\bin\mingw32-make.exe -j4 || exit /b 2

rem "C:\Program Files (x86)\Inno Setup 5\iscc.exe" "/SdefaultSignTool=C:\Program Files (x86)\Windows Kits\10\bin\x86\signTool.exe sign /f C:\Certificates\Elektroline_code_signing.p12 /p 123456789asdfghjkl /t http://timestamp.comodoca.com/authenticode $f" shvspy/shvspy.iss  || exit /b 2
rem "C:\Program Files (x86)\Inno Setup 5\iscc.exe" "/SdefaultSignTool=C:\Program Files (x86)\Windows Kits\10\bin\x86\signTool.exe sign /f C:\Certificates\Elektroline_code_signing.p12 /p 123456789asdfghjkl /t http://timestamp.comodoca.com/authenticode $f" clients/bfsview/bfsview.iss  || exit /b 2

"C:\Program Files (x86)\Inno Setup 5\iscc.exe" shvspy/shvspy.iss  || exit /b 2
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" clients/bfsview/bfsview.iss || exit /b 2
