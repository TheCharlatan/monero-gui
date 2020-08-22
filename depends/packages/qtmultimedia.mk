package=qtmultimedia
$(package)_version=5.15.0
$(package)_download_path=https://download.qt.io/official_releases/qt/5.15/$($(package)_version)/submodules
$(package)_file_name=$(package)-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=0708d867697f392dd3600c5c1c88f5c61b772a5250a4d059dca67b844af0fbd7
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
