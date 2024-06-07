// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Microsoft Corporation. All rights reserved.
 */

#include "ipe.h"
#include "eval.h"
#include "hooks.h"
#include "eval.h"

extern const char *const ipe_boot_policy;
bool ipe_enabled;

static struct lsm_blob_sizes ipe_blobs __ro_after_init = {
#ifdef CONFIG_BLK_DEV_INITRD
	.lbs_superblock = sizeof(struct ipe_sb),
#endif /* CONFIG_BLK_DEV_INITRD */
#ifdef CONFIG_IPE_PROP_DM_VERITY
	.lbs_bdev = sizeof(struct ipe_bdev),
#endif /* CONFIG_IPE_PROP_DM_VERITY */
#ifdef CONFIG_IPE_PROP_FS_VERITY
	.lbs_inode = sizeof(struct ipe_inode),
#endif /* CONFIG_IPE_PROP_FS_VERITY */
#ifdef CONFIG_IPE_PROP_INTENDED_PATHNAME
	.lbs_file = sizeof(struct ipe_file),
#endif /* CONFIG_IPE_PROP_INTENDED_PATHNAME */
};

#ifdef CONFIG_BLK_DEV_INITRD
struct ipe_sb *ipe_sb(const struct super_block *sb)
{
	return sb->s_security + ipe_blobs.lbs_superblock;
}
#endif /* CONFIG_BLK_DEV_INITRD */

#ifdef CONFIG_IPE_PROP_DM_VERITY
struct ipe_bdev *ipe_bdev(struct block_device *b)
{
	return b->security + ipe_blobs.lbs_bdev;
}
#endif /* CONFIG_IPE_PROP_DM_VERITY */

#ifdef CONFIG_IPE_PROP_FS_VERITY
struct ipe_inode *ipe_inode(const struct inode *inode)
{
	return inode->i_security + ipe_blobs.lbs_inode;
}
#endif /* CONFIG_IPE_PROP_FS_VERITY */

#ifdef CONFIG_IPE_PROP_INTENDED_PATHNAME
struct ipe_file *ipe_file(const struct file *f)
{
	return f->f_security + ipe_blobs.lbs_file;
}
#endif /* CONFIG_IPE_PROP_INTENDED_PATHNAME */

static struct security_hook_list ipe_hooks[] __ro_after_init = {
	LSM_HOOK_INIT(bprm_check_security, ipe_bprm_check_security),
	LSM_HOOK_INIT(mmap_file, ipe_mmap_file),
	LSM_HOOK_INIT(file_mprotect, ipe_file_mprotect),
	LSM_HOOK_INIT(kernel_read_file, ipe_kernel_read_file),
	LSM_HOOK_INIT(kernel_load_data, ipe_kernel_load_data),
#ifdef CONFIG_BLK_DEV_INITRD
	LSM_HOOK_INIT(unpack_initramfs_security, ipe_unpack_initramfs),
#endif /* CONFIG_BLK_DEV_INITRD */
#ifdef CONFIG_IPE_PROP_DM_VERITY
	LSM_HOOK_INIT(bdev_free_security, ipe_bdev_free_security),
	LSM_HOOK_INIT(bdev_setsecurity, ipe_bdev_setsecurity),
#endif /* CONFIG_IPE_PROP_DM_VERITY */
#ifdef CONFIG_IPE_PROP_FS_VERITY
	LSM_HOOK_INIT(inode_setsecurity, ipe_inode_setsecurity),
#endif /* CONFIG_IPE_PROP_FS_VERITY */
	LSM_HOOK_INIT(file_open, ipe_file_open),
#ifdef CONFIG_IPE_PROP_INTENDED_PATHNAME
	LSM_HOOK_INIT(file_free_security, ipe_file_free_security),
	LSM_HOOK_INIT(file_set_userspace_pathname, ipe_file_set_userspace_pathname),
#endif /* CONFIG_IPE_PROP_INTENDED_PATHNAME */
};

/**
 * ipe_init - Entry point of IPE.
 *
 * This is called at LSM init, which happens occurs early during kernel
 * start up. During this phase, IPE registers its hooks and loads the
 * builtin boot policy.
 * Return:
 * * 0		- OK
 * * -ENOMEM	- Out of memory
 */
static int __init ipe_init(void)
{
	struct ipe_policy *p = NULL;

	security_add_hooks(ipe_hooks, ARRAY_SIZE(ipe_hooks), "ipe");
	ipe_enabled = true;

	if (ipe_boot_policy) {
		p = ipe_new_policy(ipe_boot_policy, strlen(ipe_boot_policy),
				   NULL, 0);
		if (IS_ERR(p))
			return PTR_ERR(p);

		rcu_assign_pointer(ipe_active_policy, p);
	}

	return 0;
}

DEFINE_LSM(ipe) = {
	.name = "ipe",
	.init = ipe_init,
	.blobs = &ipe_blobs,
};
