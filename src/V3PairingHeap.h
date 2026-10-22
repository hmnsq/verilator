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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"

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

template <typename T_کونچي>
class PairingHeap final {
عام:
    بنية نود_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    بنية Link final {
        نود_2* m_ptr = nullptr;  // The managed pointer

        Link() = افتراضي;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE فراغ link(نود_2* targetp) {
            m_ptr = targetp;
            إذا (!targetp) أرجع;
#إذا VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE فراغ linkNonNull(نود_2* targetp) {
            m_ptr = targetp;
#إذا VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE نود_2* unlink() {
            نود_2* ثابت result = m_ptr;
#إذا VL_DEBUG
            إذا (result) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#endif
            m_ptr = nullptr;
            أرجع result;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE نود_2* ptr() ثابت { أرجع m_ptr; }
        VL_ATTR_ALWINLINE operator bool() ثابت { أرجع m_ptr; }
        VL_ATTR_ALWINLINE bool operator!() ثابت { أرجع !m_ptr; }
        VL_ATTR_ALWINLINE نود_2* operator->() ثابت { أرجع m_ptr; }
        VL_ATTR_ALWINLINE نود_2& operator*() ثابت { أرجع *m_ptr; }
    };

    // A single node in the pairing heap tree
    بنية نود_2 VL_NOT_FINAL {
        Link m_next;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        نود_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_کونچي m_کونچي{};  // The key in the heap

        // CONSTRUCTOR
        explicit نود_2() = افتراضي;
        VL_UNCOPYABLE(نود_2);

        // METHODS
        VL_ATTR_ALWINLINE ثابت T_کونچي& کونچي() ثابت { أرجع m_کونچي; }
        VL_ATTR_ALWINLINE bool operator<(ثابت نود_2& that) ثابت { أرجع m_کونچي < that.m_کونچي; }
        VL_ATTR_ALWINLINE bool operator>(ثابت نود_2& that) ثابت { أرجع that.m_کونچي < m_کونچي; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE فراغ replaceWith(نود_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            إذا (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE فراغ replaceWithNonNull(نود_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE فراغ yank() {
            m_next.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

خاص:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_اکر;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

عام:
    explicit PairingHeap() = افتراضي;

    // METHODS
    bool empty() ثابت { أرجع !m_اکر; }

    // Insert given node into this heap with given key.
    فراغ سيسيڤ(نود_2* nodep, T_کونچي کونچي) {
        // Update key of node
        nodep->m_کونچي = کونچي;
        سيسيڤ(nodep);
    }

    // Insert given node into this heap with key already set in the node
    فراغ سيسيڤ(نود_2* nodep) {
#إذا VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_next && !nodep->m_kids, "Already linked");
#endif
        // Just stick it at the front of the root list
        nodep->m_next.link(m_اکر.unlink());
        m_اکر.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    فراغ remove(نود_2* nodep) {
        إذا (!nodep->m_next) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } وإلا إذا (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_next.unlink());
        } وإلا {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            نود_2* ثابت reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    نود_2* max() ثابت {
        // Heap might be empty
        إذا (!m_اکر) أرجع nullptr;
        // If the root have siblings reduce them
        إذا (m_اکر->m_next) m_اکر.linkNonNull(reduce(m_اکر.unlink()));
        // The root element is the largest
        أرجع m_اکر.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    نود_2* secondMax() ثابت {
#إذا VL_DEBUG
        UASSERT(m_اکر, "'max' would have returned nullptr");
        UASSERT(!m_اکر->m_next, "'max' would have reduced");
#endif
        // If there are no children, there is no second element
        إذا (!m_اکر->m_kids) أرجع nullptr;
        // If there are multiple children, reduce them
        إذا (m_اکر->m_kids->m_next) m_اکر->m_kids.linkNonNull(reduce(m_اکر->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        أرجع m_اکر->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    template <typename T_Update>
    فراغ increase_کونچي(نود_2* nodep, T_Update نيلاي) {
        // Update the key
        nodep->m_کونچي.increase(نيلاي);
        // Increasing the key of the root is easy
        إذا (nodep == m_اکر.ptr()) أرجع;
        // Otherwise we do have a little work to do
        إذا (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_next.unlink());
        } وإلا إذا (!nodep->m_next) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } وإلا {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            نود_2* ثابت kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_next.unlink());
            kidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_next.linkNonNull(m_اکر.unlink());
        m_اکر.linkNonNull(nodep);
    }

خاص:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE ساكن نود_2* merge(نود_2* ap, نود_2* bp) {
#إذا VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_next, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_next, "Not root b");
#endif
        إذا (*ap > *bp) {  // bp goes under ap
            bp->m_next.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            أرجع ap;
        }  // else ap goes under bp
        ap->m_next.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        أرجع bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE ساكن نود_2* reduce(نود_2* nodep) {
#إذا VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#endif
        // If there is only one node in the list, then there is nothing to do
        إذا (!nodep->m_next) أرجع nodep;
        // The result node
        نود_2* resultp = nullptr;
        // Pairwise merge the child nodes
        طالما (nodep) {
            // Pop off the first nodes
            نود_2* ثابت ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            إذا (!nodep->m_next) {
                ap->m_next.link(resultp);
                resultp = ap;
                اكسر;
            }
            // Pop off the second nodes
            نود_2* ثابت bp = nodep->m_next.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_next.unlink();
            // Merge the current pair
            نود_2* ثابت mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        طالما (resultp->m_next) {
            // Pop first two results
            نود_2* ثابت ap = resultp;
            نود_2* ثابت bp = resultp->m_next.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_next.unlink();
            // Merge the current pair
            نود_2* ثابت mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Done
        أرجع resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(حجم(PairingHeap<صحيح>) == حجم(PairingHeap<صحيح>::نود_2*), "Should be a pointer");

#endif  // Guard
