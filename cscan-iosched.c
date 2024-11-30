/*
 * C-SCAN IO Scheduler
 * Linux Kernel v4.13.9
 * 
 * baseado em noop-iosched.c, por Jens Axboe.
 */

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/div64.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/jiffies.h>
#include <linux/types.h>


/* estruturas de dados para o C-SCAN. */
struct cscan_data_s {
	struct list_head queue; // lista de requisições
	int pointer;
	struct timer_list timer; // timer para despachar requisições
};

struct disk_data_s {
	int head_last_pos;	// ultima posição da cabeça
	int head_pos;		// posição atual da cabeça
	char head_dir;		// direçao de acesso 
};

// PARÂMETROS
static int queue_size = 10;
static int wait_time = 100;
static bool debug = false;

module_param(queue_size, int, 0644);
MODULE_PARM_DESC(queue_size, "Tamanho da fila de requisições");
module_param(wait_time, int, 0644);
MODULE_PARM_DESC(wait_time, "Tempo de espera entre requisições");
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Modo de depuração");

struct disk_data_s disk_data;

/* essa função está igual ao NOOP, realiza merges de blocos adjacentes */
static void cscan_merged_requests(struct request_queue *q, struct request *rq, struct request *next)
{
	list_del_init(&next->queuelist);
}

/* Esta função despacha o próximo bloco a ser lido. */
static int cscan_dispatch(struct request_queue *q, int force)
{
	// printk(KERN_ALERT "[C-SCAN] DISPATCH\n");
    struct cscan_data_s *nd = q->elevator->elevator_data;
    struct request *rq = NULL;
    struct request *best = NULL;
    struct list_head *pos;
	uint64_t time;

    if (list_empty(&nd->queue)) {
        if (debug)
            printk(KERN_ALERT "[C-SCAN] dsp -1 (fila vazia)\n");
        return 0;
    }

    // Percorre a fila para encontrar a próxima requisição no sentido atual
    list_for_each(pos, &nd->queue) {
        struct request *tmp = list_entry(pos, struct request, queuelist);
        if (blk_rq_pos(tmp) >= nd->pointer) {
            best = tmp;
            break;
        }
    }

    // Se não encontrou, reinicia a busca no início da lista (comportamento circular)
    if (!best) {
		if(debug) 
			printk(KERN_EMERG "[C-SCAN] dsp -1 (não encontrou - voltando ao começo)\n");
        best = list_first_entry(&nd->queue, struct request, queuelist);
    }

    // Despacha a requisição encontrada
    rq = best;
    list_del_init(&rq->queuelist);
    elv_dispatch_sort(q, rq);
    nd->pointer = blk_rq_pos(rq); // Atualiza a posição do header

	char direction = rq_data_dir(rq) == READ ? 'R' : 'W';
	time = ktime_get_ns();
	do_div(time, 1000000);
	if(debug)
		printk(KERN_ALERT "[C-SCAN] dsp %c %llu (%llu ms)\n", direction, blk_rq_pos(rq), time);

    return 1;
}

/* Esta função adiciona uma requisição ao disco em uma fila */
static void cscan_add_request(struct request_queue *q, struct request *rq)
{
	struct cscan_data_s *nd = q->elevator->elevator_data;
	struct list_head *pos;
	char direction;
	uint64_t time;

	// Adiciona a requisição na fila na posicao correta
	list_for_each(pos, &nd->queue) {
		struct request *tmp = list_entry(pos, struct request, queuelist);
		if (blk_rq_pos(rq) < blk_rq_pos(tmp)) {
			list_add_tail(&rq->queuelist, pos);
			direction = rq_data_dir(rq) == READ ? 'R' : 'W';
			time = ktime_get_ns();
			do_div(time, 1000000);
			if(debug)
				printk(KERN_ALERT "[C-SCAN] add %c %llu (%llu ms)\n", direction, blk_rq_pos(rq), time);
			return;
		}
	}

	// Se não encontrou posição, adiiociona no final da fila
	list_add_tail(&rq->queuelist, &nd->queue);
	direction = rq_data_dir(rq) == READ ? 'R' : 'W';
	time = ktime_get_ns();
	do_div(time, 1000000);
	if(debug)
		printk(KERN_ALERT "[C-SCAN] add %c %llu (%llu ms)\n", direction, blk_rq_pos(rq), time);
}

/* Callback do timer */
static void cscan_timer_fn(unsigned long data)
{
    struct cscan_data_s *nd = (struct cscan_data_s *)data;
    
	if(!nd || !nd->timer.data) {
		printk(KERN_ALERT "[C-SCAN] timer_fn -1 (timer com problema)\n");
		return;
	}

    if (!list_empty(&nd->queue)) {
		struct request_queue *q = (struct request_queue *)nd->timer.data;
        cscan_dispatch(q, 0);
    }

	mod_timer(&nd->timer, jiffies + msecs_to_jiffies(wait_time));
}

/* Esta função inicializa as estruturas de dados necessárias para o escalonador */
static int cscan_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct cscan_data_s *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	// Inicializa o timer
    init_timer(&nd->timer);
	nd->timer.function = cscan_timer_fn;
	nd->timer.data = (unsigned long)q;
	mod_timer(&nd->timer, jiffies + msecs_to_jiffies(wait_time));

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);

	return 0;
}

static void cscan_exit_queue(struct elevator_queue *e)
{
	struct cscan_data_s *nd = e->elevator_data;

	/* Implementação da finalização da fila (queue).
	 * Use como exemplo o driver noop-iosched.c
	 */

	// Desativa o timer
	del_timer_sync(&nd->timer);

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_cscan = {
	.ops.sq = {
		.elevator_merge_req_fn		= cscan_merged_requests,
		.elevator_dispatch_fn		= cscan_dispatch,
		.elevator_add_req_fn		= cscan_add_request,
		.elevator_init_fn		= cscan_init_queue,
		.elevator_exit_fn		= cscan_exit_queue,
	},
	.elevator_name = "c-scan",
	.elevator_owner = THIS_MODULE,
};

static int cscan_init(void)
{
	struct disk_data_s *disk = &disk_data;
	
	disk->head_last_pos = -1;
	disk->head_pos = -1;
	disk->head_dir = 'P';
	printk(KERN_ALERT "C-SCAN driver init\n");
	
	return elv_register(&elevator_cscan);
}

/* Finalização do driver. */
static void cscan_exit(void)
{
	printk(KERN_ALERT "C-SCAN driver exit\n");
	
	elv_unregister(&elevator_cscan);
}

module_init(cscan_init);
module_exit(cscan_exit);

MODULE_AUTHOR("Ingrid Carolina, Bianca Zuchinali, Vinicius Mibielli");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("C-SCAN IO scheduler skeleton");