/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    chlists.h
 * @brief   Lists and Queues header.
 *
 * @addtogroup os_lists
 * @details Linked-element access is centralized in the next/previous
 *          accessors. At hardening level 2 or higher, or with debug assertions
 *          enabled, doubly-linked accesses check reciprocal link consistency.
 *          At level 3, fetched pointers are also validated before dereferencing.
 *          Single-link accesses provide only the level-3 pointer check.
 * @note    Operations require readable input objects and stable links during
 *          access. Mutation functions check the affected links before
 *          updating them; these are local checks, not full integrity scans.
 * @{
 */

#ifndef CHLISTS_H
#define CHLISTS_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a generic single link list header and element.
 */
typedef struct ch_list ch_list_t;

/**
 * @brief   Structure representing a generic single link list header
 *          and element.
 */
struct ch_list {
  ch_list_t             *next;      /**< @brief Next in the list/queue.     */
};

/**
 * @brief   Type of a generic bidirectional linked list header and element.
 */
typedef struct ch_queue ch_queue_t;

/**
 * @brief   Structure representing a generic bidirectional linked list header
 *          and element.
 */
struct ch_queue {
  ch_queue_t            *next;      /**< @brief Next in the list/queue.     */
  ch_queue_t            *prev;      /**< @brief Previous in the queue.      */
};

/**
 * @brief   Type of a generic priority-ordered bidirectional linked list
 *          header and element.
 */
typedef struct ch_priority_queue ch_priority_queue_t;

/**
 * @brief   Structure representing a generic priority-ordered bidirectional
 *          linked list header and element.
 * @note    The link fields must have the same layout as those in
 *          @p ch_queue_t and @p prio must follow them. Thread queue elements
 *          overlay the two structures and priority inheritance reads
 *          @p prio while an element is linked through @p ch_queue_t.
 */
struct ch_priority_queue {
  ch_priority_queue_t   *next;      /**< @brief Next in the queue.          */
  ch_priority_queue_t   *prev;      /**< @brief Previous in the queue.      */
  tprio_t               prio;       /**< @brief Priority of this element.   */
};

/**
 * @brief   Type of a generic bidirectional linked delta list
 *          header and element.
 */
typedef struct ch_delta_list ch_delta_list_t;

/**
 * @brief   Delta list element and header structure.
 */
struct ch_delta_list {
  ch_delta_list_t       *next;      /**< @brief Next in the delta list.     */
  ch_delta_list_t       *prev;      /**< @brief Previous in the delta list. */
  sysinterval_t         delta;      /**< @brief Time interval from previous.*/
};

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Data part of a static queue object initializer.
 * @details This macro should be used when statically initializing a
 *          queue that is part of a bigger structure.
 *
 * @param[in] name      the name of the queue variable
 */
#define __CH_QUEUE_DATA(name) {(ch_queue_t *)&name, (ch_queue_t *)&name}

/**
 * @brief   Static queue object initializer.
 * @details Statically initialized queues require no explicit
 *          initialization using @p queue_init().
 *
 * @param[in] name      the name of the queue variable
 */
#define CH_QUEUE_DECL(name)                                                 \
  ch_queue_t name = __CH_QUEUE_DATA(name)

/**
 * @brief   Iterate over a queue list forwards
 * @pre     The cursor element must remain linked until the loop advances.
 *
 * @param[in] pos       pointer to @p ch_queue_t object to use as a loop cursor
 * @param[in] head      pointer to @p ch_queue_t head of queue
 *
 * @notapi
 */
#define ch_queue_for_each(pos, head)                                        \
  for (pos = ch_queue_next(head); pos != (head); pos = ch_queue_next(pos))

/**
 * @brief   Iterate over a queue list backwards
 * @pre     The cursor element must remain linked until the loop advances.
 *
 * @param[in] pos       pointer to @p ch_queue_t object to use as a loop cursor
 * @param[in] head      pointer to @p ch_queue_t head of queue
 *
 * @notapi
 */
#define ch_queue_for_each_reverse(pos, head)                                \
  for (pos = ch_queue_prev(head); pos != (head); pos = ch_queue_prev(pos))

/**
 * @brief   Get the enclosing object of a queue object
 *
 * @param[in] ptr       pointer to the member @p ch_queue_t object
 * @param[in] type      the type of the enclosing object
 * @param[in] member    the name of the @p ch_queue_t object
 *
 * @notapi
 */
#define ch_queue_get_owner(ptr, type, member)                               \
  __CH_OWNEROF(ptr, type, member)

/**
 * @brief   Get the first entry of a queue
 * @note    The queue is assumed to be not empty
 *
 * @param[in] head      pointer to @p ch_queue_t head of queue
 * @param[in] type      the type of the enclosing object
 * @param[in] member    the name of the @p ch_queue_t object
 *
 * @notapi
 */
#define ch_queue_first_owner(head, type, member)                            \
  __CH_OWNEROF(ch_queue_next(head), type, member)

/**
 * @brief   Get the last entry of a queue
 * @note    The queue is assumed to be not empty
 *
 * @param[in] head      pointer to @p ch_queue_t head of queue
 * @param[in] type      the type of the enclosing object
 * @param[in] member    the name of the @p ch_queue_t object
 *
 * @notapi
 */
#define ch_queue_last_owner(head, type, member)                             \
  __CH_OWNEROF(ch_queue_prev(head), type, member)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/* Early function prototypes required by the following headers.*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   List initialization.
 *
 * @param[out] lp       pointer to the list header
 *
 * @notapi
 */
static inline void ch_list_init(ch_list_t *lp) {

  lp->next = lp;
}

/**
 * @brief   Returns the next element in a single link list.
 * @pre     @p lp must point to a readable header or element.
 *          The link must remain stable during this operation.
 * @note    At hardening level 3 the fetched pointer is validated
 *          (NULL and alignment checks by default). Debug assertions alone
 *          do not enable this pointer check.
 *
 * @param[in] lp        pointer to the list header or element
 * @return              The next element pointer, possibly the header.
 *
 * @notapi
 */
static inline ch_list_t *ch_list_next(const ch_list_t *lp) {
  ch_list_t *next = lp->next;

  chSftValidateDataPointerX(3, next);

  return next;
}

/**
 * @brief   Evaluates to @p true if the specified list is empty.
 *
 * @param[in] lp        pointer to the list header
 * @return              The status of the list.
 *
 * @notapi
 */
static inline bool ch_list_isempty(ch_list_t *lp) {

  return (bool)(ch_list_next(lp) == lp);
}

/**
 * @brief   Evaluates to @p true if the specified list is not empty.
 *
 * @param[in] lp        pointer to the list header
 * @return              The status of the list.
 *
 * @notapi
 */
static inline bool ch_list_notempty(ch_list_t *lp) {

  return (bool)(ch_list_next(lp) != lp);
}

/**
 * @brief   Pushes an element on top of a stack list.
 *
 * @param[in] lp    the pointer to the list header
 * @param[in] p     the pointer to the element to be inserted in the list
 *
 * @notapi
 */
static inline void ch_list_link(ch_list_t *lp, ch_list_t *p) {

  p->next = ch_list_next(lp);
  lp->next = p;
}

/**
 * @brief   Pops an element from the top of a stack list and returns it.
 * @pre     The list must be non-empty before calling this function.
 *
 * @param[in] lp        the pointer to the list header
 * @return              The removed element pointer.
 *
 * @notapi
 */
static inline ch_list_t *ch_list_unlink(ch_list_t *lp) {
  ch_list_t *p;

  chDbgAssert(ch_list_notempty(lp), "empty list");

  p = ch_list_next(lp);
  lp->next = ch_list_next(p);

  return p;
}

/**
 * @brief   Queue initialization.
 *
 * @param[out] qp       pointer to the queue header
 *
 * @notapi
 */
static inline void ch_queue_init(ch_queue_t *qp) {

  qp->next = qp;
  qp->prev = qp;
}

/**
 * @brief   Returns the next element in a queue.
 * @pre     @p qp must point to a readable header or linked element.
 *          The links must remain stable during this operation.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, the traversed forward/backward link pair is checked
 *          for consistency.
 * @note    At hardening level 3 the fetched pointer is validated before
 *          dereferencing it (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this pointer check.
 *
 * @param[in] qp        pointer to the queue header or element
 * @return              The next element pointer, possibly the header.
 *
 * @notapi
 */
static inline ch_queue_t *ch_queue_next(const ch_queue_t *qp) {
  ch_queue_t *next = qp->next;

  chSftValidateDataPointerX(3, next);
  chSftAssert(2, next->prev == qp, "link back");

  return next;
}

/**
 * @brief   Returns the previous element in a queue.
 * @pre     @p qp must point to a readable header or linked element.
 *          The links must remain stable during this operation.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, the traversed forward/backward link pair is checked
 *          for consistency.
 * @note    At hardening level 3 the fetched pointer is validated before
 *          dereferencing it (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this pointer check.
 *
 * @param[in] qp        pointer to the queue header or element
 * @return              The previous element pointer, possibly the header.
 *
 * @notapi
 */
static inline ch_queue_t *ch_queue_prev(const ch_queue_t *qp) {
  ch_queue_t *prev = qp->prev;

  chSftValidateDataPointerX(3, prev);
  chSftAssert(2, prev->next == qp, "link back");

  return prev;
}

/**
 * @brief   Evaluates to @p true if the specified queue is empty.
 *
 * @param[in] qp        pointer to the queue header
 * @return              The status of the queue.
 *
 * @notapi
 */
static inline bool ch_queue_isempty(const ch_queue_t *qp) {

  return (bool)(ch_queue_next(qp) == qp);
}

/**
 * @brief   Evaluates to @p true if the specified queue is not empty.
 *
 * @param[in] qp        pointer to the queue header
 * @return              The status of the queue.
 *
 * @notapi
 */
static inline bool ch_queue_notempty(const ch_queue_t *qp) {

  return (bool)(ch_queue_next(qp) != qp);
}

/**
 * @brief   Inserts an element into a queue.
 *
 * @param[in] qp        the pointer to the queue header
 * @param[in] p         the pointer to the element to be inserted in the queue
 *
 * @notapi
 */
static inline void ch_queue_insert(ch_queue_t *qp, ch_queue_t *p) {
  ch_queue_t *prev = ch_queue_prev(qp);

  p->next    = qp;
  p->prev    = prev;
  prev->next = p;
  qp->prev   = p;
}

/**
 * @brief   Removes the first-out element from a queue and returns it.
 * @note    If the queue is priority ordered then this function returns the
 *          element with the highest priority.
 *
 * @param[in] qp        the pointer to the queue list header
 * @return              The removed element pointer.
 *
 * @notapi
 */
static inline ch_queue_t *ch_queue_fifo_remove(ch_queue_t *qp) {
  ch_queue_t *p, *next;

  chDbgAssert(ch_queue_notempty(qp), "empty queue");

  p    = ch_queue_next(qp);
  next = ch_queue_next(p);

  qp->next   = next;
  next->prev = qp;

  return p;
}

/**
 * @brief   Removes the last-out element from a queue and returns it.
 * @note    If the queue is priority ordered then this function returns the
 *          element with the lowest priority.
 *
 * @param[in] qp    the pointer to the queue list header
 * @return          The removed element pointer.
 *
 * @notapi
 */
static inline ch_queue_t *ch_queue_lifo_remove(ch_queue_t *qp) {
  ch_queue_t *p = ch_queue_prev(qp);
  ch_queue_t *prev = ch_queue_prev(p);

  qp->prev   = prev;
  prev->next = qp;

  return p;
}

/**
 * @brief   Removes an element from a queue and returns it.
 * @details The element is removed from the queue regardless of its relative
 *          position and regardless the used insertion method.
 *
 * @param[in] p         the pointer to the element to be removed from the queue
 * @return              The removed element pointer.
 *
 * @notapi
 */
static inline ch_queue_t *ch_queue_dequeue(ch_queue_t *p) {
  ch_queue_t *prev = ch_queue_prev(p);
  ch_queue_t *next = ch_queue_next(p);

  prev->next = next;
  next->prev = prev;

  return p;
}

/**
 * @brief   Priority queue initialization.
 * @note    The queue header priority is initialized to zero, all other
 *          elements in the queue are assumed to have priority greater
 *          than zero.
 *
 * @param[out] pqp      pointer to the priority queue header
 *
 * @notapi
 */
static inline void ch_pqueue_init(ch_priority_queue_t *pqp) {

  pqp->next = pqp;
  pqp->prev = pqp;
  pqp->prio = (tprio_t)0;
}

/**
 * @brief   Returns the next element in a priority queue.
 * @pre     @p pqp must point to a readable header or linked element.
 *          The links must remain stable during this operation.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, the traversed forward/backward link pair is checked
 *          for consistency.
 * @note    At hardening level 3 the fetched pointer is validated before
 *          dereferencing it (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this pointer check.
 *
 * @param[in] pqp       pointer to the priority queue header or element
 * @return              The next element pointer, possibly the header.
 *
 * @notapi
 */
static inline ch_priority_queue_t *ch_pqueue_next(const ch_priority_queue_t *pqp) {
  ch_priority_queue_t *next = pqp->next;

  chSftValidateDataPointerX(3, next);
  chSftAssert(2, next->prev == pqp, "link back");

  return next;
}

/**
 * @brief   Returns the previous element in a priority queue.
 * @pre     @p pqp must point to a readable header or linked element.
 *          The links must remain stable during this operation.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, the traversed forward/backward link pair is checked
 *          for consistency.
 * @note    At hardening level 3 the fetched pointer is validated before
 *          dereferencing it (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this pointer check.
 *
 * @param[in] pqp       pointer to the priority queue header or element
 * @return              The previous element pointer, possibly the header.
 *
 * @notapi
 */
static inline ch_priority_queue_t *ch_pqueue_prev(const ch_priority_queue_t *pqp) {
  ch_priority_queue_t *prev = pqp->prev;

  chSftValidateDataPointerX(3, prev);
  chSftAssert(2, prev->next == pqp, "link back");

  return prev;
}

/**
 * @brief   Removes the highest priority element from a priority queue and
 *          returns it.
 *
 * @param[in] pqp       the pointer to the priority queue list header
 * @return              The removed element pointer.
 *
 * @notapi
 */
static inline ch_priority_queue_t *ch_pqueue_remove_highest(ch_priority_queue_t *pqp) {
  ch_priority_queue_t *p = ch_pqueue_next(pqp);
  ch_priority_queue_t *next = ch_pqueue_next(p);

  pqp->next  = next;
  next->prev = pqp;

  return p;
}

/**
 * @brief   Inserts an element in the priority queue placing it behind
 *          its peers.
 * @details The element is positioned behind all elements with higher or
 *          equal priority.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, consistency of forward/backward link pairs is checked
 *          while traversing the list.
 * @note    At hardening level 3 pointers are also validated before
 *          dereferencing them (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this check.
 *
 * @param[in] pqp       the pointer to the priority queue list header
 * @param[in] p         the pointer to the element to be inserted in the queue
 * @return              The inserted element pointer.
 *
 * @notapi
 */
static inline ch_priority_queue_t *ch_pqueue_insert_behind(ch_priority_queue_t *pqp,
                                                           ch_priority_queue_t *p) {
  ch_priority_queue_t *prev;

  /* Scanning priority queue, the list is assumed to be mostly empty.*/
  do {
    pqp = ch_pqueue_next(pqp);
  } while (unlikely(pqp->prio >= p->prio));

  /* Insertion on prev.*/
  prev = ch_pqueue_prev(pqp);
  p->next    = pqp;
  p->prev    = prev;
  prev->next = p;
  pqp->prev  = p;

  return p;
}

/**
 * @brief   Inserts an element in the priority queue placing it ahead of
 *          its peers.
 * @details The element is positioned ahead of all elements with higher or
 *          equal priority.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, consistency of forward/backward link pairs is checked
 *          while traversing the list.
 * @note    At hardening level 3 pointers are also validated before
 *          dereferencing them (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this check.
 *
 * @param[in] pqp       the pointer to the priority queue list header
 * @param[in] p         the pointer to the element to be inserted in the queue
 * @return              The inserted element pointer.
 *
 * @notapi
 */
static inline ch_priority_queue_t *ch_pqueue_insert_ahead(ch_priority_queue_t *pqp,
                                                          ch_priority_queue_t *p) {
  ch_priority_queue_t *prev;

  /* Scanning priority queue, the list is assumed to be mostly empty.*/
  do {
    pqp = ch_pqueue_next(pqp);
  } while (unlikely(pqp->prio > p->prio));

  /* Insertion on prev.*/
  prev = ch_pqueue_prev(pqp);
  p->next    = pqp;
  p->prev    = prev;
  prev->next = p;
  pqp->prev  = p;

  return p;
}

/**
 * @brief   Delta list initialization.
 *
 * @param[out] dlhp    pointer to the delta list header
 *
 * @notapi
 */
static inline void ch_dlist_init(ch_delta_list_t *dlhp) {

  dlhp->next  = dlhp;
  dlhp->prev  = dlhp;
  dlhp->delta = (sysinterval_t)-1;
}

/**
 * @brief   Returns the next element in a delta list.
 * @pre     @p dlp must point to a readable header or linked element.
 *          The links must remain stable during this operation.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, the traversed forward/backward link pair is checked
 *          for consistency.
 * @note    At hardening level 3 the fetched pointer is validated before
 *          dereferencing it (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this pointer check.
 *
 * @param[in] dlp       pointer to the delta list header or element
 * @return              The next element pointer, possibly the header.
 *
 * @notapi
 */
static inline ch_delta_list_t *ch_dlist_next(const ch_delta_list_t *dlp) {
  ch_delta_list_t *next = dlp->next;

  chSftValidateDataPointerX(3, next);
  chSftAssert(2, next->prev == dlp, "link back");

  return next;
}

/**
 * @brief   Returns the previous element in a delta list.
 * @pre     @p dlp must point to a readable header or linked element.
 *          The links must remain stable during this operation.
 * @note    At hardening level 2 or higher, or when @p CH_DBG_ENABLE_ASSERTS
 *          is enabled, the traversed forward/backward link pair is checked
 *          for consistency.
 * @note    At hardening level 3 the fetched pointer is validated before
 *          dereferencing it (NULL and alignment checks by default).
 *          Debug assertions alone do not enable this pointer check.
 *
 * @param[in] dlp       pointer to the delta list header or element
 * @return              The previous element pointer, possibly the header.
 *
 * @notapi
 */
static inline ch_delta_list_t *ch_dlist_prev(const ch_delta_list_t *dlp) {
  ch_delta_list_t *prev = dlp->prev;

  chSftValidateDataPointerX(3, prev);
  chSftAssert(2, prev->next == dlp, "link back");

  return prev;
}

/**
 * @brief   Evaluates to @p true if the specified delta list is empty.
 *
 * @param[in] dlhp      pointer to the delta list header
 * @return              The status of the delta list.
 *
 * @notapi
 */
static inline bool ch_dlist_isempty(ch_delta_list_t *dlhp) {

  return (bool)(dlhp == ch_dlist_next(dlhp));
}

/**
 * @brief   Evaluates to @p true if the specified queue is not empty.
 *
 * @param[in] dlhp      pointer to the delta list header
 * @return              The status of the delta list.
 *
 * @notapi
 */
static inline bool ch_dlist_notempty(ch_delta_list_t *dlhp) {

  return (bool)(dlhp != ch_dlist_next(dlhp));
}

/**
 * @brief   Last element in the delta list check.
 *
 * @param[in] dlhp      pointer to the delta list header
 * @param[in] dlp       pointer to the delta list element
 *
 * @notapi
 */
static inline bool ch_dlist_islast(ch_delta_list_t *dlhp,
                                   ch_delta_list_t *dlp) {

  return (bool)(ch_dlist_next(dlp) == dlhp);
}

/**
 * @brief   Fist element in the delta list check.
 *
 * @param[in] dlhp      pointer to the delta list header
 * @param[in] dlp       pointer to the delta list element
 *
 * @notapi
 */
static inline bool ch_dlist_isfirst(ch_delta_list_t *dlhp,
                                    ch_delta_list_t *dlp) {

  return (bool)(ch_dlist_next(dlhp) == dlp);
}

/**
 * @brief   Inserts an element after another header element.
 *
 * @param[in] dlhp      pointer to the delta list header element
 * @param[in] dlp       element to be inserted after the header element
 * @param[in] delta     delta of the element to be inserted
 *
 * @notapi
 */
static inline void ch_dlist_insert_after(ch_delta_list_t *dlhp,
                                         ch_delta_list_t *dlp,
                                         sysinterval_t delta) {
  ch_delta_list_t *next = ch_dlist_next(dlhp);

  dlp->delta = delta;
  dlp->prev  = dlhp;
  dlp->next  = next;
  next->prev = dlp;
  dlhp->next = dlp;
}

/**
 * @brief   Inserts an element before another header element.
 *
 * @param[in] dlhp      pointer to the delta list header element
 * @param[in] dlp       element to be inserted before the header element
 * @param[in] delta     delta of the element to be inserted
 *
 * @notapi
 */
static inline void ch_dlist_insert_before(ch_delta_list_t *dlhp,
                                          ch_delta_list_t *dlp,
                                          sysinterval_t delta) {
  ch_delta_list_t *prev = ch_dlist_prev(dlhp);

  dlp->delta = delta;
  dlp->next  = dlhp;
  dlp->prev  = prev;
  prev->next = dlp;
  dlhp->prev = dlp;
}

/**
 * @brief   Inserts an element in a delta list.
 *
 * @param[in] dlhp      pointer to the delta list header element
 * @param[in] dlep      element to be inserted before the header element
 * @param[in] delta     delta of the element to be inserted
 *
 * @notapi
 */
static inline void ch_dlist_insert(ch_delta_list_t *dlhp,
                                   ch_delta_list_t *dlep,
                                   sysinterval_t delta) {
  ch_delta_list_t *dlp;

  /* The delta list is scanned in order to find the correct position for
     this element. */
  dlp = ch_dlist_next(dlhp);
  while (likely(dlp->delta < delta)) {
    /* Debug assert if the element is already in the list.*/
    chDbgAssert(dlp != dlep, "element already in list");

    delta -= dlp->delta;
    dlp = ch_dlist_next(dlp);
  }

  /* The timer is inserted in the delta list.*/
  ch_dlist_insert_before(dlp, dlep, delta);

  /* Adjusting delta for the following element.*/
  dlp->delta -= delta;

  /* Special case when the inserted element is in last position in the list,
     the value in the header must be restored, just doing it is faster than
     checking then doing.*/
  dlhp->delta = (sysinterval_t)-1;
}

/**
 * @brief   Dequeues an element from the delta list.
 *
 * @param[in] dlhp      pointer to the delta list header
 *
 * @notapi
 */
static inline ch_delta_list_t *ch_dlist_remove_first(ch_delta_list_t *dlhp) {
  ch_delta_list_t *dlp = ch_dlist_next(dlhp);
  ch_delta_list_t *next = ch_dlist_next(dlp);

  dlhp->next = next;
  next->prev = dlhp;

  return dlp;
}

/**
 * @brief   Dequeues an element from the delta list.
 * @note    This function only unlinks the element; it does NOT adjust the
 *          successor's @p delta field. The caller is responsible for adding
 *          @p dlp->delta to @p dlp->next->delta to preserve the cumulative
 *          time invariant of the delta list, unless the removal context makes
 *          such an adjustment unnecessary (e.g. the element is being fired and
 *          @p lasttime is being stepped forward by @p dlp->delta instead).
 * @note    The @p dlp->next pointer is intentionally left intact after the
 *          unlink so that callers can still read the old successor through
 *          @p dlp->next when performing the delta adjustment after the call.
 *
 * @param[in] dlp       pointer to the delta list element
 *
 * @notapi
 */
static inline ch_delta_list_t *ch_dlist_dequeue(ch_delta_list_t *dlp) {
  ch_delta_list_t *prev = ch_dlist_prev(dlp);
  ch_delta_list_t *next = ch_dlist_next(dlp);

  prev->next = next;
  next->prev = prev;

  return dlp;
}

#endif /* CHLISTS_H */

/** @} */
