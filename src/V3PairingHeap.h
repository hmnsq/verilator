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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"

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

template <typename T_চাবি>
class PairingHeap final {
সর্বজনীন:
    কাঠামো গ্রন্থি_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    কাঠামো Link final {
        গ্রন্থি_2* m_ptr = nullptr;  // The managed pointer

        Link() = পূর্বনির্ধারিত;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE শূন্য link(গ্রন্থি_2* targetp) {
            m_ptr = targetp;
            যদি (!targetp) ফেরত;
#যদি VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE শূন্য linkNonNull(গ্রন্থি_2* targetp) {
            m_ptr = targetp;
#যদি VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE গ্রন্থি_2* unlink() {
            গ্রন্থি_2* ধ্রুবক ফলাফল = m_ptr;
#যদি VL_DEBUG
            যদি (ফলাফল) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#endif
            m_ptr = nullptr;
            ফেরত ফলাফল;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE গ্রন্থি_2* ptr() ধ্রুবক { ফেরত m_ptr; }
        VL_ATTR_ALWINLINE operator bool() ধ্রুবক { ফেরত m_ptr; }
        VL_ATTR_ALWINLINE bool operator!() ধ্রুবক { ফেরত !m_ptr; }
        VL_ATTR_ALWINLINE গ্রন্থি_2* operator->() ধ্রুবক { ফেরত m_ptr; }
        VL_ATTR_ALWINLINE গ্রন্থি_2& operator*() ধ্রুবক { ফেরত *m_ptr; }
    };

    // A single node in the pairing heap tree
    কাঠামো গ্রন্থি_2 VL_NOT_FINAL {
        Link m_পরবর্তী;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        গ্রন্থি_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_চাবি m_চাবি{};  // The key in the heap

        // CONSTRUCTOR
        explicit গ্রন্থি_2() = পূর্বনির্ধারিত;
        VL_UNCOPYABLE(গ্রন্থি_2);

        // METHODS
        VL_ATTR_ALWINLINE ধ্রুবক T_চাবি& চাবি() ধ্রুবক { ফেরত m_চাবি; }
        VL_ATTR_ALWINLINE bool operator<(ধ্রুবক গ্রন্থি_2& that) ধ্রুবক { ফেরত m_চাবি < that.m_চাবি; }
        VL_ATTR_ALWINLINE bool operator>(ধ্রুবক গ্রন্থি_2& that) ধ্রুবক { ফেরত that.m_চাবি < m_চাবি; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE শূন্য replaceWith(গ্রন্থি_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            যদি (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE শূন্য replaceWithNonNull(গ্রন্থি_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE শূন্য yank() {
            m_পরবর্তী.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

ব্যক্তিগত:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_মূল;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

সর্বজনীন:
    explicit PairingHeap() = পূর্বনির্ধারিত;

    // METHODS
    bool empty() ধ্রুবক { ফেরত !m_মূল; }

    // Insert given node into this heap with given key.
    শূন্য ঢোকাও(গ্রন্থি_2* nodep, T_চাবি চাবি) {
        // Update key of node
        nodep->m_চাবি = চাবি;
        ঢোকাও(nodep);
    }

    // Insert given node into this heap with key already set in the node
    শূন্য ঢোকাও(গ্রন্থি_2* nodep) {
#যদি VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_পরবর্তী && !nodep->m_kids, "Already linked");
#endif
        // Just stick it at the front of the root list
        nodep->m_পরবর্তী.link(m_মূল.unlink());
        m_মূল.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    শূন্য remove(গ্রন্থি_2* nodep) {
        যদি (!nodep->m_পরবর্তী) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } অন্যথা যদি (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_পরবর্তী.unlink());
        } অন্যথা {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            গ্রন্থি_2* ধ্রুবক reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_পরবর্তী.linkNonNull(nodep->m_পরবর্তী.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    গ্রন্থি_2* max() ধ্রুবক {
        // Heap might be empty
        যদি (!m_মূল) ফেরত nullptr;
        // If the root have siblings reduce them
        যদি (m_মূল->m_পরবর্তী) m_মূল.linkNonNull(reduce(m_মূল.unlink()));
        // The root element is the largest
        ফেরত m_মূল.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    গ্রন্থি_2* secondMax() ধ্রুবক {
#যদি VL_DEBUG
        UASSERT(m_মূল, "'max' would have returned nullptr");
        UASSERT(!m_মূল->m_পরবর্তী, "'max' would have reduced");
#endif
        // If there are no children, there is no second element
        যদি (!m_মূল->m_kids) ফেরত nullptr;
        // If there are multiple children, reduce them
        যদি (m_মূল->m_kids->m_পরবর্তী) m_মূল->m_kids.linkNonNull(reduce(m_মূল->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        ফেরত m_মূল->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    template <typename T_Update>
    শূন্য increase_চাবি(গ্রন্থি_2* nodep, T_Update মান) {
        // Update the key
        nodep->m_চাবি.increase(মান);
        // Increasing the key of the root is easy
        যদি (nodep == m_মূল.ptr()) ফেরত;
        // Otherwise we do have a little work to do
        যদি (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_পরবর্তী.unlink());
        } অন্যথা যদি (!nodep->m_পরবর্তী) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } অন্যথা {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            গ্রন্থি_2* ধ্রুবক kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_পরবর্তী.unlink());
            kidsp->m_পরবর্তী.linkNonNull(nodep->m_পরবর্তী.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_পরবর্তী.linkNonNull(m_মূল.unlink());
        m_মূল.linkNonNull(nodep);
    }

ব্যক্তিগত:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE স্থির গ্রন্থি_2* merge(গ্রন্থি_2* ap, গ্রন্থি_2* bp) {
#যদি VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_পরবর্তী, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_পরবর্তী, "Not root b");
#endif
        যদি (*ap > *bp) {  // bp goes under ap
            bp->m_পরবর্তী.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            ফেরত ap;
        }  // else ap goes under bp
        ap->m_পরবর্তী.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        ফেরত bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE স্থির গ্রন্থি_2* reduce(গ্রন্থি_2* nodep) {
#যদি VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#endif
        // If there is only one node in the list, then there is nothing to do
        যদি (!nodep->m_পরবর্তী) ফেরত nodep;
        // The result node
        গ্রন্থি_2* resultp = nullptr;
        // Pairwise merge the child nodes
        যখন (nodep) {
            // Pop off the first nodes
            গ্রন্থি_2* ধ্রুবক ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            যদি (!nodep->m_পরবর্তী) {
                ap->m_পরবর্তী.link(resultp);
                resultp = ap;
                বিরতি;
            }
            // Pop off the second nodes
            গ্রন্থি_2* ধ্রুবক bp = nodep->m_পরবর্তী.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_পরবর্তী.unlink();
            // Merge the current pair
            গ্রন্থি_2* ধ্রুবক mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_পরবর্তী.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        যখন (resultp->m_পরবর্তী) {
            // Pop first two results
            গ্রন্থি_2* ধ্রুবক ap = resultp;
            গ্রন্থি_2* ধ্রুবক bp = resultp->m_পরবর্তী.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_পরবর্তী.unlink();
            // Merge the current pair
            গ্রন্থি_2* ধ্রুবক mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_পরবর্তী.link(resultp);
            resultp = mergedp;
        }
        // Done
        ফেরত resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(আকার(PairingHeap<পূর্ণসংখ্যা>) == আকার(PairingHeap<পূর্ণসংখ্যা>::গ্রন্থি_2*), "Should be a pointer");

#endif  // Guard
