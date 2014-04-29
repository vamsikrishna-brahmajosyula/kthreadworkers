/*
 *	kthreadwork.c
 *
 * Copyright (C) 2014  Vamsi Krishna B (vamsikrishna.brahmajosyula@gmail.com)
 * 
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.

 */



#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/string.h> 
#include <linux/kthread.h> 
#include <linux/wait.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vamsi");
MODULE_DESCRIPTION("Kernel Worker Thread assignment");

#define MAX_SEQ_NUM 100 

#define THREADS_MAX 5 

DECLARE_WAIT_QUEUE_HEAD(producer_q);

DECLARE_WAIT_QUEUE_HEAD(consumer_q);

int prod_kthids[THREADS_MAX];
int cons_kthids[THREADS_MAX];

struct task_struct * prod_threads[THREADS_MAX];
struct task_struct * cons_threads[THREADS_MAX];

typedef struct __wkth_struct
{
  int seq_no;
  char data[64];
  int p_turn;
  int c_turn;
  int data_consumed;
} wkth_struct;

wkth_struct wkth;

int prod_func(void * data)
{
  int * prod_id = (int *) data;

  while(wkth.seq_no < MAX_SEQ_NUM)
    {
     	

      /* Add to wait queue until turn comes */      
      wait_event_interruptible(producer_q, ( wkth.seq_no == MAX_SEQ_NUM ) || ( ( wkth.p_turn == *prod_id )
      								      && ( wkth.data_consumed == 1 ) ));
      

      /* All work has already been produced; quit */
      if (wkth.seq_no == MAX_SEQ_NUM)
	break;
      

      wkth.seq_no++;
	      
      sprintf(wkth.data,"Worker%d: Work Seq#%d",*prod_id,wkth.seq_no);
     
      printk(KERN_INFO "Worker%d: Work Seq#%d",*prod_id,wkth.seq_no);		
      /* Set the turn to next producer */
      
      wkth.c_turn = wkth.p_turn ;

      wkth.data_consumed = 0;
      
      /* Wake up the consumers waiting on consumer Q */
      wake_up_interruptible(&consumer_q);




    }
  
  while(1)
    {
      if(kthread_should_stop())
	break;
      else
	schedule(); 
    }
  
  
  return 0;
}

int cons_func(void * data)
{
  int * cons_id = (int *) data;
  
  
  while(wkth.seq_no <= MAX_SEQ_NUM)
    {
      
      /* Add to wait queue until turn comes */
      wait_event_interruptible(consumer_q,( (wkth.seq_no == MAX_SEQ_NUM) && ( wkth.data_consumed == 1 ) ) || ( ( wkth.c_turn == *cons_id ) 
													       && ( wkth.data_consumed == 0 ) ));

      
      if(kthread_should_stop())
	break;

      
      if (wkth.seq_no == MAX_SEQ_NUM && wkth.data_consumed == 1)
	{
	  /* All work has already been consumed */
	  break;
	}
	          
      printk(KERN_INFO "Reader%d: %s",*cons_id,wkth.data);		


      /* Set the turn to next producer */

      if(wkth.c_turn == THREADS_MAX)
	wkth.p_turn = 1 ;
      else
	wkth.p_turn += 1;
	      	
      wkth.data_consumed = 1;
      
      
      /* Wake up the producer Q */
      wake_up_interruptible(&producer_q);
      if (wkth.seq_no == MAX_SEQ_NUM)
	{
	  /* Force other consumers to quit */
	  wake_up_interruptible(&consumer_q);
	  break;
	}


    }
  
  while(1) 
    {
      if(kthread_should_stop())
	break;
      else
	schedule(); 
    }
 
  
  return 0;
}

void cleanup_worker_threads(void)
{
  int th_id_it = 0;

  for ( th_id_it = 0 ; th_id_it < THREADS_MAX ; th_id_it++ )
    {
      if(prod_threads[th_id_it] != NULL)
	kthread_stop(prod_threads[th_id_it]);

      if(cons_threads[th_id_it] != NULL)
	kthread_stop(cons_threads[th_id_it]);
      
      
    }

  

}

void init_worker_threads(void)
{
  
  int th_id_it = 0;

  wkth.seq_no = 0;
  wkth.p_turn = 1;
  wkth.c_turn = 0;
  wkth.data_consumed = 1;
  
  strcpy (wkth.data,"");
  
  for ( th_id_it = 0 ; th_id_it < THREADS_MAX ; th_id_it++ )
    {
      prod_kthids[th_id_it] = th_id_it + 1;
      prod_threads[th_id_it] = kthread_run(&prod_func,(void *) &(prod_kthids[th_id_it]),"kwth_prod/%d",th_id_it + 1);

      cons_kthids[th_id_it] = th_id_it + 1;
      cons_threads[th_id_it] = kthread_run(&cons_func,(void *) &(cons_kthids[th_id_it]),"kwth_cons/%d",th_id_it + 1);
      
    }

}

static int __init kwth_init(void)
{
    printk(KERN_INFO "Loaded kwth module\n");
    init_worker_threads();
    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit kwth_cleanup(void)
{
    printk(KERN_INFO "Cleaning up module.\n");
    cleanup_worker_threads();
}



module_init(kwth_init);
module_exit(kwth_cleanup);
