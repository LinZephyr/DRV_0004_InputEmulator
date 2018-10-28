/* 参考drivers\input\keyboard\gpio_keys.c */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>




static struct input_dev *input_emulator_dev;

static struct timer_list buttons_timer;
static struct pin_desc *irq_pd;

static int key_num = 0;

enum of_gpio_flags *flag;
struct device_node *np;


int irq;


static void buttons_timer_function(unsigned long data)
{
	unsigned int pinval;

	pinval = gpio_get_value(key_num);

	if (pinval)
	{
		/* 松开 : 最后一个参数: 0-松开, 1-按下 */
		/* 上报事件到处理(handler)层 */
		input_event(input_emulator_dev, EV_KEY, KEY_L, 0);
		
		/* 上报同步类事件，表示按键已经上报完成了 */
		input_sync(input_emulator_dev);
	}
	else
	{
		/* 按下 */
		input_event(input_emulator_dev, EV_KEY, KEY_L, 1);
		input_sync(input_emulator_dev);
	}
}


static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	/* 10ms后启动定时器处理函数 */
	mod_timer(&buttons_timer, jiffies+HZ/100);
	return IRQ_RETVAL(IRQ_HANDLED);
}


static const struct of_device_id gpio_input_of_match[] = {
        { .compatible = "gpio_input" },
        { }
};

static int gpio_input_probe(struct platform_device *pdev)
{
	int error;
	unsigned long irqflags;

	np = pdev->dev.of_node;

	

	/* 定时器初始化 */
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	add_timer(&buttons_timer);

	/* GPIO口初始化 */
	key_num = of_get_named_gpio_flags(np, "gpio_input_num5" , 0, flag);
	gpio_request(key_num, "key_num");
	gpio_direction_input(key_num);
	gpio_free(key_num);

	/* 中断初始化 */
	irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	irq = gpio_to_irq(key_num);
	error = request_irq(irq, buttons_irq, irqflags, "zf_key", NULL);
	if (error) {
		printk("Unable to register input device, error: %d\n",
			error);
	}
	
	return 0;
}



static int gpio_input_remove(struct platform_device *pdev)
{
	struct gpio_ctl_gpio *data = platform_get_drvdata(pdev);

	devm_kfree(&pdev->dev,NULL);
    return 0;
}


static struct platform_driver gpio_input_driver = {
        .probe = gpio_input_probe,
        .remove = gpio_input_remove,
        .driver = {
                .name           = "gpio_input",
                .of_match_table = of_match_ptr(gpio_input_of_match),
        },
};



static int input_emulator_init(void)
{
	int i;
	int error;



	platform_driver_register(&gpio_input_driver);


	
	/* 1. 分配一个input_dev结构体 */
	input_emulator_dev = input_allocate_device();

	/* 2. 设置 */
	/* 2.1 能产生哪类事件 */
	set_bit(EV_KEY, input_emulator_dev->evbit);
//	set_bit(EV_REP, input_emulator_dev->evbit);
	 
	/* 2.2 能产生所有的按键 */
	for (i = 0; i < BITS_TO_LONGS(KEY_CNT); i++)
		input_emulator_dev->keybit[i] = ~0UL;

	/* 2.3 为android构造一些设备信息 */
	input_emulator_dev->name = "zf_input";
	input_emulator_dev->id.bustype = 1;
	input_emulator_dev->id.vendor  = 0x1234;
	input_emulator_dev->id.product = 0x5678;
	input_emulator_dev->id.version = 1;

	/* 3. 注册 */
	error = input_register_device(input_emulator_dev);
	if (error) {
		printk("Unable to register input device, error: %d\n",
			error);
	}
	
	return 0;
}

static void input_emulator_exit(void)
{
	//platform_driver_unregister(&gpio_input_driver);

	free_irq(irq, NULL);
	del_timer(&buttons_timer);
	input_unregister_device(input_emulator_dev);
	input_free_device(input_emulator_dev);	
	
	platform_driver_unregister(&gpio_input_driver);
}

module_init(input_emulator_init);

module_exit(input_emulator_exit);

MODULE_LICENSE("GPL");

