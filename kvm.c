#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm-generic/ioctl.h>
#include <linux/kvm.h>
#include <fcntl.h>
#include <linux/falloc.h>

#define KVM_CAP_VM_TYPES 232
#define KVM_X86_DEFAULT_VM	0
#define KVM_X86_SW_PROTECTED_VM	1

#define KVM_CREATE_GUEST_MEMFD _IOWR(KVMIO,  0xd4, struct kvm_create_guest_memfd)
#define KVM_GUEST_MEMFD_ALLOW_HUGEPAGE         (1ULL << 0)

#define KVM_SET_USER_MEMORY_REGION2 _IOW(KVMIO, 0x49, \
                                      struct kvm_userspace_memory_region2)
//#define KVM_MEM_PRIVATE         (1UL << 2)

struct kvm_create_guest_memfd {
        __u64 size;
        __u64 flags;
        __u64 reserved[6];
};

struct kvm_userspace_memory_region2 {
        __u32 slot;
        __u32 flags;
        __u64 guest_phys_addr;
        __u64 memory_size;
        __u64 userspace_addr;
        __u64 gmem_offset;
        __u32 gmem_fd;
        __u32 pad1;
        __u64 pad2[14];
};

/*
#define KVMIO 0xAE
#define KVM_GET_API_VERSION       _IO(KVMIO,   0x00)
#define KVM_CREATE_VM             _IO(KVMIO,   0x01)
#define KVM_CHECK_EXTENSION       _IO(KVMIO,   0x03)

 * static struct file_operations kvm_chardev_ops = {
 *         .unlocked_ioctl = kvm_dev_ioctl,
 *         .llseek         = noop_llseek,
 *         KVM_COMPAT(kvm_dev_ioctl),
 * };
 *
 * static struct miscdevice kvm_dev = {
 *         KVM_MINOR,
 *         "kvm",
 *         &kvm_chardev_ops,
 * };
 *
 * r = misc_register(&kvm_dev);
 *
 * static const struct file_operations kvm_vm_fops = {
 *         .release        = kvm_vm_release,
 *         .unlocked_ioctl = kvm_vm_ioctl,
 *         .llseek         = noop_llseek,
 *         KVM_COMPAT(kvm_vm_compat_ioctl),
 * }
 */

char guestmem[33554432];

int main()
{
    int devfd, gmemfd, ret;
    int vmfd;
    int nr_slots;
    struct kvm_userspace_memory_region2 mem;
    struct kvm_create_guest_memfd gmem;
    int type;
    unsigned long size = 0x200000000;

    if ((devfd = open("/dev/kvm", O_RDWR)) < 0) {
	    printf("/devkvm open failed errno = %d \n", errno);
	    exit(devfd);
    }

    ret = ioctl(devfd, KVM_GET_API_VERSION, 0);
    if (ret < 0) {
	    printf("/devkvm open failed errno = %d \n", errno);
	    goto out;
    }
    printf("KVM_API_VERSION successful = 0x%x\n", ret);

    if((nr_slots = ioctl(devfd, KVM_CHECK_EXTENSION, KVM_CAP_NR_MEMSLOTS)) < 0) {
	    printf("extension KVM_CHECK_EXTENSION failed = %d \n", errno);
	    goto out;
    }
    printf("KVM_CAP_NR_MEMSLOTS successful = %d\n", nr_slots);

    if((ret = ioctl(devfd, KVM_CHECK_EXTENSION, KVM_CAP_VM_TYPES)) < 0) {
	    printf("extension KVM_CHECK_EXTENSION KVM_CAP_VM_TYPES failed = %d \n", errno);
	    goto out;
    }

    if (ret ^ ((1 < KVM_X86_DEFAULT_VM) | (1 < KVM_X86_SW_PROTECTED_VM)) == 0)
        printf("KVM_CHECK_EXTENSION Success = KVM_X86_DEFAUL_VM|KVM_X86_SW_PROTECTED_VM =0x%x\n", ret);
    else 
	printf("KVM_CHECK_EXTENSION failed ret = %x\n", ret);

    type = KVM_X86_SW_PROTECTED_VM;
    if((vmfd = ioctl(devfd, KVM_CREATE_VM, type)) < 0) {
	    printf("KVM_CREWATE_VM failed = %d \n", -errno);
	    goto out;
    }
    printf("KVM_CREATE_VM type %s Succeeded\n",  
	    KVM_X86_DEFAULT_VM ? "KVM_X86_DEFAULT_VM" : "KVM_X86_SW_PROTECTED_VM");

    gmem.size = size;
    gmem.flags =  KVM_GUEST_MEMFD_ALLOW_HUGEPAGE;
    if ((gmemfd = ioctl(vmfd, KVM_CREATE_GUEST_MEMFD, &gmem)) < 0) {
	    printf("KVM_CREATE_GUEST_MEMFD failed = %d \n", -errno);
	    goto out1;
    }
    printf("KVM_GUEST_MEMFD_HUGE_PMD success gmemfd=%d\n", gmemfd);

    if (fallocate(gmemfd, FALLOC_FL_KEEP_SIZE, 0, size) < 0) {
	    printf("fallocate() for %lx failed = %d \n", size, -errno);
	    goto out2;
    }
    
    printf("fallocate() success type return size = %lx\n", size);
    getchar();

    if (fallocate(gmemfd, FALLOC_FL_KEEP_SIZE| FALLOC_FL_PUNCH_HOLE, 0, size) < 0) {
	    printf("fallocate() for %lx failed = %d \n", size, -errno);
	    goto out;
    }

    printf("fallocate() success type return\n");
    getchar();

    mem.slot=1;
    mem.guest_phys_addr = 0x10000000;
    mem.userspace_addr = ((unsigned long) &guestmem) & ~(0x1000-1);
    mem.flags = KVM_MEM_PRIVATE;
    mem.memory_size = size;
    mem.gmem_fd = gmemfd;
    mem.gmem_offset = 0;
    if ((ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION2, &mem)) < 0) {
        printf("KVM_SET_USER_MEMORY_REGION2 failed %d \n", -errno);
	goto out2;
    }
    printf("KVM_SET_USER_MEMORY_REGION2 Success\n");
    printf("type return to continue\n");
    getchar();

out2:
    close(gmemfd);


    /*
    mem.slot = 1;
    mem.userspace_addr = ((unsigned long) &guestmem) & ~(0x1000-1);
    mem.guest_phys_addr = 0x10000000;
    mem.flags = 0;
    mem.memory_size = 33554432;
    printf("mem.userspace_addr=%llx\n", mem.userspace_addr);
    ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &mem);
    if (ret < 0) {
	    printf(" KVM_SET_USER_MEMORY_REGION failed errno = %d \n", errno);
    }

    printf("KVM_KVM_SET_USER_MEMORY_REGION successful = 0x%x\n", ret);
*/
out1:
    close(vmfd);

out:
    close(devfd);
}
