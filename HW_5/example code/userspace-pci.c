#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pci/pci.h>
#include <stdint.h>
#include <linux/types.h>

#define MEM_WINDOW_SZ  0x0200000

static struct pci_filter filter;    /* Device filter */

static struct option opts[] = {
	{"address", 1, NULL, 'a' },
	{"debug", 0, NULL, 'D' },
	{"device", 1, NULL, 'd' },
	{"slot", 1, NULL, 's' },
	{ 0, 0, NULL, '0' }
};

static void usage(char *progname, char *idfile)
{
	printf(	"Usage: %s [options] [device]   (%s)\n\n"
		"Options:\n"
		"-D\t\tPCI debugging\n"
		"-o\t\tRegister offset\n"
		"Device:\n"
		"-d [<vendor>]:[<device>]\t\t\tShow selected devices\n"
		"-s [[[[<domain>]:]<bus>]:][<slot>][.[<func>]]"
			"\tShow devices in selected slots\n\n",
		progname, idfile);
}

static int print_register(struct pci_dev *dev, u32 offset)
{
	volatile void *mem;
	int dev_mem_fd;

	dev_mem_fd = open("/dev/mem", O_RDONLY);
	if (dev_mem_fd < 0) {
		perror("open");
		return -1;
	}

	mem = mmap(NULL, MEM_WINDOW_SZ, PROT_READ, MAP_SHARED, dev_mem_fd,
			(dev->base_addr[0] & PCI_ADDR_MEM_MASK));
	if (mem == MAP_FAILED) {
		perror("mmap/readable - try rebooting with iomem=relaxed");
		close(dev_mem_fd);
		return -1;
	}

	printf("0x%x\n", *((u32 *)(mem + offset)));

	close(dev_mem_fd);
	munmap((void *)mem, MEM_WINDOW_SZ);

	return 0;
}

int main(int argc, char **argv)
{
	int ch;
	struct pci_access *pacc;
	struct pci_dev *dev;
	char *errmsg;
	char buf[128];
	u32 offset = 0;
	u32 value = 0;
	u64 lvalue = 0;
	int device_specified = 0;
	int got_offset = 0;
	int ret;

	if (getuid() != 0) {
		printf("%s: must be run as root\n", argv[0]);
		exit(1);
	}

	pacc = pci_alloc();		/* Get the pci_access structure */
	if (pacc == NULL) {
		perror("pci_alloc");
		exit(1);
	}
	pci_filter_init(pacc, &filter);

	while ((ch = getopt_long(argc, argv, "o:d:s:", opts, NULL)) != -1) {
		switch (ch) {
		case 'D':
			pacc->debugging++;
			break;
		case 'o':
			offset = strtol(optarg, NULL, 0);
			got_offset++;
			break;
		case 'd':
			/* Show only selected devices */
			if ((errmsg = pci_filter_parse_id(&filter, optarg))) {
				printf("%s\n", errmsg);
				exit(1);
			}
			device_specified++;
			break;
		case 's':
			/* Show only devices in selected slots */
			if ((errmsg = pci_filter_parse_slot(&filter, optarg))) {
				printf("%s\n", errmsg);
				exit(1);
			}
			device_specified++;
			break;
		case '?':
		default:
			usage(argv[0], pacc->id_file_name);
			exit(1);
			break;
		}
	}

	if (!device_specified) {
		printf("No device given\n");
		usage(argv[0], pacc->id_file_name);
		exit(1);
	}

	if (!got_offset) {
		printf("No offset given\n");
		usage(argv[0], pacc->id_file_name);
		exit(1);
	}

	pci_init(pacc);			/* Initialize the PCI library */
	pci_scan_bus(pacc);		/* Get the list of devices */

	if (pacc->debugging)
		printf(	"filter: "
			"bus=0x%x slot=0x%x func=0x%x\n"
			"\tvendor=0x%x device=0x%x\n\n",
			filter.bus, filter.slot, filter.func,
			filter.vendor, filter.device);

	/* Iterate over all devices to find the single one we want */
	for (dev = pacc->devices; dev; dev = dev->next) {
		if (pci_filter_match(&filter, dev))
			break;
	}

	if (!dev) {
		printf("no device found\n");
		ret = -1;
		goto out;
	}

	/* Fill in header info we need */
	pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_SIZES);

	printf(	"%02x:%02x.%d (%04x:%04x)\n%s\n",
			dev->bus, dev->dev, dev->func, 
			dev->vendor_id, dev->device_id,
			pci_lookup_name(pacc, buf, sizeof(buf),
				PCI_LOOKUP_VENDOR|PCI_LOOKUP_DEVICE, 
				dev->vendor_id, dev->device_id, 0, 0));

	ret = print_register(dev, offset);

out:
	pci_cleanup(pacc);		/* Close everything */
	return ret;
}

