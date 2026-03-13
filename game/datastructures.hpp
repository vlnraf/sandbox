#ifndef DATA_STRUCTURES_HPP
#define DATA_STRUCTURES_HPP

#define ASSERT(condition, msg)                                              \
    do {                                                                    \
        if(!(condition)) {                                                  \
            LOGERROR("ASSERT FAILED: %s | %s:%d", msg, __FILE__, __LINE__); \
            __builtin_debugtrap();                                          \
        }                                                                   \
    } while(0)

//NOTE: I am using macros because if i pass a pointer to a function and assign it 
//      it would be assigned only locally it behaves like a value pointer
//example:
//  void test(int* a){
//     b = [2];
//     a = b;
//  }
// int main(void){
//      int a[3];
//      test(a);
//  }
//  a will still point to a in the main not at b

// STACK -------------------------------
#define stackPush(f, n)         \
    ((n)->next = (f), (f) = (n))\

#define stackPop(f)     \
    ((f) = (f)->next)   \

// QUEUE -------------------------------
#define queuePush(f, l, n)  \
    do{                     \
        (n)->next = NULL;   \
        if(f == NULL){      \
            f = n;          \
            l = n;          \
        }else{              \
            (l)->next = n;  \
            (l) = n;        \
        }                   \
    } while(0)

#define queuePop(f, l)                  \
    ASSERT((l && f), "Queue is empty"); \
    (f) = (f)->next                     \


// LINKED LIST --------------------------
#define llInsert(f, l, p, n)    \
    (n)->next = (p)->next;      \
    (p)->next = (n);            \
    if((p) == (l)) {(l) = (n);}


#define llRemove(f, l, p, n)    \
    (p)->next = n->next;        \
    if((n) == (l)) {(l) = (p);} \
    (n) = NULL;

#endif