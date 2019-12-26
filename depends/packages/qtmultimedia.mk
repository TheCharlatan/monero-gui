package=qtmultimedia
$(package)_version=5.9.8
$(package)_download_path=https://download.qt.io/official_releases/qt/5.9/$($(package)_version)/submodules
$(package)_file_name=$(package)-opensource-src-$($(package)_version).tar.xz
$(package)_sha256_hash=007881d47fb1f91dab0734385998324c8550bdd21e71bfc2f3b466311946c1a6
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
