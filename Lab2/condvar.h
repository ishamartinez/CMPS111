// Isha Aggarwal
// iaggarwa@ucsc.edu
// CMPS 111 Lab 2
// 12 Feb 2018


#ifndef CONDVAR_H
#define CONDVAR_H

#include "threads/semaphore.h"
#include "threads/lock.h"

/* Condition variable */
struct condvar {
    struct list waiters; /* List of semaphore_elems */
};
bool compare_semaphores(const struct list_elem *first, const struct list_elem *second, void *aux UNUSED);

void condvar_init(struct condvar *);
void condvar_wait(struct condvar *, struct lock *);
void condvar_signal(struct condvar *, struct lock *);
void condvar_broadcast(struct condvar *, struct lock *);

#endif /* UCSC CMPS111 */
