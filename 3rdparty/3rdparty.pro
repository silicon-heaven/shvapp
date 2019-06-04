TEMPLATE = subdirs
CONFIG += ordered

!debpkg {
SUBDIRS += \
	libshv/3rdparty/necrolog/libnecrolog \
	libshv/libshvchainpack \
	libshv/libshvcore \
	libshv/libshvcoreqt \
    libshv/libshviotqt \
    libshv/libshvvisu \
    libshv/utils \
	libshv/tests \

}

