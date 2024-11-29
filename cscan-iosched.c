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


/* estruturas de dados para o C-SCAN. */
struct cscan_data_s {
	struct list_head queue; // lista de requisições
	
};

struct disk_data_s {
	int head_last_pos;	// ultima posição da cabeça
	int head_pos;		// posição atual da cabeça
	char head_dir;		// direçao de acesso ([P]arked, [L]eft, [R]ight)
};

// PARÂMETROS
static int queue_size = 10;
static int wait_time = 100;
static bool debug = 0;

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
	struct cscan_data_s *nd = q->elevator->elevator_data;
	char direction;
	struct request *rq;
	uint64_t time;

	if(list_empty(&nd->queue)){
		printk(KERN_ALERT "[C-SCAN] dsp -1\n");
		return 0;
	}



	// Código base
	rq = list_first_entry_or_null(&nd->queue, struct request, queuelist);
	if (rq) {
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		
		direction = rq_data_dir(rq) == READ ? 'R' : 'W';
		time = ktime_get_ns();
		do_div(time, 1000000);
		printk(KERN_ALERT "[C-SCAN] dsp %c %llu (%llu ms)\n", direction, blk_rq_pos(rq), time);

		return 1;
	}

	return 0;
}

/* Esta função adiciona uma requisição ao disco em uma fila */
static void cscan_add_request(struct request_queue *q, struct request *rq)
{
	// Ajustar essa função

	struct cscan_data_s *nd = q->elevator->elevator_data;
	//struct disk_data_s *disk = &disk_data;
	char direction;
	uint64_t time;

	/* Aqui deve-se adicionar uma requisição na fila do driver.
	 * Use como exemplo o driver noop-iosched.c
	 */
	list_add_tail(&rq->queuelist, &nd->queue);
	
	direction = rq_data_dir(rq) == READ ? 'R' : 'W';
	time = ktime_get_ns();
	do_div(time, 1000000);
	printk(KERN_ALERT "[C-SCAN] add %c %llu (%llu ms)\n", direction, blk_rq_pos(rq), time);
}

/* Esta função inicializa as estruturas de dados necessárias para o escalonador */
static int cscan_init_queue(struct request_queue *q, struct elevator_type *e)
{
	// Também implementar isso 

	struct cscan_data_s *nd;
	struct elevator_queue *eq;

	/* Implementação da inicialização da fila (queue).
	 * Use como exemplo a inicialização da fila no driver noop-iosched.c
	 */

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

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);

	return 0;
}

static void cscan_exit_queue(struct elevator_queue *e)
{
	// Implementar isso aqui
	struct cscan_data_s *nd = e->elevator_data;

	/* Implementação da finalização da fila (queue).
	 * Use como exemplo o driver noop-iosched.c
	 */
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
