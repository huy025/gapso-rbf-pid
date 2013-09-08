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
	{ 0x7f53f337, "fifo_create_dflt" },
	{ 0xc910137b, "qdisc_reset" },
	{ 0xa3e4da51, "__qdisc_calculate_pkt_len" },
	{ 0x87a45ee9, "_raw_spin_lock_bh" },
	{ 0x20ab909, "register_qdisc" },
	{ 0x46917606, "kthread_create_on_node" },
	{ 0x25d4d744, "skb_trim" },
	{ 0x2a04c897, "gnet_stats_copy_app" },
	{ 0xb86e4ab9, "random32" },
	{ 0x50eedeb8, "printk" },
	{ 0x534c513c, "kthread_stop" },
	{ 0xb4390f9a, "mcount" },
	{ 0x47b08a39, "nla_put" },
	{ 0x9f147a05, "qdisc_tree_decrease_qlen" },
	{ 0x407e9d24, "noop_qdisc" },
	{ 0x9c9c472e, "unregister_qdisc" },
	{ 0x9d4690ac, "bfifo_qdisc_ops" },
	{ 0x6223cafb, "_raw_spin_unlock_bh" },
	{ 0x4f391d0e, "nla_parse" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0xc4e74a76, "kfree_skb" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x8acc9d46, "wake_up_process" },
	{ 0x6dce0b90, "qdisc_destroy" },
	{ 0x7ecb001b, "__per_cpu_offset" },
	{ 0xd2965f6f, "kthread_should_stop" },
	{ 0x85670f1d, "rtnl_is_locked" },
	{ 0x92d5279b, "__init_rwsem" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "C288E0201C64FAE36C89385");
