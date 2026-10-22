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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"

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

template <typename T_ଚାବି>
class PairingHeap final {
ସର୍ବସାଧାରଣ:
    ସଂରଚନା ଗଣ୍ଠି_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    ସଂରଚନା Link final {
        ଗଣ୍ଠି_2* m_ptr = nullptr;  // The managed pointer

        Link() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ଶୂନ୍ୟ link(ଗଣ୍ଠି_2* targetp) {
            m_ptr = targetp;
            ଯଦି (!targetp) ଫେରନ୍ତୁ;
#ଯଦି VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ଶୂନ୍ୟ linkNonNull(ଗଣ୍ଠି_2* targetp) {
            m_ptr = targetp;
#ଯଦି VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE ଗଣ୍ଠି_2* unlink() {
            ଗଣ୍ଠି_2* ଅଚଳ result = m_ptr;
#ଯଦି VL_DEBUG
            ଯଦି (result) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#endif
            m_ptr = nullptr;
            ଫେରନ୍ତୁ result;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE ଗଣ୍ଠି_2* ptr() ଅଚଳ { ଫେରନ୍ତୁ m_ptr; }
        VL_ATTR_ALWINLINE operator bool() ଅଚଳ { ଫେରନ୍ତୁ m_ptr; }
        VL_ATTR_ALWINLINE bool operator!() ଅଚଳ { ଫେରନ୍ତୁ !m_ptr; }
        VL_ATTR_ALWINLINE ଗଣ୍ଠି_2* operator->() ଅଚଳ { ଫେରନ୍ତୁ m_ptr; }
        VL_ATTR_ALWINLINE ଗଣ୍ଠି_2& operator*() ଅଚଳ { ଫେରନ୍ତୁ *m_ptr; }
    };

    // A single node in the pairing heap tree
    ସଂରଚନା ଗଣ୍ଠି_2 VL_NOT_FINAL {
        Link m_next;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        ଗଣ୍ଠି_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_ଚାବି m_ଚାବି{};  // The key in the heap

        // CONSTRUCTOR
        explicit ଗଣ୍ଠି_2() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        VL_UNCOPYABLE(ଗଣ୍ଠି_2);

        // METHODS
        VL_ATTR_ALWINLINE ଅଚଳ T_ଚାବି& ଚାବି() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବି; }
        VL_ATTR_ALWINLINE bool operator<(ଅଚଳ ଗଣ୍ଠି_2& that) ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବି < that.m_ଚାବି; }
        VL_ATTR_ALWINLINE bool operator>(ଅଚଳ ଗଣ୍ଠି_2& that) ଅଚଳ { ଫେରନ୍ତୁ that.m_ଚାବି < m_ଚାବି; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ଶୂନ୍ୟ replaceWith(ଗଣ୍ଠି_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            ଯଦି (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ଶୂନ୍ୟ replaceWithNonNull(ଗଣ୍ଠି_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE ଶୂନ୍ୟ yank() {
            m_next.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_ମୂଳ;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

ସର୍ବସାଧାରଣ:
    explicit PairingHeap() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    bool empty() ଅଚଳ { ଫେରନ୍ତୁ !m_ମୂଳ; }

    // Insert given node into this heap with given key.
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(ଗଣ୍ଠି_2* nodep, T_ଚାବି ଚାବି) {
        // Update key of node
        nodep->m_ଚାବି = ଚାବି;
        ଭର୍ତ୍ତି(nodep);
    }

    // Insert given node into this heap with key already set in the node
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(ଗଣ୍ଠି_2* nodep) {
#ଯଦି VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_next && !nodep->m_kids, "Already linked");
#endif
        // Just stick it at the front of the root list
        nodep->m_next.link(m_ମୂଳ.unlink());
        m_ମୂଳ.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    ଶୂନ୍ୟ remove(ଗଣ୍ଠି_2* nodep) {
        ଯଦି (!nodep->m_next) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } ଅନ୍ୟଥା ଯଦି (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_next.unlink());
        } ଅନ୍ୟଥା {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            ଗଣ୍ଠି_2* ଅଚଳ reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    ଗଣ୍ଠି_2* max() ଅଚଳ {
        // Heap might be empty
        ଯଦି (!m_ମୂଳ) ଫେରନ୍ତୁ nullptr;
        // If the root have siblings reduce them
        ଯଦି (m_ମୂଳ->m_next) m_ମୂଳ.linkNonNull(reduce(m_ମୂଳ.unlink()));
        // The root element is the largest
        ଫେରନ୍ତୁ m_ମୂଳ.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    ଗଣ୍ଠି_2* secondMax() ଅଚଳ {
#ଯଦି VL_DEBUG
        UASSERT(m_ମୂଳ, "'max' would have returned nullptr");
        UASSERT(!m_ମୂଳ->m_next, "'max' would have reduced");
#endif
        // If there are no children, there is no second element
        ଯଦି (!m_ମୂଳ->m_kids) ଫେରନ୍ତୁ nullptr;
        // If there are multiple children, reduce them
        ଯଦି (m_ମୂଳ->m_kids->m_next) m_ମୂଳ->m_kids.linkNonNull(reduce(m_ମୂଳ->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        ଫେରନ୍ତୁ m_ମୂଳ->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    template <typename T_Update>
    ଶୂନ୍ୟ increase_ଚାବି(ଗଣ୍ଠି_2* nodep, T_Update ମୂଲ୍ୟ) {
        // Update the key
        nodep->m_ଚାବି.increase(ମୂଲ୍ୟ);
        // Increasing the key of the root is easy
        ଯଦି (nodep == m_ମୂଳ.ptr()) ଫେରନ୍ତୁ;
        // Otherwise we do have a little work to do
        ଯଦି (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_next.unlink());
        } ଅନ୍ୟଥା ଯଦି (!nodep->m_next) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } ଅନ୍ୟଥା {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            ଗଣ୍ଠି_2* ଅଚଳ kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_next.unlink());
            kidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_next.linkNonNull(m_ମୂଳ.unlink());
        m_ମୂଳ.linkNonNull(nodep);
    }

ବ୍ୟକ୍ତିଗତ:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE ସ୍ଥିର ଗଣ୍ଠି_2* merge(ଗଣ୍ଠି_2* ap, ଗଣ୍ଠି_2* bp) {
#ଯଦି VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_next, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_next, "Not root b");
#endif
        ଯଦି (*ap > *bp) {  // bp goes under ap
            bp->m_next.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            ଫେରନ୍ତୁ ap;
        }  // else ap goes under bp
        ap->m_next.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        ଫେରନ୍ତୁ bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE ସ୍ଥିର ଗଣ୍ଠି_2* reduce(ଗଣ୍ଠି_2* nodep) {
#ଯଦି VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#endif
        // If there is only one node in the list, then there is nothing to do
        ଯଦି (!nodep->m_next) ଫେରନ୍ତୁ nodep;
        // The result node
        ଗଣ୍ଠି_2* resultp = nullptr;
        // Pairwise merge the child nodes
        ଯେତେବେଳେ (nodep) {
            // Pop off the first nodes
            ଗଣ୍ଠି_2* ଅଚଳ ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            ଯଦି (!nodep->m_next) {
                ap->m_next.link(resultp);
                resultp = ap;
                ଭାଙ୍ଗ;
            }
            // Pop off the second nodes
            ଗଣ୍ଠି_2* ଅଚଳ bp = nodep->m_next.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_next.unlink();
            // Merge the current pair
            ଗଣ୍ଠି_2* ଅଚଳ mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        ଯେତେବେଳେ (resultp->m_next) {
            // Pop first two results
            ଗଣ୍ଠି_2* ଅଚଳ ap = resultp;
            ଗଣ୍ଠି_2* ଅଚଳ bp = resultp->m_next.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_next.unlink();
            // Merge the current pair
            ଗଣ୍ଠି_2* ଅଚଳ mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Done
        ଫେରନ୍ତୁ resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(ଆକାର(PairingHeap<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>) == ଆକାର(PairingHeap<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::ଗଣ୍ଠି_2*), "Should be a pointer");

#endif  // Guard
