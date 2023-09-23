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
    int devfd, ret;
    int vmfd;
    int nr_slots;
    struct kvm_userspace_memory_region mem;

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

    nr_slots = ioctl(devfd, KVM_CHECK_EXTENSION, KVM_CAP_NR_MEMSLOTS);
    if (nr_slots < 0) {
	    printf("extension KVM_CHECK_EXTENSION failed = %d \n", errno);
	    goto out;
    }
    printf("KVM_CAP_NR_MEMSLOTS successful = %d\n", nr_slots);

    vmfd = ioctl(devfd, KVM_CREATE_VM, 0);
    if (vmfd < 0) {
	    printf("/devkvm open failed errno = %d \n", vmfd);
	    goto out;
    }
    printf("KVM_CREATE_VM successful = 0x%x\n", vmfd);

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
    close(vmfd);

out:
    close(devfd);
}
