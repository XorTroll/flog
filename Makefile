ifndef KSD_FS
$(error KSD_FS is not set)
endif

FLOG = flog
SRC_DIR = $(FLOG)/source

.PHONY: main clean ksd_fs
.DEFAULT_GOAL := main

ksd_fs:
	@python build_ksd_fs.py --cpp $(KSD_FS) $(SRC_DIR)/ksd_FileSystem.gen.cpp

main: ksd_fs
	@$(MAKE) -C flog

clean:
	@$(MAKE) clean -C flog