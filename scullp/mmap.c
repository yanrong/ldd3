/*  -*- C -*-
 * mmap.c -- memory mapping for the scullp char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: _mmap.c.in,v 1.13 2004/10/18 18:07:36 corbet Exp $
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <asm/pgtable.h>
#include <linux/fs.h>
#include <linux/version.h>

#include "scullp.h"

/*
 * open and close: just keep track of how many times the device is
 * mapped, to avoid releasing it.
 */
void scullp_vma_open(struct vm_area_struct *vma)
{
    struct scullp_dev *dev = vma->vm_private_data;
    dev->vmas++;
}

void scullp_vma_close(struct vm_area_struct *vma)
{
    struct scullp_dev *dev = vma->vm_private_data;
    dev->vmas--;
}

/*
 * The nopage method: the core of the file. It retrieves the
 * page required from the scullp device and returns it to the
 * user. The count for the page must be incremented, because
 * it is automatically decremented at page unmap.
 *
 * For this reason, "order" must be zero. Otherwise, only the first
 * page has its count incremented, and the allocating module must
 * release it as a whole block. Therefore, it isn't possible to map
 * pages from a multipage block: when they are unmapped, their count
 * is individually decreased, and would drop to 0.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
typedef int vm_fault_t;
#endif
static vm_fault_t scullp_vma_nopage(struct vm_fault *vmf)
{
    unsigned long offset;
    struct vm_area_struct *vma = vmf->vma;
    struct scullp_dev *ptr;
    struct scullp_dev *dev = vma->vm_private_data;
    struct page *page = NULL;
    void *pageptr = NULL; /* default to "missing */
    vm_fault_t retval = VM_FAULT_NOPAGE;

    mutex_lock(&dev->mutex);
    offset = (unsigned long)(vmf->address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);

    if (offset >= dev->size) goto out; /* out of range */

	/*
	 * Now retrieve the scullp device from the list,then the page.
	 * If the device has holes, the process receives a SIGBUS when
	 * accessing the hole.
	 */
	offset >>= PAGE_SHIFT; /* offset is a number of pages */
	for (ptr = dev; ptr && offset >= dev->qset;) {
		ptr = ptr->next;
		offset -= dev->qset;
	}
	if (ptr && ptr->data) pageptr = ptr->data[offset];
	if (!pageptr) goto out; /* hole or end-of-file */
    page = virt_to_page(pageptr);

	/* got it, now increment the count */
	get_page(page);
    vmf->page = page;
    retval = 0;
out:
	mutex_unlock(&dev->mutex);
	return retval;
}

struct  vm_operations_struct scullp_vm_ops = {
    .open = scullp_vma_open,
    .close = scullp_vma_close,
    .fault = scullp_vma_nopage,
};

int scullp_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct inode *inode = filp->f_path.dentry->d_inode;

    /* refuse to map it if order is not 0 */
    if (scullp_devices[iminor(inode)].order)
        return -ENODEV;

    /* don't do anything here: "nopage" will set up page table entries */
    vma->vm_ops = &scullp_vm_ops;
    vma->vm_private_data = filp->private_data;
    scullp_vma_open(vma);
    return 0;
}
