package=qtquickcontrols
$(package)_version=5.15.0
$(package)_download_path=https://download.qt.io/official_releases/qt/5.15/$($(package)_version)/submodules
$(package)_file_name=$(package)-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=7072cf4cd27e9f18b36b1c48dec7c79608cf87ba847d3fc3de133f220ec1acee
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
