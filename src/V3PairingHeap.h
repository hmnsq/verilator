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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"

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

template <typename T_താക്കോൽ>
class PairingHeap final {
പൊതു:
    ഘടന കണ്ണി_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    ഘടന Link final {
        കണ്ണി_2* m_ptr = nullptr;  // The managed pointer

        Link() = സ്ഥിരസ്ഥിതി;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ശൂന്യം link(കണ്ണി_2* targetp) {
            m_ptr = targetp;
            എങ്കിൽ (!targetp) തിരികെ;
#എങ്കിൽ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ശൂന്യം linkNonNull(കണ്ണി_2* targetp) {
            m_ptr = targetp;
#എങ്കിൽ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#endif
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE കണ്ണി_2* unlink() {
            കണ്ണി_2* സ്ഥിരം result = m_ptr;
#എങ്കിൽ VL_DEBUG
            എങ്കിൽ (result) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#endif
            m_ptr = nullptr;
            തിരികെ result;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE കണ്ണി_2* ptr() സ്ഥിരം { തിരികെ m_ptr; }
        VL_ATTR_ALWINLINE operator bool() സ്ഥിരം { തിരികെ m_ptr; }
        VL_ATTR_ALWINLINE bool operator!() സ്ഥിരം { തിരികെ !m_ptr; }
        VL_ATTR_ALWINLINE കണ്ണി_2* operator->() സ്ഥിരം { തിരികെ m_ptr; }
        VL_ATTR_ALWINLINE കണ്ണി_2& operator*() സ്ഥിരം { തിരികെ *m_ptr; }
    };

    // A single node in the pairing heap tree
    ഘടന കണ്ണി_2 VL_NOT_FINAL {
        Link m_next;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        കണ്ണി_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_താക്കോൽ m_താക്കോൽ{};  // The key in the heap

        // CONSTRUCTOR
        explicit കണ്ണി_2() = സ്ഥിരസ്ഥിതി;
        VL_UNCOPYABLE(കണ്ണി_2);

        // METHODS
        VL_ATTR_ALWINLINE സ്ഥിരം T_താക്കോൽ& താക്കോൽ() സ്ഥിരം { തിരികെ m_താക്കോൽ; }
        VL_ATTR_ALWINLINE bool operator<(സ്ഥിരം കണ്ണി_2& that) സ്ഥിരം { തിരികെ m_താക്കോൽ < that.m_താക്കോൽ; }
        VL_ATTR_ALWINLINE bool operator>(സ്ഥിരം കണ്ണി_2& that) സ്ഥിരം { തിരികെ that.m_താക്കോൽ < m_താക്കോൽ; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ശൂന്യം replaceWith(കണ്ണി_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            എങ്കിൽ (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ശൂന്യം replaceWithNonNull(കണ്ണി_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE ശൂന്യം yank() {
            m_next.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

സ്വകാര്യ:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_വേര്;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

പൊതു:
    explicit PairingHeap() = സ്ഥിരസ്ഥിതി;

    // METHODS
    bool empty() സ്ഥിരം { തിരികെ !m_വേര്; }

    // Insert given node into this heap with given key.
    ശൂന്യം തിരുകുക(കണ്ണി_2* nodep, T_താക്കോൽ താക്കോൽ) {
        // Update key of node
        nodep->m_താക്കോൽ = താക്കോൽ;
        തിരുകുക(nodep);
    }

    // Insert given node into this heap with key already set in the node
    ശൂന്യം തിരുകുക(കണ്ണി_2* nodep) {
#എങ്കിൽ VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_next && !nodep->m_kids, "Already linked");
#endif
        // Just stick it at the front of the root list
        nodep->m_next.link(m_വേര്.unlink());
        m_വേര്.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    ശൂന്യം remove(കണ്ണി_2* nodep) {
        എങ്കിൽ (!nodep->m_next) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_next.unlink());
        } അല്ലെങ്കിൽ {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            കണ്ണി_2* സ്ഥിരം reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    കണ്ണി_2* max() സ്ഥിരം {
        // Heap might be empty
        എങ്കിൽ (!m_വേര്) തിരികെ nullptr;
        // If the root have siblings reduce them
        എങ്കിൽ (m_വേര്->m_next) m_വേര്.linkNonNull(reduce(m_വേര്.unlink()));
        // The root element is the largest
        തിരികെ m_വേര്.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    കണ്ണി_2* secondMax() സ്ഥിരം {
#എങ്കിൽ VL_DEBUG
        UASSERT(m_വേര്, "'max' would have returned nullptr");
        UASSERT(!m_വേര്->m_next, "'max' would have reduced");
#endif
        // If there are no children, there is no second element
        എങ്കിൽ (!m_വേര്->m_kids) തിരികെ nullptr;
        // If there are multiple children, reduce them
        എങ്കിൽ (m_വേര്->m_kids->m_next) m_വേര്->m_kids.linkNonNull(reduce(m_വേര്->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        തിരികെ m_വേര്->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    template <typename T_Update>
    ശൂന്യം increase_താക്കോൽ(കണ്ണി_2* nodep, T_Update മൂല്യം) {
        // Update the key
        nodep->m_താക്കോൽ.increase(മൂല്യം);
        // Increasing the key of the root is easy
        എങ്കിൽ (nodep == m_വേര്.ptr()) തിരികെ;
        // Otherwise we do have a little work to do
        എങ്കിൽ (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_next.unlink());
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->m_next) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } അല്ലെങ്കിൽ {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            കണ്ണി_2* സ്ഥിരം kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_next.unlink());
            kidsp->m_next.linkNonNull(nodep->m_next.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_next.linkNonNull(m_വേര്.unlink());
        m_വേര്.linkNonNull(nodep);
    }

സ്വകാര്യ:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE സ്ഥിതം കണ്ണി_2* merge(കണ്ണി_2* ap, കണ്ണി_2* bp) {
#എങ്കിൽ VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_next, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_next, "Not root b");
#endif
        എങ്കിൽ (*ap > *bp) {  // bp goes under ap
            bp->m_next.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            തിരികെ ap;
        }  // else ap goes under bp
        ap->m_next.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        തിരികെ bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE സ്ഥിതം കണ്ണി_2* reduce(കണ്ണി_2* nodep) {
#എങ്കിൽ VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#endif
        // If there is only one node in the list, then there is nothing to do
        എങ്കിൽ (!nodep->m_next) തിരികെ nodep;
        // The result node
        കണ്ണി_2* resultp = nullptr;
        // Pairwise merge the child nodes
        ആകുമ്പോൾ (nodep) {
            // Pop off the first nodes
            കണ്ണി_2* സ്ഥിരം ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            എങ്കിൽ (!nodep->m_next) {
                ap->m_next.link(resultp);
                resultp = ap;
                നിർത്തുക;
            }
            // Pop off the second nodes
            കണ്ണി_2* സ്ഥിരം bp = nodep->m_next.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_next.unlink();
            // Merge the current pair
            കണ്ണി_2* സ്ഥിരം mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        ആകുമ്പോൾ (resultp->m_next) {
            // Pop first two results
            കണ്ണി_2* സ്ഥിരം ap = resultp;
            കണ്ണി_2* സ്ഥിരം bp = resultp->m_next.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_next.unlink();
            // Merge the current pair
            കണ്ണി_2* സ്ഥിരം mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_next.link(resultp);
            resultp = mergedp;
        }
        // Done
        തിരികെ resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(വലുപ്പം(PairingHeap<പൂർണ്ണസംഖ്യ>) == വലുപ്പം(PairingHeap<പൂർണ്ണസംഖ്യ>::കണ്ണി_2*), "Should be a pointer");

#endif  // Guard
