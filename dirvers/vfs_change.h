#pragma once

int init_vfs_changes(void) __init;
void cleanup_vfs_changes(void);

void vfs_changed(int act, const char* root, const char* src, const char* dst);
//struct __krp_partition__ {
//		unsigned char major, minor;
//			struct list_head list;
//				char root[0];
//} krp_partition;


