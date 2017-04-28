#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct greedy_data {
   struct list_head lower_queue;
   struct list_head upper_queue;
   sector_t disk_head;
};

static void greedy_merged_requests(struct request_queue *q, struct request *rq, struct request *next) {
   list_del_init(&next->queuelist);
}

static int greedy_dispatch(struct request_queue *q, int force) {
   struct greedy_data *gd = q->elevator->elevator_data;
   
   struct list_head* target;
   struct list_head* next_lower = gd->lower_queue.prev;
   struct list_head* next_upper = gd->upper_queue.next;
   
   bool is_lower_empty = (next_lower == &gd->lower_queue);
   bool is_upper_empty = (next_upper == &gd->upper_queue); 

   // Determine what the target dispatch request is, if any.
   if(is_lower_empty && is_upper_empty) return 0;
   if(is_lower_empty) target = next_upper;
   else if(is_upper_empty) target = next_lower;
   else {
      unsigned long lower_distance = gd->disk_head - blk_rq_pos(list_entry_rq(next_lower));
      unsigned long upper_distance = blk_rq_pos(list_entry_rq(next_upper)) - gd->disk_head;
      if(lower_distance < upper_distance) target = next_lower;
      else                                target = next_upper;
   }

   gd->disk_head = rq_end_sector(list_entry_rq(target));
   list_del_init(target);
   elv_dispatch_add_tail(q, list_entry_rq(target));
   return 1;
}

static void greedy_add_request(struct request_queue* q, struct request* rq) {
   struct greedy_data* gd = q->elevator->elevator_data;
   struct list_head* target = (blk_rq_pos(rq) < gd->disk_head ? &gd->lower_queue : &gd->upper_queue);
   
   struct request* pos;
   list_for_each_entry_reverse(pos, target, queuelist)
      if(blk_rq_pos(rq) > blk_rq_pos(pos)) break;
   list_add(&rq->queuelist, &pos->queuelist);
}

static struct request* greedy_former_request(struct request_queue* q, struct request* rq) {
   struct greedy_data* gd = q->elevator->elevator_data;

   if (rq->queuelist.prev == &gd->lower_queue || rq->queuelist.prev == &gd->upper_queue)
      return NULL;
   return list_entry_rq(rq->queuelist.prev);
}

static struct request* greedy_latter_request(struct request_queue* q, struct request* rq) {
   struct greedy_data* gd = q->elevator->elevator_data;

   if (rq->queuelist.next == &gd->lower_queue || rq->queuelist.next == &gd->upper_queue)
      return NULL;
   return list_entry_rq(rq->queuelist.next);
}

static int greedy_init_queue(struct request_queue *q, struct elevator_type *e) {
   struct greedy_data *gd;
   struct elevator_queue *eq;

   eq = elevator_alloc(q, e);
   if(!eq) return -ENOMEM;

   gd = kmalloc_node(sizeof(*gd), GFP_KERNEL, q->node);
   if (!gd) {
      kobject_put(&eq->kobj);
      return -ENOMEM;
   }
   eq->elevator_data = gd;

   INIT_LIST_HEAD(&gd->lower_queue);
   INIT_LIST_HEAD(&gd->upper_queue);
   gd->disk_head = 0ul;

   spin_lock_irq(q->queue_lock);
   q->elevator = eq;
   spin_unlock_irq(q->queue_lock);
   return 0;
}

static void greedy_exit_queue(struct elevator_queue *e) {
   struct greedy_data *gd = e->elevator_data;

   BUG_ON(!list_empty(&gd->lower_queue));
   BUG_ON(!list_empty(&gd->upper_queue));
   kfree(gd);
}

static struct elevator_type elevator_greedy = {
   .ops = {
      .elevator_merge_req_fn  = greedy_merged_requests,
      .elevator_dispatch_fn   = greedy_dispatch,
      .elevator_add_req_fn    = greedy_add_request,
      .elevator_former_req_fn = greedy_former_request,
      .elevator_latter_req_fn = greedy_latter_request,
      .elevator_init_fn       = greedy_init_queue,
      .elevator_exit_fn       = greedy_exit_queue,
   },
   .elevator_name = "greedy",
   .elevator_owner = THIS_MODULE,
};

static int __init greedy_init(void) {
   return elv_register(&elevator_greedy);
}

static void __exit greedy_exit(void) {
   elv_unregister(&elevator_greedy);
}

module_init(greedy_init);
module_exit(greedy_exit);

MODULE_AUTHOR("Team eXtreme");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Greedy IO scheduler");
