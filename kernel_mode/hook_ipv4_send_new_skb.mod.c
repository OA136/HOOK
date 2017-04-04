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
	{ 0x96cec1da, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xc4369a4, __VMLINUX_SYMBOL_STR(nf_unregister_hooks) },
	{ 0x941ea991, __VMLINUX_SYMBOL_STR(nf_register_hooks) },
	{ 0xc936e567, __VMLINUX_SYMBOL_STR(kfree_skb) },
	{ 0x8762ebcc, __VMLINUX_SYMBOL_STR(dev_queue_xmit_sk) },
	{ 0x32d3f498, __VMLINUX_SYMBOL_STR(skb_push) },
	{ 0xd018433e, __VMLINUX_SYMBOL_STR(skb_checksum) },
	{ 0xc4d0ec1e, __VMLINUX_SYMBOL_STR(skb_put) },
	{ 0x461e6c85, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0xcf9d4c0c, __VMLINUX_SYMBOL_STR(__nf_nat_mangle_tcp_packet) },
	{ 0x765b5a4a, __VMLINUX_SYMBOL_STR(__nf_ct_ext_add_length) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x1e6d26a8, __VMLINUX_SYMBOL_STR(strstr) },
	{ 0x449ad0a7, __VMLINUX_SYMBOL_STR(memcmp) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xba7e1125, __VMLINUX_SYMBOL_STR(__pskb_pull_tail) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=nf_nat,nf_conntrack";


MODULE_INFO(srcversion, "D0F0EDC2BD6CA151D643C86");
