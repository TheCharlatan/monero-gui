package=libgpg-error
$(package)_version=1.39
$(package)_download_path=ftp://ftp.gnupg.org/gcrypt/$(package)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=3d4dc56588d62ff01067af192e2d3de38ef4c060857ed8da77327365477569ca

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

