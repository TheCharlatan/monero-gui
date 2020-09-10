package=libgcrypt
$(package)_version=1.8.6
$(package)_download_path=ftp://ftp.gnupg.org/gcrypt/$(package)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=a774b12be3fbd50cce1a3c74d0a31c208546c835504ae49f3181787aea6ce59e
$(package)_dependencies=libgpg-error

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

