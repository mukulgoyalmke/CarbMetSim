#ifndef PRIQ_H

#define PRIQ_H

/* Classes representing a Priority queue. This data structure is
 * particular efficient for adding items to a list and then deleting
 * the item with the smallest cost. 
 */

/* Representation of an individual element on a
 * priority queue.
 */

class PriQElt {
    PriQElt *left;
    PriQElt *right;
    PriQElt *parent;
    unsigned dist;
  protected:
    unsigned cost0;
    unsigned cost1;
    unsigned tie1;
    unsigned tie2;
  public:
    friend class PriQ;
    inline PriQElt();
    inline bool costs_less(PriQElt *);
    friend int main(int argc, char *argv[]);
};

// Constructor
inline PriQElt::PriQElt()
{
    left = 0;
    right = 0;
    parent = 0;
    dist = 0;
    cost0 = 0;
    cost1 = 0;
    tie1 = 0;
    tie2 = 0;
}

// Cost comparison function
inline bool PriQElt::costs_less(PriQElt *oqe)
{
    if (cost0 < oqe->cost0)
	return(true);
    else if (cost0 > oqe->cost0)
	return(false);
    else if (cost1 < oqe->cost1)
	return(true);
    else if (cost1 > oqe->cost1)
	return(false);
    else if (tie1 > oqe->tie1)
	return(true);
    else if (tie1 < oqe->tie1)
	return(false);
    else if (tie2 > oqe->tie2)
	return(true);
    else
	return(false);
}

/* Representation of the Priority queue itself
 */

class PriQ {
    PriQElt *root;
    void priq_adjust(PriQElt *balance_pt, int deleting);
    void priq_merge(PriQ & otherq);
  public:
    inline PriQ();
    inline PriQElt *priq_gethead();
    PriQElt *priq_rmhead();
    void priq_add(PriQElt *item);
    void priq_delete(PriQElt *item);
};

// Inline functions
inline PriQ::PriQ() : root(0)
{
}
inline PriQElt *PriQ::priq_gethead()
{
    return(root);
}

#endif
