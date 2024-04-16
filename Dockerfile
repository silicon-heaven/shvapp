FROM exoti/docker-debian-bookworm:latest

ARG qt_version=6.5.3
ARG COMMIT_SHA=000000

SHELL ["bash", "-e", "-u", "-x", "-o", "pipefail", "-O", "inherit_errexit", "-c"]


USER root
RUN apt -y install liblua5.4-dev
USER build-user


ADD --chown=build-user . /home/build-user/shv
RUN --mount=id=eline-shv,type=cache,target=$HOME/.ccache,uid=1000,gid=1000 <<EOF
    CXXFLAGS="-DGIT_COMMIT=${COMMIT_SHA}" cmake \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$HOME/shv-install/usr" \
        -DCMAKE_PREFIX_PATH="$HOME/${qt_version}/gcc_64" \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DUSE_QT6=ON \
	-DWITH_LDAP=ON \
        -G Ninja \
        -B "$HOME/shv-build" \
        -S "$HOME/shv"
EOF

RUN --mount=id=eline-shv,type=cache,target=$HOME/.ccache,uid=1000,gid=1000 cmake --build "$HOME/shv-build"
RUN --mount=id=eline-shv,type=cache,target=$HOME/.ccache,uid=1000,gid=1000 cmake --install "$HOME/shv-build"
RUN PATH="$HOME/${qt_version}/gcc_64/bin:$PATH" \
    LDAI_OUTPUT="$HOME/shv-x86_64.AppImage" \
    LD_LIBRARY_PATH="$HOME/shv-install/usr/lib:$HOME/${qt_version}/gcc_64/lib" \
    APPIMAGE_EXTRACT_AND_RUN=1 \
    "$HOME/linuxdeploy-x86_64.AppImage" \
        --appdir "$HOME/shv-install" \
        --desktop-file "$HOME/shv/distro/shv.AppDir/shv.desktop" \
        --icon-file "$HOME/shv/distro/shv.AppDir/shv.svg" \
        --plugin qt \
        --custom-apprun "$HOME/shv/distro/shv.AppDir/AppRun" \
        --output appimage
