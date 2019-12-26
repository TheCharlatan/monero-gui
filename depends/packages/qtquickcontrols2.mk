package=qtquickcontrols2
$(package)_version=5.9.8
$(package)_download_path=https://download.qt.io/official_releases/qt/5.9/$($(package)_version)/submodules
$(package)_file_name=$(package)-opensource-src-$($(package)_version).tar.xz
$(package)_sha256_hash=6c1e47188facca82f443e2d3d9692d5a9574db073c37e218b1d89f795b697018
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
