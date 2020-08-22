package=qtquickcontrols2
$(package)_version=5.15.0
$(package)_download_path=https://download.qt.io/official_releases/qt/5.15/$($(package)_version)/submodules
$(package)_file_name=$(package)-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=839abda9b58cd8656b2e5f46afbb484e63df466481ace43318c4c2022684648f
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
