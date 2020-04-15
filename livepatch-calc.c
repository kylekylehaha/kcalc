#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/livepatch.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "expression.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Patch calc kernel module");
MODULE_VERSION("0.1");

void livepatch_nop_cleanup(struct expr_func *f, void *c)
{
    /* suppress compilation warnings */
    (void) f;
    (void) c;
}

int livepatch_nop(struct expr_func *f, vec_expr_t args, void *c)
{
    (void) args;
    (void) c;
    pr_err("function nop is now patched\n");
    return 0;
}

int livepatch_fib(int n)
{
    pr_err("livepatch_fib is now patched\n");
    int num = n >> 4;
    int frac = n & 15;
    if (frac >= 8) {
        printk("Error: Invalid number!");
        return -1;
    }

    for (int i = 0; i < frac; i++)
        num *= 10;

    if (num < 0)
        return -1;

    int clz = __builtin_clz(num);
    int digit = 32 - clz;
    num <<= clz;
    int fn = 0, fn1 = 1;
    for (int i = 0; i < digit; i++) {
        int f2n, f2n1;
        f2n1 = fn1 * fn1 + fn * fn;
        f2n = fn * 2 * fn1 - fn * fn;
        fn = f2n;
        fn1 = f2n1;
        if (num & 0x80000000) {
            fn = f2n1;
            fn1 = f2n + f2n1;
        }
        num <<= 1;
    }
    printk("expected answer is %d\n", fn);
    return fn << 4;
}

void livepatch_fib_cleanup(struct expr_func *f, void *c)
{
    /* suppress compilation warning */
    (void) f;
    (void) c;
}

/* clang-format off */
static struct klp_func funcs[] = {
    {
        .old_name = "user_func_nop",
        .new_func = livepatch_nop,
    },
    {
        .old_name = "user_func_nop_cleanup",
        .new_func = livepatch_nop_cleanup,
    },
    {
        .old_name = "fib_clz",
        .new_func = livepatch_fib,
    },
    {
        .old_name = "user_func_fib_cleanup",
        .new_func = livepatch_fib_cleanup,
    },
    {},
};
static struct klp_object objs[] = {
    {
        .name = "calc",
        .funcs = funcs,
    },
    {},
};
/* clang-format on */

static struct klp_patch patch = {
    .mod = THIS_MODULE,
    .objs = objs,
};

static int livepatch_calc_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
    return klp_enable_patch(&patch);
#else
    int ret = klp_register_patch(&patch);
    if (ret)
        return ret;
    ret = klp_enable_patch(&patch);
    if (ret) {
        WARN_ON(klp_unregister_patch(&patch));
        return ret;
    }
    return 0;
#endif
}

static void livepatch_calc_exit(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
    WARN_ON(klp_unregister_patch(&patch));
#endif
}

module_init(livepatch_calc_init);
module_exit(livepatch_calc_exit);
MODULE_INFO(livepatch, "Y");
