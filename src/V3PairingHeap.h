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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3PAIRINGHEAP_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3PAIRINGHEAP_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

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

ಮಾದರಿ <typename T_ಕೀಲಿ>
class Pairing_ರಾಶಿ final {
ಸಾರ್ವಜನಿಕ:
    ರಚನೆ ಗಂಟು_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    ರಚನೆ Link final {
        ಗಂಟು_2* m_ptr = nullptr;  // The managed pointer

        Link() = ಪೂರ್ವನಿಯೋಜಿತ;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ಶೂನ್ಯ link(ಗಂಟು_2* targetp) {
            m_ptr = targetp;
            ಆದರೆ (!targetp) ಹಿಂತಿರುಗಿ;
#ಆದರೆ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#ಆದರೆ_ಅಂತ್ಯ
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ಶೂನ್ಯ linkNonNull(ಗಂಟು_2* targetp) {
            m_ptr = targetp;
#ಆದರೆ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#ಆದರೆ_ಅಂತ್ಯ
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE ಗಂಟು_2* unlink() {
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ = m_ptr;
#ಆದರೆ VL_DEBUG
            ಆದರೆ (ಫಲಿತಾಂಶ) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#ಆದರೆ_ಅಂತ್ಯ
            m_ptr = nullptr;
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE ಗಂಟು_2* ptr() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ptr; }
        VL_ATTR_ALWINLINE ಕ್ರಿಯಾಕಾರಕ bool() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ptr; }
        VL_ATTR_ALWINLINE bool ಕ್ರಿಯಾಕಾರಕ!() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_ptr; }
        VL_ATTR_ALWINLINE ಗಂಟು_2* ಕ್ರಿಯಾಕಾರಕ->() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ptr; }
        VL_ATTR_ALWINLINE ಗಂಟು_2& ಕ್ರಿಯಾಕಾರಕ*() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ *m_ptr; }
    };

    // A single node in the pairing heap tree
    ರಚನೆ ಗಂಟು_2 VL_NOT_FINAL {
        Link m_ಮುಂದಿನ;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        ಗಂಟು_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_ಕೀಲಿ m_ಕೀಲಿ{};  // The key in the heap

        // CONSTRUCTOR
        explicit ಗಂಟು_2() = ಪೂರ್ವನಿಯೋಜಿತ;
        VL_UNCOPYABLE(ಗಂಟು_2);

        // METHODS
        VL_ATTR_ALWINLINE ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ಕೀಲಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ; }
        VL_ATTR_ALWINLINE bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ ಗಂಟು_2& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ < that.m_ಕೀಲಿ; }
        VL_ATTR_ALWINLINE bool ಕ್ರಿಯಾಕಾರಕ>(ಸ್ಥಿರಾಂಕ ಗಂಟು_2& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ that.m_ಕೀಲಿ < m_ಕೀಲಿ; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ಶೂನ್ಯ replaceWith(ಗಂಟು_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            ಆದರೆ (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ಶೂನ್ಯ replaceWithNonNull(ಗಂಟು_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE ಶೂನ್ಯ yank() {
            m_ಮುಂದಿನ.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

ಖಾಸಗಿ:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_ಬೇರು;

    // CONSTRUCTORS
    VL_UNCOPYABLE(Pairing_ರಾಶಿ);

ಸಾರ್ವಜನಿಕ:
    explicit Pairing_ರಾಶಿ() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    bool ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_ಬೇರು; }

    // Insert given node into this heap with given key.
    ಶೂನ್ಯ ಸೇರಿಸು(ಗಂಟು_2* nodep, T_ಕೀಲಿ ಕೀಲಿ) {
        // Update key of node
        nodep->m_ಕೀಲಿ = ಕೀಲಿ;
        ಸೇರಿಸು(nodep);
    }

    // Insert given node into this heap with key already set in the node
    ಶೂನ್ಯ ಸೇರಿಸು(ಗಂಟು_2* nodep) {
#ಆದರೆ VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_ಮುಂದಿನ && !nodep->m_kids, "Already linked");
#ಆದರೆ_ಅಂತ್ಯ
        // Just stick it at the front of the root list
        nodep->m_ಮುಂದಿನ.link(m_ಬೇರು.unlink());
        m_ಬೇರು.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    ಶೂನ್ಯ remove(ಗಂಟು_2* nodep) {
        ಆದರೆ (!nodep->m_ಮುಂದಿನ) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_ಮುಂದಿನ.unlink());
        } ಇಲ್ಲದಿದ್ದರೆ {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_ಮುಂದಿನ.linkNonNull(nodep->m_ಮುಂದಿನ.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    ಗಂಟು_2* max() ಸ್ಥಿರಾಂಕ {
        // Heap might be empty
        ಆದರೆ (!m_ಬೇರು) ಹಿಂತಿರುಗಿ nullptr;
        // If the root have siblings reduce them
        ಆದರೆ (m_ಬೇರು->m_ಮುಂದಿನ) m_ಬೇರು.linkNonNull(reduce(m_ಬೇರು.unlink()));
        // The root element is the largest
        ಹಿಂತಿರುಗಿ m_ಬೇರು.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    ಗಂಟು_2* ಎರಡನೇ_Max() ಸ್ಥಿರಾಂಕ {
#ಆದರೆ VL_DEBUG
        UASSERT(m_ಬೇರು, "'max' would have returned nullptr");
        UASSERT(!m_ಬೇರು->m_ಮುಂದಿನ, "'max' would have reduced");
#ಆದರೆ_ಅಂತ್ಯ
        // If there are no children, there is no second element
        ಆದರೆ (!m_ಬೇರು->m_kids) ಹಿಂತಿರುಗಿ nullptr;
        // If there are multiple children, reduce them
        ಆದರೆ (m_ಬೇರು->m_kids->m_ಮುಂದಿನ) m_ಬೇರು->m_kids.linkNonNull(reduce(m_ಬೇರು->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        ಹಿಂತಿರುಗಿ m_ಬೇರು->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    ಮಾದರಿ <typename T_Update>
    ಶೂನ್ಯ increase_ಕೀಲಿ(ಗಂಟು_2* nodep, T_Update ಮೌಲ್ಯ) {
        // Update the key
        nodep->m_ಕೀಲಿ.increase(ಮೌಲ್ಯ);
        // Increasing the key of the root is easy
        ಆದರೆ (nodep == m_ಬೇರು.ptr()) ಹಿಂತಿರುಗಿ;
        // Otherwise we do have a little work to do
        ಆದರೆ (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_ಮುಂದಿನ.unlink());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->m_ಮುಂದಿನ) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } ಇಲ್ಲದಿದ್ದರೆ {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_ಮುಂದಿನ.unlink());
            kidsp->m_ಮುಂದಿನ.linkNonNull(nodep->m_ಮುಂದಿನ.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_ಮುಂದಿನ.linkNonNull(m_ಬೇರು.unlink());
        m_ಬೇರು.linkNonNull(nodep);
    }

ಖಾಸಗಿ:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE ಸ್ಥಿರ ಗಂಟು_2* ವಿಲೀನ(ಗಂಟು_2* ap, ಗಂಟು_2* bp) {
#ಆದರೆ VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_ಮುಂದಿನ, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_ಮುಂದಿನ, "Not root b");
#ಆದರೆ_ಅಂತ್ಯ
        ಆದರೆ (*ap > *bp) {  // bp goes under ap
            bp->m_ಮುಂದಿನ.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            ಹಿಂತಿರುಗಿ ap;
        }  // else ap goes under bp
        ap->m_ಮುಂದಿನ.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        ಹಿಂತಿರುಗಿ bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE ಸ್ಥಿರ ಗಂಟು_2* reduce(ಗಂಟು_2* nodep) {
#ಆದರೆ VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#ಆದರೆ_ಅಂತ್ಯ
        // If there is only one node in the list, then there is nothing to do
        ಆದರೆ (!nodep->m_ಮುಂದಿನ) ಹಿಂತಿರುಗಿ nodep;
        // The result node
        ಗಂಟು_2* resultp = nullptr;
        // Pairwise merge the child nodes
        ಆಗ (nodep) {
            // Pop off the first nodes
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            ಆದರೆ (!nodep->m_ಮುಂದಿನ) {
                ap->m_ಮುಂದಿನ.link(resultp);
                resultp = ap;
                ನಿಲ್ಲಿಸು;
            }
            // Pop off the second nodes
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ bp = nodep->m_ಮುಂದಿನ.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_ಮುಂದಿನ.unlink();
            // Merge the current pair
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ mergedp = ವಿಲೀನ(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_ಮುಂದಿನ.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        ಆಗ (resultp->m_ಮುಂದಿನ) {
            // Pop first two results
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ ap = resultp;
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ bp = resultp->m_ಮುಂದಿನ.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_ಮುಂದಿನ.unlink();
            // Merge the current pair
            ಗಂಟು_2* ಸ್ಥಿರಾಂಕ mergedp = ವಿಲೀನ(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_ಮುಂದಿನ.link(resultp);
            resultp = mergedp;
        }
        //ಮುಗಿದ
        ಹಿಂತಿರುಗಿ resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(ಗಾತ್ರ(Pairing_ರಾಶಿ<ಪೂರ್ಣಾಂಕ>) == ಗಾತ್ರ(Pairing_ರಾಶಿ<ಪೂರ್ಣಾಂಕ>::ಗಂಟು_2*), "Should be a pointer");

#ಆದರೆ_ಅಂತ್ಯ  // Guard
