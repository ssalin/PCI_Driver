#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xef025c67, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xb6b46a7c, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x7f20c786, __VMLINUX_SYMBOL_STR(pci_unregister_driver) },
	{ 0xa62f13c9, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xaefd4a17, __VMLINUX_SYMBOL_STR(__pci_register_driver) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x3d582493, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xe7770a43, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x42c8de35, __VMLINUX_SYMBOL_STR(ioremap_nocache) },
	{ 0x5fe56825, __VMLINUX_SYMBOL_STR(dev_set_drvdata) },
	{ 0x94745f80, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x582bcf5, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x4a80e1f0, __VMLINUX_SYMBOL_STR(pci_set_master) },
	{ 0xbd8f9155, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x6f178d97, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x59a1608b, __VMLINUX_SYMBOL_STR(pci_request_selected_regions) },
	{ 0x9e52959c, __VMLINUX_SYMBOL_STR(dma_set_mask) },
	{ 0x51f59549, __VMLINUX_SYMBOL_STR(pci_enable_device_mem) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xf774db66, __VMLINUX_SYMBOL_STR(pci_disable_device) },
	{ 0xa428e106, __VMLINUX_SYMBOL_STR(pci_release_selected_regions) },
	{ 0x45ff592d, __VMLINUX_SYMBOL_STR(pci_select_bars) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xedc03953, __VMLINUX_SYMBOL_STR(iounmap) },
	{ 0xb749ae89, __VMLINUX_SYMBOL_STR(dev_get_drvdata) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "7CBE82CE32A2443B94D4698");
