#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0xb63f5c0e, "module_layout" },
	{ 0x1c42e2fb, "seq_release" },
	{ 0x3dbf649f, "seq_read" },
	{ 0x61184933, "seq_lseek" },
	{ 0xa90c928a, "param_ops_int" },
	{ 0xd9d1aa25, "queue_show_base_rbfgrad" },
	{ 0x5564892d, "seq_printf" },
	{ 0xa3063b57, "seq_open" },
	{ 0xd40601b4, "create_proc_entry" },
	{ 0xb7c3c6f3, "remove_proc_entry" },
	{ 0x1bbdda1e, "array_element_rbfgrad" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=sch_rbfgrad";


MODULE_INFO(srcversion, "E19A052D75474AED918727B");
