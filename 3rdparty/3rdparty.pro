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
	libshv/samples/cli/sampleshvbroker \
	libshv/samples/cli/sampleshvclient \
	libshv/utils \
	#libshv/tests \

qtHaveModule(gui) {
SUBDIRS += \
    libshv/libshvvisu \
	libshv/samples/gui/samplegraph \

}
}

