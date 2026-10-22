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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"

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

template <typename T_ચાવી>
class PairingHeap final {
જાહેર:
    સંરચના ગાંઠ_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    સંરચના Link final {
        ગાંઠ_2* m_ptr = nullptr;  // The managed pointer

        Link() = મૂળભૂત;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE શૂન્ય link(ગાંઠ_2* targetp) {
            m_ptr = targetp;
            જો (!targetp) પાછા;
#જો VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE શૂન્ય linkNonNull(ગાંઠ_2* targetp) {
            m_ptr = targetp;
#જો VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE ગાંઠ_2* unlink() {
            ગાંઠ_2* અચલ result = m_ptr;
#જો VL_DEBUG
            જો (result) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#endif
            m_ptr = nullptr;
            પાછા result;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE ગાંઠ_2* ptr() અચલ { પાછા m_ptr; }
        VL_ATTR_ALWINLINE operator bool() અચલ { પાછા m_ptr; }
        VL_ATTR_ALWINLINE bool operator!() અચલ { પાછા !m_ptr; }
        VL_ATTR_ALWINLINE ગાંઠ_2* operator->() અચલ { પાછા m_ptr; }
        VL_ATTR_ALWINLINE ગાંઠ_2& operator*() અચલ { પાછા *m_ptr; }
    };

    // A single node in the pairing heap tree
    સંરચના ગાંઠ_2 VL_NOT_FINAL {
        Link m_next;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        ગાંઠ_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_ચાવી m_ચાવી{};  // The key in the heap

        // CONSTRUCTOR
        explicit ગાંઠ_2() = મૂળભૂત;
        VL_UNCOPYABLE(ગાંઠ_2);

        // METHODS
        VL_ATTR_ALWINLINE અચલ T_ચાવી& ચાવી() અચલ { પાછા m_ચાવી; }
        VL_ATTR_ALWINLINE bool operator<(અચલ ગાંઠ_2& that) અચલ { પાછા m_ચાવી < that.m_ચાવી; }
        VL_ATTR_ALWINLINE bool operator>(અચલ ગાંઠ_2& that) અચલ { પાછા that.m_ચાવી < m_ચાવી; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE શૂન્ય replaceWith(ગાંઠ_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            જો (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE શૂન્ય replaceWithNonNull(ગાંઠ_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE શૂન્ય yank() {
            m_next.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

ખાનગી:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_મૂળ;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

જાહેર:
    explicit PairingHeap() = મૂળભૂત;

    // METHODS
    bool empty() અચલ { પાછા !m_મૂળ; }

    // Insert given node into this heap with given key.
    શૂન્ય ઉમેરો(ગાંઠ_2* nodep, T_ચાવી ચાવી) {
        // Update key of node
        nodep->m_ચાવી = ચાવી;
        ઉમેરો(nodep);
    }

    // Insert given node into this heap with key already set in the node
    શૂન્ય ઉમેરો(ગાંઠ_2* nodep) {
#જો VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_next && !nodep->m_kids, "Already linked");
#endif
        // Just stick it at the front of the root list
        nodep->m_next.link(m_મૂળ.unlink());
        m_મૂળ.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    શૂન્ય remove(ગાંઠ_2* nodep) {
        જો (!nodep->m_next) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } નહીંતર જો (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_next.unlink());
        } નહીંતર {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            ગાંઠ_2* અચલ reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    ગાંઠ_2* max() અચલ {
        // Heap might be empty
        જો (!m_મૂળ) પાછા nullptr;
        // If the root have siblings reduce them
        જો (m_મૂળ->m_next) m_મૂળ.linkNonNull(reduce(m_મૂળ.unlink()));
        // The root element is the largest
        પાછા m_મૂળ.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    ગાંઠ_2* secondMax() અચલ {
#જો VL_DEBUG
        UASSERT(m_મૂળ, "'max' would have returned nullptr");
        UASSERT(!m_મૂળ->m_next, "'max' would have reduced");
#endif
        // If there are no children, there is no second element
        જો (!m_મૂળ->m_kids) પાછા nullptr;
        // If there are multiple children, reduce them
        જો (m_મૂળ->m_kids->m_next) m_મૂળ->m_kids.linkNonNull(reduce(m_મૂળ->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        પાછા m_મૂળ->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    template <typename T_Update>
    શૂન્ય increase_ચાવી(ગાંઠ_2* nodep, T_Update કિંમત) {
        // Update the key
        nodep->m_ચાવી.increase(કિંમત);
        // Increasing the key of the root is easy
        જો (nodep == m_મૂળ.ptr()) પાછા;
        // Otherwise we do have a little work to do
        જો (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_next.unlink());
        } નહીંતર જો (!nodep->m_next) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } નહીંતર {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            ગાંઠ_2* અચલ kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_next.unlink());
            kidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_next.linkNonNull(m_મૂળ.unlink());
        m_મૂળ.linkNonNull(nodep);
    }

ખાનગી:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE સ્થિર ગાંઠ_2* merge(ગાંઠ_2* ap, ગાંઠ_2* bp) {
#જો VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_next, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_next, "Not root b");
#endif
        જો (*ap > *bp) {  // bp goes under ap
            bp->m_next.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            પાછા ap;
        }  // else ap goes under bp
        ap->m_next.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        પાછા bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE સ્થિર ગાંઠ_2* reduce(ગાંઠ_2* nodep) {
#જો VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#endif
        // If there is only one node in the list, then there is nothing to do
        જો (!nodep->m_next) પાછા nodep;
        // The result node
        ગાંઠ_2* resultp = nullptr;
        // Pairwise merge the child nodes
        જ્યારે (nodep) {
            // Pop off the first nodes
            ગાંઠ_2* અચલ ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            જો (!nodep->m_next) {
                ap->m_next.link(resultp);
                resultp = ap;
                તોડો;
            }
            // Pop off the second nodes
            ગાંઠ_2* અચલ bp = nodep->m_next.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_next.unlink();
            // Merge the current pair
            ગાંઠ_2* અચલ mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        જ્યારે (resultp->m_next) {
            // Pop first two results
            ગાંઠ_2* અચલ ap = resultp;
            ગાંઠ_2* અચલ bp = resultp->m_next.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_next.unlink();
            // Merge the current pair
            ગાંઠ_2* અચલ mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Done
        પાછા resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(કદ(PairingHeap<પૂર્ણાંક>) == કદ(PairingHeap<પૂર્ણાંક>::ગાંઠ_2*), "Should be a pointer");

#endif  // Guard
