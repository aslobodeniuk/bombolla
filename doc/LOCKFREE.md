# Reference-counting based waits

1. Action tree

   To execute certain action it's pushed to the Thread Pool (TP).
   If result of this action (A1) is used in another action (A2) - before pushing A1 to TP,
   we increase a reference count of the Action Chain (AC) of A2.

   For example we have an expression tree A0(A1(A4 A5) A2  A3).
   This is a dependency tree

      <----A3
   A0 <----A2
      <--- A1 <--- A4
              <--- A5

   Leaves of the tree : A2, A3, A4 and A5 can be executed in parallel.
   But A1 can only be executed when A4 and A5 are ready.
   A0 can only be executed when A1 A2 and A3 are ready.

   So what we do is that:
   - For each leaf - increase the refcount of AC of the parent and push to TP.
   - When this action is executed (from TP) it decreases the refcound of AC.
   - When the AC refcount reaches 0 - it executes the action of this AC.
   - To execute action of an AC it does the same as what the leaves were doing -
     increase the refcount on a parent and push to the TP.
   

2. Object access

   If an action performed on a certain object or a group of objects (O1 O2 O3..),
   but also another action is also scheduled for some of these objects, we manage to
   make each object used only by one action at a time.

   This also bases on refcounting.
   - When a certain action (A0) is going to be executed using objects O0 and O1, it
     appends itself to the Action Chain of each of the objects.

     Technically speaking it:
     1. increases the refcount of itself 2 times (1 for O0 another for O2)
     2. appends it's action (to decrease the ref) to the list of O0
     3. appends it's action (to decrease the ref) to the list of O1

     - if the action of list was empty the action (decreasing the refcount of A0) is executed immediatelly
     - 



Bombolla Async Mutex - BAM.
---------------------------

typedef struct BAM {
	GAsyncQueue actions_q;
	gboolean locked; // needed??
	wrapper_cb;
	wrapper_data;

	BAM* locked_by_lock_multiple; 
}

// called from the wrapper
void cb (BAM *bam, cb, data) {

}

bam_lock (bam, cb, data)
----> tries to "lock", if already locked pushes (cb, data) to the queue of actions
----> actions are executed one by one until the queue is empty. Then bam becomes unlocked.
----> IMHO can be implemented on pure lock-free

??? needed?? bam_interrupt (bam, cb, data) // flush the queue
Maybe for error handling....

bam_lock_multiple ((list of bams), cb, data)
----> creates a refcounted object and increases a ref by the number in the list
----> cb "locks" the bam and unrefs the object. In fact when the cb is executed the object is already locked.
      But it just doesn't chain to the next action from the queue, but returns instead.
      So while it's "locked" the queue grows: actions are not executed
      
----> requests this cb on each bam
----> when the refcount of the object reaches zero it unblocks each object
----> this means that it executes actions from queues of the bams one by one.
----> if more threads are requesting things frequently this overcharges the thread BUT I'm not sure this case makes sense,
because in this paradigm waiting's purpose is to wait until all the actions are done (to be able to commit the state).
Also in any case cb just pushes another task to the thread pool, so it's not one thread anyway.

??? bam_drain (bam, cb, data)

---------------------------
Note: with this primitive "action tree" seems to be unneeded, since each action is going to
lock all the dependencies.


---------------------------
FIXME: conflict when

bam_lock_multiple ((b1, b2), cb, data)
vs
bam_lock_multiple ((b2, b1), cb, data)
