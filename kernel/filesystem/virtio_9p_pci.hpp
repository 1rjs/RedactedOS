#pragma once

#include "fsdriver.hpp"
#include "virtio/virtio_pci.h"
#include "data_struct/indexmap.hpp"

class Virtio9PDriver : public FSDriver {
public:
    bool init(uint32_t partition_sector) override;
    FS_RESULT open_file(const char* path, file* descriptor) override;
    size_t read_file(file *descriptor, void* buf, size_t size) override;
    sizedptr list_contents(const char *path) override;
private:
    virtio_device np_dev;
    size_t choose_version();
    uint32_t open(uint32_t fid);
    uint32_t attach();
    sizedptr list_contents(uint32_t fid, uint64_t offset = 0);
    uint32_t walk_dir(uint32_t fid, char *path);
    uint64_t read(uint32_t fid, uint64_t offset, void* file);
    uint64_t get_attribute(uint32_t fid, uint64_t mask);
    size_t max_msize;
    uint32_t vfid;
    uint32_t mid;

    uint32_t root;

    IndexMap<void*> open_files;
};