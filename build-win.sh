echo "making SHV" $WORKSPACE
export PATH=/c/mingw-12/x86_64-12.1.0-release-posix-seh-rt_v10-rev3/mingw64/bin:/c/Qt5/Tools/CMake_64/bin:$PATH
export CXXFLAGS="-DGIT_COMMIT=${CI_COMMIT_SHA} -DGIT_BRANCH=${CI_COMMIT_REF_SLUG} -DBUILD_ID=${CI_PIPELINE_ID}"

cmake.exe \
    -DWITH_SHV_WEBSOCKETS=OFF \
    -G "MinGW Makefiles" \
    -DCMAKE_PREFIX_PATH=C:/Qt5/5.15.2/mingw81_64 \
    -DCMAKE_INSTALL_PREFIX=. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DUSE_QT6=OFF \
    . || exit 2

cmake.exe --build . -j8 || exit 2

# We're installing into the build directory, all the .iss scripts depend on that.
cmake.exe --install . || exit 2

#[ -d bin/translations ] || mkdir bin/translations  || exit 2

JN50VIEW_VERSION=`grep APP_VERSION clients/jn50view/src/version.h | cut -d\" -f2`
BFSVIEW_VERSION=`grep APP_VERSION clients/bfsview/src/version.h | cut -d\" -f2`
#FLATLINE_VERSION=`grep APP_VERSION clients/flatline/src/version.h | cut -d\" -f2`
#SHVSPY_VERSION=`grep APP_VERSION shvspy/src/version.h | cut -d\" -f2`

echo "making jn50view-doc"
cd clients/jn50view/doc/help
c:/Qt/Tools/mingw730_64/bin/mingw32-make.exe html || exit 2
cd ../../../..

#"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${VERSION}" "-SdefaultSignTool=C:\Program Files (x86)\Windows Kits\10\bin\x86\signTool.exe sign /f C:\Certificates\Elektroline_code_signing.p12 /p 123456789asdfghjkl /t http://timestamp.comodoca.com/authenticode \$f" brclab/brclab.iss  || exit 2
#"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${SHVSPY_VERSION}" shvspy/shvspy.iss  || exit /b 2
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${BFSVIEW_VERSION}" clients/bfsview/bfsview.iss || exit /b 2
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${JN50VIEW_VERSION}" clients/jn50view/jn50view.iss || exit /b 2
#"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "-DVERSION=${FLATLINE_VERSION}" clients/flatline/flatline.iss || exit /b 2




