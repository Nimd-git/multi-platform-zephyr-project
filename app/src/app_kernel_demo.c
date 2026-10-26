// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：内核服务层
 *   任务调度 / 同步原语 / 消息通信 / 事件通知 / 系统时钟
 *
 * 双线程生产者-消费者：
 *   producer 线程：周期翻转 plat-led0（HAL 统一封装），通过消息队列发送计数，
 *                  并用 k_event 通知消费者；
 *   consumer 线程：等待事件后从消息队列取数并记录；
 *   k_timer        ：周期计时，演示定时器中断回调；
 *   k_work         ：系统工作队列周期任务，演示延迟工作项。
 *
 * 同一份代码在 Cortex-M3（真 stellaris GPIO）与 RV64（gpio_emul）上运行。
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include "app_kernel_demo.h"
#include <platform/plat_gpio.h>
#include <platform/plat_health.h>

#if defined(CONFIG_GPIO) && DT_NODE_HAS_STATUS(DT_ALIAS(plat_led0), okay)
#include <zephyr/drivers/gpio.h>
#define HAS_PLAT_LED 1
static const struct gpio_dt_spec plat_led =
	GPIO_DT_SPEC_GET(DT_ALIAS(plat_led0), gpios);
#else
#define HAS_PLAT_LED 0
#endif

LOG_MODULE_REGISTER(kernel_demo);

#define PRODUCER_PERIOD_MS 1000U
#define WORK_PERIOD_MS     2000U
#define TIMER_PERIOD_MS    500U
#define MSGQ_ITEMS         8U
#define THREAD_STACK_SIZE  4096
#define PRODUCER_PRIO      5
#define CONSUMER_PRIO      6

#define EV_DATA_READY BIT(0)

struct demo_msg {
	uint32_t sequence;
	uint32_t led_state;
};

K_MSGQ_DEFINE(demo_msgq, sizeof(struct demo_msg), MSGQ_ITEMS, 4);

static struct k_event demo_event;
static struct k_timer demo_timer;
static struct k_work_delayable demo_work;
static atomic_t timer_ticks;
static atomic_t work_runs;

K_THREAD_STACK_DEFINE(producer_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(consumer_stack, THREAD_STACK_SIZE);
static struct k_thread producer_thread;
static struct k_thread consumer_thread;

static void timer_expiry_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	atomic_inc(&timer_ticks);
}

static void work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	atomic_inc(&work_runs);
	LOG_INF("[work] alive, runs=%u timer_ticks=%u",
		(uint32_t)atomic_get(&work_runs),
		(uint32_t)atomic_get(&timer_ticks));
	(void)k_work_reschedule(&demo_work, K_MSEC(WORK_PERIOD_MS));
}

static void producer_entry(void *p1, void *p2, void *p3)
{
	static uint32_t sequence;
	struct demo_msg msg;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

#if HAS_PLAT_LED
	int ret = plat_gpio_output_init(&plat_led);

	if (ret != 0) {
		LOG_ERR("plat-led0 init failed: %d", ret);
		plat_health_report_error("led", ret);
	}
#endif

	while (1) {
		k_msleep(PRODUCER_PERIOD_MS);

#if HAS_PLAT_LED
		int led = plat_led_toggle(&plat_led);

		if (led < 0) {
			plat_health_report_error("led", led);
			led = 0;
		}
		msg.led_state = (uint32_t)led;
#else
		msg.led_state = 0U;
#endif
		msg.sequence = sequence++;

		while (k_msgq_put(&demo_msgq, &msg, K_NO_WAIT) != 0) {
			/* 队列满：丢弃最旧一条，演示异常处理路径 */
			struct demo_msg dropped;

			(void)k_msgq_get(&demo_msgq, &dropped, K_NO_WAIT);
			plat_health_report_error("msgq", -ENOSPC);
		}
		k_event_set(&demo_event, EV_DATA_READY);
	}
}

static void consumer_entry(void *p1, void *p2, void *p3)
{
	struct demo_msg msg;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		(void)k_event_wait(&demo_event, EV_DATA_READY, true, K_FOREVER);

		while (k_msgq_get(&demo_msgq, &msg, K_NO_WAIT) == 0) {
			LOG_INF("[consumer] seq=%u led=%u (qslots=%u)",
				msg.sequence, msg.led_state,
				(uint32_t)k_msgq_num_free_get(&demo_msgq));
		}
	}
}

void app_kernel_demo_start(void)
{
	k_event_init(&demo_event);
	k_timer_init(&demo_timer, timer_expiry_handler, NULL);
	k_work_init_delayable(&demo_work, work_handler);

	k_timer_start(&demo_timer, K_MSEC(TIMER_PERIOD_MS),
		      K_MSEC(TIMER_PERIOD_MS));
	(void)k_work_schedule(&demo_work, K_MSEC(WORK_PERIOD_MS));

	k_thread_create(&producer_thread, producer_stack,
			K_THREAD_STACK_SIZEOF(producer_stack),
			producer_entry, NULL, NULL, NULL,
			PRODUCER_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&producer_thread, "producer");

	k_thread_create(&consumer_thread, consumer_stack,
			K_THREAD_STACK_SIZEOF(consumer_stack),
			consumer_entry, NULL, NULL, NULL,
			CONSUMER_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&consumer_thread, "consumer");

	LOG_INF("[kernel-demo] threads/timer/work started");
}
