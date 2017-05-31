PKG_NAME=openthread
PKG_URL=https://github.com/openthread/openthread.git
PKG_VERSION=fbfd76a990b81f007957e1bd774e51bce742e53e
PKG_BUILDDIR ?= $(BINDIRBASE)/pkg/$(BOARD)/$(PKG_NAME)

ifneq (,$(filter libopenthread-cli-ftd,$(USEMODULE)))
    $(info Compile OpenThread for FTD device)
    OPENTHREAD_ARGS+= --enable-cli-app=ftd
endif
ifneq (,$(filter libopenthread-cli-mtd,$(USEMODULE)))
    $(info Compile OpenThread for MTD device)
    OPENTHREAD_ARGS+= --enable-cli-app=mtd
endif
ifneq (,$(filter libopenthread-ncp-ftd,$(USEMODULE)))
    $(info Compile OpenThread with NCP)
    OPENTHREAD_ARGS += --enable-ncp-app=ftd --with-ncp-bus=uart
endif
OPENTHREAD_ARGS += --enable-application-coap
$(info $$OPENTHREAD_ARGS is [${OPENTHREAD_ARGS}])

.PHONY: all

OPENTHREAD_COMMON_FLAGS=-fdata-sections -ffunction-sections -Os
all: git-download
	cd $(PKG_BUILDDIR) && PREFIX="/" ./bootstrap
	cd $(PKG_BUILDDIR) && CPP="$(CPP)" CC="$(CC)" CXX="$(CXX)"\
		OBJC="" OBJCXX="" AR="$(AR)" RANLIB="$(RANLIB)" NM="$(NM)" \
		STRIP="$(STRIP)" \
		CPPFLAGS="$(OPENTHREAD_COMMON_FLAGS) $(CFLAGS_CPU) " \
		CFLAGS="$(OPENTHREAD_COMMON_FLAGS) $(CFLAGS_CPU) " \
		CXXFLAGS="$(OPENTHREAD_COMMON_FLAGS) $(CFLAGS_CPU) -fno-exceptions -fno-rtti " \
		LDFLAGS="$(OPENTHREAD_COMMON_FLAGS) $(CFLAGS_CPU) -nostartfiles -specs=nano.specs \
		-specs=nosys.specs -Wl,--gc-sections -Wl,-Map=map.map " \
		./configure --disable-docs --host=$(TARGET_ARCH) --target=$(TARGET_ARCH) \
		--prefix=/ --enable-default-logging ${OPENTHREAD_ARGS}
	cd $(PKG_BUILDDIR) &&  DESTDIR=$(PKG_BUILDDIR)/output PREFIX=/ make -j4 --no-print-directory install

	cp $(PKG_BUILDDIR)/output/lib/libmbedcrypto.a ${BINDIR}/libmbedcrypto.a
ifneq (,$(filter libopenthread-cli-ftd,$(USEMODULE)))
	cp $(PKG_BUILDDIR)/output/lib/libopenthread-ftd.a ${BINDIR}/libopenthread-ftd.a
	cp $(PKG_BUILDDIR)/output/lib/libopenthread-cli-ftd.a ${BINDIR}/libopenthread-cli-ftd.a
endif
ifneq (,$(filter libopenthread-cli-mtd,$(USEMODULE)))
	cp $(PKG_BUILDDIR)/output/lib/libopenthread-mtd.a ${BINDIR}/libopenthread-mtd.a
	cp $(PKG_BUILDDIR)/output/lib/libopenthread-cli-mtd.a ${BINDIR}/libopenthread-cli-mtd.a
endif
ifneq (,$(filter libopenthread-ncp-ftd,$(USEMODULE)))
	cp $(PKG_BUILDDIR)/output/lib/libopenthread-ftd.a ${BINDIR}/libopenthread-ftd.a
	cp $(PKG_BUILDDIR)/output/lib/libopenthread-ncp-ftd.a ${BINDIR}/libopenthread-ncp-ftd.a
endif
	sed -ie 's/BASE/_BASE/g' $(PKG_BUILDDIR)/output/include/openthread/types.h
include $(RIOTBASE)/pkg/pkg.mk
