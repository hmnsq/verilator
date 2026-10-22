// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Pairing Heap data structure
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3PAIRINGHEAP_H_
#define VERILATOR_V3PAIRINGHEAP_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"

//=============================================================================
// Pairing heap (max-heap) with increase key and delete.
//
// While this is written as a generic data structure, its interface and
// implementation is finely tuned for use by parallel scheduling, and is critical
// to Verilation performance, so be very careful changing anything or adding any
// new operations that would impact either memory usage, or performance of the
// existing operations. This data structure is fully deterministic, meaning
// the order in which elements with equal keys are retrieved only depends on
// the order of operations performed on the heap.
//=============================================================================

template <typename T_کلید>
class PairingHeap final {
عمومی:
    ساختار گره_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    ساختار Link final {
        گره_2* m_ptr = nullptr;  // The managed pointer

        Link() = پیشفرض;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE تهی link(گره_2* targetp) {
            m_ptr = targetp;
            اگر (!targetp) بازگشت;
#اگر VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE تهی linkNonNull(گره_2* targetp) {
            m_ptr = targetp;
#اگر VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE گره_2* unlink() {
            گره_2* ثابت result = m_ptr;
#اگر VL_DEBUG
            اگر (result) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#endif
            m_ptr = nullptr;
            بازگشت result;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE گره_2* ptr() ثابت { بازگشت m_ptr; }
        VL_ATTR_ALWINLINE operator bool() ثابت { بازگشت m_ptr; }
        VL_ATTR_ALWINLINE bool operator!() ثابت { بازگشت !m_ptr; }
        VL_ATTR_ALWINLINE گره_2* operator->() ثابت { بازگشت m_ptr; }
        VL_ATTR_ALWINLINE گره_2& operator*() ثابت { بازگشت *m_ptr; }
    };

    // A single node in the pairing heap tree
    ساختار گره_2 VL_NOT_FINAL {
        Link m_next;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        گره_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_کلید m_کلید{};  // The key in the heap

        // CONSTRUCTOR
        explicit گره_2() = پیشفرض;
        VL_UNCOPYABLE(گره_2);

        // METHODS
        VL_ATTR_ALWINLINE ثابت T_کلید& کلید() ثابت { بازگشت m_کلید; }
        VL_ATTR_ALWINLINE bool operator<(ثابت گره_2& that) ثابت { بازگشت m_کلید < that.m_کلید; }
        VL_ATTR_ALWINLINE bool operator>(ثابت گره_2& that) ثابت { بازگشت that.m_کلید < m_کلید; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE تهی replaceWith(گره_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            اگر (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE تهی replaceWithNonNull(گره_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE تهی yank() {
            m_next.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

خصوصی:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_ریشه;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

عمومی:
    explicit PairingHeap() = پیشفرض;

    // METHODS
    bool empty() ثابت { بازگشت !m_ریشه; }

    // Insert given node into this heap with given key.
    تهی درج(گره_2* nodep, T_کلید کلید) {
        // Update key of node
        nodep->m_کلید = کلید;
        درج(nodep);
    }

    // Insert given node into this heap with key already set in the node
    تهی درج(گره_2* nodep) {
#اگر VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_next && !nodep->m_kids, "Already linked");
#endif
        // Just stick it at the front of the root list
        nodep->m_next.link(m_ریشه.unlink());
        m_ریشه.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    تهی remove(گره_2* nodep) {
        اگر (!nodep->m_next) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } وگرنه اگر (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_next.unlink());
        } وگرنه {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            گره_2* ثابت reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    گره_2* max() ثابت {
        // Heap might be empty
        اگر (!m_ریشه) بازگشت nullptr;
        // If the root have siblings reduce them
        اگر (m_ریشه->m_next) m_ریشه.linkNonNull(reduce(m_ریشه.unlink()));
        // The root element is the largest
        بازگشت m_ریشه.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    گره_2* secondMax() ثابت {
#اگر VL_DEBUG
        UASSERT(m_ریشه, "'max' would have returned nullptr");
        UASSERT(!m_ریشه->m_next, "'max' would have reduced");
#endif
        // If there are no children, there is no second element
        اگر (!m_ریشه->m_kids) بازگشت nullptr;
        // If there are multiple children, reduce them
        اگر (m_ریشه->m_kids->m_next) m_ریشه->m_kids.linkNonNull(reduce(m_ریشه->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        بازگشت m_ریشه->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    template <typename T_Update>
    تهی increase_کلید(گره_2* nodep, T_Update مقدار) {
        // Update the key
        nodep->m_کلید.increase(مقدار);
        // Increasing the key of the root is easy
        اگر (nodep == m_ریشه.ptr()) بازگشت;
        // Otherwise we do have a little work to do
        اگر (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_next.unlink());
        } وگرنه اگر (!nodep->m_next) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } وگرنه {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            گره_2* ثابت kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_next.unlink());
            kidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_next.linkNonNull(m_ریشه.unlink());
        m_ریشه.linkNonNull(nodep);
    }

خصوصی:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE ایستا گره_2* merge(گره_2* ap, گره_2* bp) {
#اگر VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_next, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_next, "Not root b");
#endif
        اگر (*ap > *bp) {  // bp goes under ap
            bp->m_next.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            بازگشت ap;
        }  // else ap goes under bp
        ap->m_next.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        بازگشت bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE ایستا گره_2* reduce(گره_2* nodep) {
#اگر VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#endif
        // If there is only one node in the list, then there is nothing to do
        اگر (!nodep->m_next) بازگشت nodep;
        // The result node
        گره_2* resultp = nullptr;
        // Pairwise merge the child nodes
        وقتی (nodep) {
            // Pop off the first nodes
            گره_2* ثابت ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            اگر (!nodep->m_next) {
                ap->m_next.link(resultp);
                resultp = ap;
                بشکن;
            }
            // Pop off the second nodes
            گره_2* ثابت bp = nodep->m_next.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_next.unlink();
            // Merge the current pair
            گره_2* ثابت mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        وقتی (resultp->m_next) {
            // Pop first two results
            گره_2* ثابت ap = resultp;
            گره_2* ثابت bp = resultp->m_next.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_next.unlink();
            // Merge the current pair
            گره_2* ثابت mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Done
        بازگشت resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(اندازه(PairingHeap<صحیح>) == اندازه(PairingHeap<صحیح>::گره_2*), "Should be a pointer");

#endif  // Guard
