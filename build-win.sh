echo "making shv" $WORKSPACE
PATH=/c/Qt/Tools/mingw730_64/bin:$PATH
c:/Qt/5.12.0/mingw73_64/bin/qmake.exe DEFINES+=GIT_COMMIT=${CI_COMMIT_SHA} DEFINES+=GIT_BRANCH=${CI_COMMIT_REF_SLUG} MINGW_DIR=c:/Qt/Tools/mingw730_64 -r CONFIG+=with-flatline CONFIG+=release CONFIG+=force_debug_info CONFIG+=separate_debug_info shv.pro || exit 2
c:/Qt/Tools/mingw730_64/bin/mingw32-make.exe -j4 || exit 2

#[ -d bin/translations ] || mkdir bin/translations  || exit 2

JN50VIEW_VERSION=`grep APP_VERSION clients/jn50view/src/version.h | cut -d\" -f2`
BFSVIEW_VERSION=`grep APP_VERSION clients/bfsview/src/version.h | cut -d\" -f2`
FLATLINE_VERSION=`grep APP_VERSION clients/flatline/src/version.h | cut -d\" -f2`

echo "making jn50view-doc"
cd clients/jn50view/doc/help
c:/Qt/Tools/mingw730_64/bin/mingw32-make.exe html || exit 2
cd ../../../..

#"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${VERSION}" "-SdefaultSignTool=C:\Program Files (x86)\Windows Kits\10\bin\x86\signTool.exe sign /f C:\Certificates\Elektroline_code_signing.p12 /p 123456789asdfghjkl /t http://timestamp.comodoca.com/authenticode \$f" brclab/brclab.iss  || exit 2
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" shvspy/shvspy.iss  || exit /b 2
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${BFSVIEW_VERSION}" clients/bfsview/bfsview.iss || exit /b 2
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${JN50VIEW_VERSION}" clients/jn50view/jn50view.iss || exit /b 2
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${FLATLINE_VERSION}" clients/flatline/flatline.iss || exit /b 2




