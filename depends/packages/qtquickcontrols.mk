package=qtquickcontrols
$(package)_version=5.15.1
$(package)_download_path=https://download.qt.io/official_releases/qt/5.15/$($(package)_version)/submodules
$(package)_file_name=$(package)-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=0172f88779305aae57f3842538e91361ae9bc5ca2275ee5ce9d455309f0f2c7e
$(package)_dependencies=qt

define $(package)_config_cmds
  $(host_prefix)/native/bin/qmake
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) install
endef
