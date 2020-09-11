package=qtmultimedia
$(package)_version=5.15.1
$(package)_download_path=https://download.qt.io/official_releases/qt/5.15/$($(package)_version)/submodules
$(package)_file_name=$(package)-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=ed6e75bec9c98559c0fbc91ff746185b1e1845139b2c7a5a843e1e8880697d99
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
