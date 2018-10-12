/*
 * Sam Salin
 * 4/11/16
 * ECE 373
 *
 * HW 4: blink light with timer
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/time.h>

#define DEVCNT 1
#define DEVNAME "hw_4_driver"
#define LEDCTL 0x00e00
 
static struct mydev_dev {
	struct cdev cdev;
	/* more stuff will go in here later... */
} mydev;

static dev_t mydev_node;
static char *pe_driver_name = "lights";
static DEFINE_PCI_DEVICE_TABLE(pci_tbl) = {
	{PCI_DEVICE(0x8086, 0x150c) }, /*numbers from lspci, lspci -n. could use 0x1501 instead of 0x150c*/
	{}, /*needed for some reason*/
};


/*PCI STUFF*/
struct pes{
	struct pci_dev *pdev;
	void *hw_addr;
};

static int b_r = 2;
module_param(b_r, int, S_IRUSR | S_IWUSR);

static struct pes *usr; 
static int pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {	
 struct pes * pe;
 u32 ioremap_len;
 u32 led_reg;
 int err;
	
 err = pci_enable_device_mem(pdev);
 if(err)
	return err; /*nonzero implies error*/
 /*hi/lo DMA prep*/
 err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
 if (err) {
	dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
	goto err_dma;
 }

 /*connect pci*/
 err = pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM), pe_driver_name);
 if (err) {
	dev_info(&pdev->dev, "pci_request_selected_regions failed %d\n", err);
	goto err_pci_reg;
 } 

 pci_set_master(pdev);
 pe = kzalloc(sizeof(*pe), GFP_KERNEL);
 if (!pe) {
	err = -ENOMEM;
	goto err_pe_alloc;
 }
 
 pe->pdev = pdev;
 pci_set_drvdata(pdev, pe);
 
 /*map dev mem*/
 ioremap_len = min_t(int, pci_resource_len(pdev, 0), 1024);
 pe->hw_addr = ioremap(pci_resource_start(pdev, 0), ioremap_len);
 if (!pe->hw_addr) {
	err = -EIO;
	dev_info(&pdev->dev, "ioremap(0x%04x, 0x%04x) failed: 0x%x\n",
		(unsigned int)pci_resource_start(pdev, 0),
		(unsigned int)pci_resource_len(pdev, 0), err);
	goto err_ioremap;
 }

 led_reg = readl(pe->hw_addr + LEDCTL);
 led_reg = led_reg & 0xffffff00;
 led_reg = led_reg | 0x0000004f;/*0x40 to make light work maybe move 2 init*/ 
 writel (led_reg, pe->hw_addr + LEDCTL);
 dev_info(&pdev->dev, "led_reg = 0x%02x\n", led_reg);
 usr = pe;
 return 0;
 
err_ioremap:
	kfree(pe);
err_pe_alloc:
	pci_release_selected_regions(pdev,pci_select_bars(pdev, IORESOURCE_MEM));
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}

static void pci_remove(struct pci_dev *pdev) {
 struct pes *pe = pci_get_drvdata(pdev);

 /* unmap device from memory */
 iounmap(pe->hw_addr);

 /* free any allocated memory */
 kfree(pe);
	
 pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
 pci_disable_device(pdev);
}

static struct pci_driver pe_driver = {
	.name = "hw_3_pci",
	.id_table = pci_tbl,
	.probe = pci_probe,
	.remove = pci_remove,
};

/*TIMER stuff*/
struct timer_list example_timer;

static struct my_time_struct {
	int b_r; /*blink rate*/
	unsigned long jiff;
} my_str;

static void example_timer_cb(unsigned long data)
{
	struct my_time_struct *val = (struct my_time_struct *)data;
	unsigned int tmp, tmp2; /*for LED_CTL contents*/
	unsigned int clr = 0x0000000f; //clr all but low 4 bts
	/*grab LED_CTL*/
	val->b_r = b_r;
	tmp = readl(usr->hw_addr + LEDCTL);
	tmp2 = tmp;
	tmp = tmp & clr;
	if(tmp == 0xe){ /*light already on*/
		tmp2 = tmp2 & 0xffffff00;
		tmp2 = tmp2 | 0x0000000f;
	} 
	else{ /*2 possible LED states*/
		tmp2 = tmp2 & 0xffffff00;
		tmp2 = tmp2 | 0x0000004e;
	}
	tmp = tmp2;	
	writel(tmp, usr->hw_addr + LEDCTL); 
	if(!val->b_r)
		val->b_r=2; /*if 0 give default value*/
	/* restart timer */
	mod_timer(&example_timer, (jiffies + (1 * (HZ/val->b_r))));
}

static int example5_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "successfully opened!\n");
	return 0;
}

static ssize_t example5_read(struct file *file, int __user *buf, size_t len, loff_t *offset) {
	/* Get a local kernel buffer set aside */
	int ret;
	if (*offset >= sizeof(int))
		return 0;
	/* Make sure our user wasn't bad... */
	if (!buf) {
		ret = -EINVAL;
		goto out;
	}
	if (copy_to_user(buf, &my_str.b_r, sizeof(int))) {
		ret = -EFAULT;
		goto out;
	}
	ret = sizeof(int);
	*offset += len;
	/* Good to go, so printk the thingy */
	printk(KERN_INFO "User got from us %x\n", *buf);

out:
	return ret;
}

static ssize_t example5_write(struct file *file, const int __user *buf, size_t len, loff_t *offset) {
	int ret;
	/* Make sure our user isn't bad... */
	if (!buf || (buf < 0)) {
		ret = -EINVAL;
		goto out;
	} 

	/* Copy from the user-provided buffer */
	if (copy_from_user(&ret, buf, len)) {
		/* uh-oh... */
		ret = -EFAULT;
		goto out;
	}
	my_str.b_r = ret; /*set for timer_cb*/
	b_r=ret;
	/* print what userspace gave us */
	printk(KERN_INFO "Userspace wrote to us %x\n",ret);

out:
	return ret;
}

/* File operations for our device */
static struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.open = example5_open,
	.read = example5_read,
	.write = example5_write,
};

static int __init example5_init(void) {
	printk(KERN_INFO "hw_3_driver module loading...");

	if (alloc_chrdev_region(&mydev_node, 0, DEVCNT, DEVNAME)) {
		printk(KERN_ERR "alloc_chrdev_region() failed!\n");
		return -1;
	}

	printk(KERN_INFO "Allocated %d devices at major: %d\n", DEVCNT,
	       MAJOR(mydev_node));

	/* Initialize the character device and add it to the kernel */
	cdev_init(&mydev.cdev, &mydev_fops);
	mydev.cdev.owner = THIS_MODULE;
	
	if (cdev_add(&mydev.cdev, mydev_node, DEVCNT)) {
		printk(KERN_ERR "cdev_add() failed!\n");
		/* clean up chrdev allocation */
		unregister_chrdev_region(mydev_node, DEVCNT);
		return -1;
	}
	pci_register_driver(&pe_driver); /*for PCI*/
	if(!b_r)
		b_r=2;/*default value if 0 is entered*/
	my_str.b_r=b_r; /**/
	setup_timer(&example_timer, example_timer_cb, (unsigned long)&my_str);
	mod_timer(&example_timer, (jiffies + ((1/b_r) * HZ))); /*pretty impt*/
	
	return 0;
}

static void __exit example5_exit(void) {
	/*kill the timer*/
	del_timer_sync(&example_timer);
	/* destroy the cdev */
	cdev_del(&mydev.cdev);

	/* clean up the devices */
	pci_unregister_driver(&pe_driver);
	unregister_chrdev_region(mydev_node, DEVCNT);
	printk(KERN_INFO "hw_3_driver module unloaded!\n");
}

MODULE_AUTHOR("Sam Salin");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");

module_init(example5_init);
module_exit(example5_exit);
