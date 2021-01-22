TEMPLATE = subdirs
CONFIG += ordered

!debpkg {
SUBDIRS += \
    necrolog/libnecrolog \
	libshv/libshvchainpack \
	libshv/libshvcore \
	libshv/libshvcoreqt \
	libshv/libshviotqt \
	libshv/libshvbroker \
	libshv/samples \
	libshv/utils \
	libshv/tests \

qtHaveModule(gui) {
SUBDIRS += \
    libshv/libshvvisu \

}
}

