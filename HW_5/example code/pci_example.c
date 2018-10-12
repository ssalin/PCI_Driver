#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/pci.h>


#define PCI_DEVICE_E2000F 0x8765
#define PE_LED_MASK 0xC
#define PE_REG_LEDS 0x0052

static char *pe_driver_name = "Realtek_poker";

static const struct pci_device_id pe_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_E2000F), 0, 0, 0 },
	/* more device ids can be listed here */

	/* required last entry */
	{0, }
};
MODULE_DEVICE_TABLE(pci, pe_pci_tbl);
MODULE_LICENSE("Dual BSD/GPL");

static int new_leds;
module_param(new_leds, int, 0);

struct pes {
	struct pci_dev *pdev;
	void *hw_addr;
};

static int __devinit pe_probe(struct pci_dev *pdev,
			      const struct pci_device_id *ent)
{
	struct pes *pe;
	u32 ioremap_len;
	u32 led_reg;
	int err;

	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	/* set up for high or low dma */
	err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
		goto err_dma;
	}

	/* set up pci connections */
	err = pci_request_selected_regions(pdev, pci_select_bars(pdev,
					   IORESOURCE_MEM), pe_driver_name);
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

	/* map device memory */
	ioremap_len = min_t(int, pci_resource_len(pdev, 0), 1024);
	pe->hw_addr = ioremap(pci_resource_start(pdev, 0), ioremap_len);
	if (!pe->hw_addr) {
		err = -EIO;
		dev_info(&pdev->dev, "ioremap(0x%04x, 0x%04x) failed: 0x%x\n",
			 (unsigned int)pci_resource_start(pdev, 0),
			 (unsigned int)pci_resource_len(pdev, 0), err);
		goto err_ioremap;
	}

	led_reg = readl(pe->hw_addr + PE_REG_LEDS);
	dev_info(&pdev->dev, "led_reg = 0x%02x\n", led_reg);

	if (new_leds) {
		led_reg = (led_reg & ~PE_REG_LEDS) | (new_leds & PE_LED_MASK);
		writeb(led_reg, (pe->hw_addr + PE_REG_LEDS));
		dev_info(&pdev->dev, "new led_reg = 0x%02x\n", led_reg);
	}

	return 0;

err_ioremap:
	kfree(pe);
err_pe_alloc:
	pci_release_selected_regions(pdev,
				     pci_select_bars(pdev, IORESOURCE_MEM));
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}

static void __devexit pe_remove(struct pci_dev *pdev)
{
	struct pes *pe = pci_get_drvdata(pdev);

	/* unmap device from memory */
	iounmap(pe->hw_addr);

	/* free any allocated memory */
	kfree(pe);

	pci_release_selected_regions(pdev,
				     pci_select_bars(pdev, IORESOURCE_MEM));
	pci_disable_device(pdev);
}

static struct pci_driver pe_driver = {
	.name     = "pci_example",
	.id_table = pe_pci_tbl,
	.probe    = pe_probe,
	.remove   = pe_remove,
};

static int __init hello_init(void)
{
	int ret;
	printk(KERN_INFO "%s loaded\n", pe_driver.name);
	ret = pci_register_driver(&pe_driver);
	return ret;
}

static void __exit hello_exit(void)
{
	pci_unregister_driver(&pe_driver);
	printk(KERN_INFO "%s unloaded\n", pe_driver.name);
}

module_init(hello_init);
module_exit(hello_exit);

