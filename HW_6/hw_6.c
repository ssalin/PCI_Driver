/*
 * Sam Salin
 * 4/11/16
 * ECE 373
 *
 * HW 6: blink lights on packet reciept
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
#include <linux/dma-mapping.h> 
#define GET_DESC(R, i) (&(((struct rx_desc *)((R).desc))[i]))//shamelessly copied from lxr e1000.h
#define NUMDESC 16 //number of descriptors in ring		     line 459, modified for struct type
#define DEVCNT 1
#define DEVNAME "hw_4_driver"
#define LEDCTL 0x00e00
#define RECCTL 0x00100
#define RDBAL  0x02800 //receive base address low
#define RDBAH  0x02804 //receive base address high
#define RDH	   0x02810
#define RDT	   0x02818
#define CTLEXT 0x00018
#define IMS    0x000D0
#define IMCR   0x000D8 //IMC

static struct mydev_dev {
	struct cdev cdev;
	/* more stuff will go in here later... */
} mydev;

static dev_t mydev_node;
static char *pe_driver_name = "packet_blinker";
static DEFINE_PCI_DEVICE_TABLE(pci_tbl) = {
	{PCI_DEVICE(0x8086, 0x150c) }, /*numbers from lspci, lspci -n. could use 0x1501 instead of 0x150c*/
	{}, /*needed for some reason*/
};

/*PCI STUFF*/
struct pes{
	struct pci_dev *pdev;
	void *hw_addr; //add offsets to get places
	struct work_struct queue;
};

//static int b_r = 2;
//module_param(b_r, int, S_IRUSR | S_IWUSR);

static struct pes *usr;

/*Descriptors*/
struct rx_desc{
        __le64 buffer_addr;
        __le16 length;
        __le16 csum;
        u8 status;
        u8 errors;
        __le16 special;
}*rsd;

// descriptor ring 
//could be done with struct rx_desc ring[16]?
 struct ring {//pretty heavily based on E1000.h		
	void *desc;	
	dma_addr_t dma;
	unsigned int size;
	unsigned int count;
	struct buffer *buf;
        void  *head;
	void  *tail;
}*rx_ring;

//BUFFER
struct buffer {
	void *buf;
	dma_addr_t dma; 
};
/*
 buffer_info->buf = databuf;
 buffer_info->length = len;
 buffer_info->dma = dma_map_single(mydev, databuf, len, DMA_FROM_DEVICE)
 rx_desc = 
 
*/ 
/*workque hdlr*/
static void wrkq_hdlr(struct work_struct *work)
{
 struct pes * adptr = container_of(work, struct pes, queue);
 u32 tmp, tmp2;
 printk(KERN_INFO "ENTERED WRKQ HDLR\n");
 mdelay (500); //sleep .5 sec
 //turn off LEDs
 tmp = readl(adptr->hw_addr + LEDCTL); //read it
 tmp = tmp & 0x000000; //clear
 tmp = tmp | 0x0f0f0f ;//turn lights off
 writel(tmp, adptr->hw_addr + LEDCTL); //store it
// do{
// }while(--count > 0);
 //bump tail
 tmp  = readl(rx_ring->tail);
 tmp2 = readl(rx_ring->head);
 printk(KERN_INFO"TAIL = %d, HEAD = %d\n",tmp,tmp2);
 
 if (tmp == 15)//last entry
	tmp = 0;
 else//not last entry
	++tmp;
 writel(tmp, rx_ring->tail);
}

/* interrupt handler */
static irqreturn_t irq_hdlr (int irq, void *data)
{
 unsigned int tmp;
 u32 led_reg;
 printk(KERN_INFO "ENTERED IRQ HDLR\n");
 tmp = readl(usr->hw_addr + 0x000C0); //irq cause reg
 if(tmp){/*our IRQ*/
 writel(0x0, usr->hw_addr+ IMCR);//clear interrupts
 /* turn on green LEDs  */
 led_reg = readl(usr->hw_addr + LEDCTL);
 led_reg = led_reg & 0x000000;
 led_reg  = led_reg | 0x0E0E00; 
 writel(led_reg, usr->hw_addr + LEDCTL);
 //workqueue thread call
 schedule_work(&(usr->queue));
 return IRQ_HANDLED;
 }
	
 /*not our IRQ, probably shouldnt happen*/
 printk(KERN_ERR"NOT OUR IRQ\n");
 tmp = 0x100000;
 writel(tmp, usr->hw_addr + IMS); //reenable IRQ
 return	IRQ_NONE;	
}

static int pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {	
 struct pes * pe;
 u32 ioremap_len;
 u32 led_reg;
 void * addr;
 unsigned int tmp;
 u64 tmp1;
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
 ioremap_len = min_t(int, pci_resource_len(pdev, 0), 131072);//2^17
 pe->hw_addr = ioremap(pci_resource_start(pdev, 0), ioremap_len);
 if (!pe->hw_addr) {
	err = -EIO;
	dev_info(&pdev->dev, "ioremap(0x%04x, 0x%04x) failed: 0x%x\n",
		(unsigned int)pci_resource_start(pdev, 0),
		(unsigned int)pci_resource_len(pdev, 0), err);
	goto err_ioremap;
 }
 
 //initailze device
 addr = pe->hw_addr; //fewer derefrences
 writel(0xFFFFFFFF, addr + IMCR); //disable IRQ
 writel(0x84000000, addr); //reset
 udelay(1); //manual said to wait 1 us after resetting before reads or writes p. 199
 writel(0xFFFFFFF, addr + IMCR); //disable IRQ again (4.6.1 datasheet)
 //clear LEDs from reset
 led_reg = readl(addr + LEDCTL);
 led_reg = led_reg & 0x0f0f0f;//set all LEDs to OFF, set invert bits 
 writel (led_reg,addr + LEDCTL);
 /* PHY */
 writel(0x400000, addr + 0x05B00); //set bit 22 GCR
 writel(0x1, addr + 0x05B64);  // set bit 0 GCR2
 writel(0x1831AF08, addr + 0x00020);  // write to MDIC
 //promiscuous mode
 tmp = readl(addr + RECCTL); 
 tmp = tmp | 0x800C; // set bit 3,4 and 15  
 writel(tmp, addr + RECCTL);
 
 //set up ring
 rx_ring = kmalloc (sizeof (struct ring), GFP_KERNEL);
 if(!rx_ring)
	printk(KERN_INFO "RING FAIL");
 //set up descriptors
 rx_ring->count = 16;
 
 //round 4k (e1000e netdev 2383)
 rx_ring->size = rx_ring->count * sizeof(struct rx_desc);
 rx_ring->buf = vmalloc(rx_ring->size); 
 rx_ring->size = ALIGN(rx_ring->size, 4096);
 rx_ring->desc = dma_alloc_coherent(&pdev->dev, rx_ring->size, &rx_ring->dma, GFP_KERNEL);
 if (!rx_ring->desc) {
	err = -ENOMEM;
	return err;
 }
 printk(KERN_INFO"ABOUT TO ALLOCATE\n"); 
 //pin buf
 for (err  = 0; err < NUMDESC; ++err){//using err as a loop counter
 	rx_ring->buf[err].buf = kmalloc (2048, GFP_KERNEL);
 	rx_ring->buf[err].dma = dma_map_single(&pdev->dev, rx_ring->buf[err].buf, 2048, DMA_FROM_DEVICE);
	rsd = GET_DESC(*rx_ring, err);
	rsd->buffer_addr = cpu_to_le64(rx_ring->buf[err].dma);
        //i feel like there should be some kind of error checking going on
 }
 printk(KERN_INFO "ALLOCATED \n");
 //head/tail
 tmp1 = rx_ring->dma;
 tmp1 = tmp1 & 0x00000000FFFFFFFF; //clear upper 32
 writel(tmp1, addr + RDBAL); //base low
 tmp1 = tmp1 >> 32;
 writel(tmp1, addr + RDBAH);  //base high 
 writel(0x800, addr + 0x02808); //set buffer size RDLEN
 writel(0x1, addr + RDH); //head desc 0
 writel(0x0, addr + RDT); //tail desc 0
 rx_ring->head = addr + RDH; //map head
 rx_ring->tail = addr + RDT; //map tail
 //writel(0xF,addr + RDT);//set tail to desc 1-head //0x3f
 
 //set up workqueue
 INIT_WORK(&pe->queue, wrkq_hdlr);
 
 //set up IRQ
 err = request_irq(pdev->irq, irq_hdlr, IRQF_SHARED, "hw6_intr", addr);//last feild 0 device not shared
 if(err)
	printk(KERN_ERR "ERROR %d\n", err);
 
 //set up receive registers
 //tmp = readl(hw_base_addr + RECCTL);
 //tell to autonegotiate speed
 //tell to accept bad packets?
 /* setup link in CTRL */
 tmp = readl(addr);
 tmp = tmp | 0x40; /* set bit 6  */ 
 writel(tmp, addr);
	
 //set receive enable
 tmp = readl(addr + RECCTL);
 tmp = tmp | 0x2;  // set bit 1
 writel(tmp, addr + RECCTL);
	
 //sets the mask for receive queue 0 interrupt in IMS
 tmp = readl(addr + IMS);
 tmp = tmp | 0x10; //set RXDMT0 to 1
 writel(tmp, addr + IMS);
 
 tmp = readl(addr + CTLEXT); 
 tmp = tmp | 0x18000000; //tell device driver has loaded
 writel(tmp, addr + CTLEXT);
 writel(0x100000, addr + IMS); //set bit 20 

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
 //turn off interrupts
 free_irq(pdev->irq,usr->hw_addr);
 //cancel workqueue
 
 //free the ring
 kfree(rx_ring);		 
        
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


static int example5_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "successfully opened!\n");
	return 0;
}

static ssize_t example5_read(struct file *file, u16 __user *buf, size_t len, loff_t *offset) {
	/* Get a local kernel buffer set aside */
	u16  tmp;
        u16  ret=0;
	if (*offset >= sizeof(int))
		return 0;
	/* Make sure our user wasn't bad... */
	if (!buf) {
		ret = -EINVAL;
		goto out;
	}
	//pack head/tail location
	ret = readl(rx_ring->head);
        ret = ret << 8;
        tmp = readl(rx_ring-> tail);
        ret = ret | tmp;
	
	if (copy_to_user(buf, &ret, sizeof(u16))) {
		ret = -EFAULT;
		goto out;
	}

	ret = sizeof(int);
	*offset += len;
out:
	return ret;
}

static ssize_t example5_write(struct file *file, const int __user *buf, size_t len, loff_t *offset) {
	return 0; 
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
	return 0;
}

static void __exit example5_exit(void) {
	//kill the workqueue
        cancel_work_sync(&usr->queue);
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
