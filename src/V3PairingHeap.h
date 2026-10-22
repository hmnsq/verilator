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

#अगर_अघोषित VERILATOR_V3PAIRINGHEAP_H_
#घोषणा VERILATOR_V3PAIRINGHEAP_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"

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

ढाँचा <typename T_कुंजी>
class PairingHeap final {
खुला:
    काष्ठा गाँठ_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    काष्ठा Link final {
        गाँठ_2* m_ptr = nullptr;  // The managed pointer

        Link() = शेष;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE व्योम link(गाँठ_2* targetp) {
            m_ptr = targetp;
            अगर (!targetp) वापस;
#अगर VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#पूर्ण_अगर
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE व्योम linkNonNull(गाँठ_2* targetp) {
            m_ptr = targetp;
#अगर VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#पूर्ण_अगर
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE गाँठ_2* unlink() {
            गाँठ_2* स्थिर परिणाम = m_ptr;
#अगर VL_DEBUG
            अगर (परिणाम) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#पूर्ण_अगर
            m_ptr = nullptr;
            वापस परिणाम;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE गाँठ_2* ptr() स्थिर { वापस m_ptr; }
        VL_ATTR_ALWINLINE चालक bool() स्थिर { वापस m_ptr; }
        VL_ATTR_ALWINLINE bool चालक!() स्थिर { वापस !m_ptr; }
        VL_ATTR_ALWINLINE गाँठ_2* चालक->() स्थिर { वापस m_ptr; }
        VL_ATTR_ALWINLINE गाँठ_2& चालक*() स्थिर { वापस *m_ptr; }
    };

    // A single node in the pairing heap tree
    काष्ठा गाँठ_2 VL_NOT_FINAL {
        Link m_अग्र;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        गाँठ_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_कुंजी m_कुंजी{};  // The key in the heap

        // CONSTRUCTOR
        explicit गाँठ_2() = शेष;
        VL_UNCOPYABLE(गाँठ_2);

        // METHODS
        VL_ATTR_ALWINLINE स्थिर T_कुंजी& कुंजी() स्थिर { वापस m_कुंजी; }
        VL_ATTR_ALWINLINE bool चालक<(स्थिर गाँठ_2& that) स्थिर { वापस m_कुंजी < that.m_कुंजी; }
        VL_ATTR_ALWINLINE bool चालक>(स्थिर गाँठ_2& that) स्थिर { वापस that.m_कुंजी < m_कुंजी; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE व्योम replaceWith(गाँठ_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            अगर (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE व्योम replaceWithNonNull(गाँठ_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE व्योम yank() {
            m_अग्र.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

निजी:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_मूल;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

खुला:
    explicit PairingHeap() = शेष;

    // METHODS
    bool empty() स्थिर { वापस !m_मूल; }

    // Insert given node into this heap with given key.
    व्योम जोड़ो(गाँठ_2* nodep, T_कुंजी कुंजी) {
        // Update key of node
        nodep->m_कुंजी = कुंजी;
        जोड़ो(nodep);
    }

    // Insert given node into this heap with key already set in the node
    व्योम जोड़ो(गाँठ_2* nodep) {
#अगर VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_अग्र && !nodep->m_kids, "Already linked");
#पूर्ण_अगर
        // Just stick it at the front of the root list
        nodep->m_अग्र.link(m_मूल.unlink());
        m_मूल.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    व्योम हटाओ(गाँठ_2* nodep) {
        अगर (!nodep->m_अग्र) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } अन्यथा अगर (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_अग्र.unlink());
        } अन्यथा {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            गाँठ_2* स्थिर reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_अग्र.linkNonNull(nodep->m_अग्र.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    गाँठ_2* max() स्थिर {
        // Heap might be empty
        अगर (!m_मूल) वापस nullptr;
        // If the root have siblings reduce them
        अगर (m_मूल->m_अग्र) m_मूल.linkNonNull(reduce(m_मूल.unlink()));
        // The root element is the largest
        वापस m_मूल.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    गाँठ_2* secondMax() स्थिर {
#अगर VL_DEBUG
        UASSERT(m_मूल, "'max' would have returned nullptr");
        UASSERT(!m_मूल->m_अग्र, "'max' would have reduced");
#पूर्ण_अगर
        // If there are no children, there is no second element
        अगर (!m_मूल->m_kids) वापस nullptr;
        // If there are multiple children, reduce them
        अगर (m_मूल->m_kids->m_अग्र) m_मूल->m_kids.linkNonNull(reduce(m_मूल->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        वापस m_मूल->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    ढाँचा <typename T_Update>
    व्योम increase_कुंजी(गाँठ_2* nodep, T_Update मूल्य) {
        // Update the key
        nodep->m_कुंजी.increase(मूल्य);
        // Increasing the key of the root is easy
        अगर (nodep == m_मूल.ptr()) वापस;
        // Otherwise we do have a little work to do
        अगर (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_अग्र.unlink());
        } अन्यथा अगर (!nodep->m_अग्र) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } अन्यथा {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            गाँठ_2* स्थिर kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_अग्र.unlink());
            kidsp->m_अग्र.linkNonNull(nodep->m_अग्र.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_अग्र.linkNonNull(m_मूल.unlink());
        m_मूल.linkNonNull(nodep);
    }

निजी:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE अटल गाँठ_2* merge(गाँठ_2* ap, गाँठ_2* bp) {
#अगर VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_अग्र, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_अग्र, "Not root b");
#पूर्ण_अगर
        अगर (*ap > *bp) {  // bp goes under ap
            bp->m_अग्र.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            वापस ap;
        }  // else ap goes under bp
        ap->m_अग्र.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        वापस bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE अटल गाँठ_2* reduce(गाँठ_2* nodep) {
#अगर VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#पूर्ण_अगर
        // If there is only one node in the list, then there is nothing to do
        अगर (!nodep->m_अग्र) वापस nodep;
        // The result node
        गाँठ_2* resultp = nullptr;
        // Pairwise merge the child nodes
        जबतक (nodep) {
            // Pop off the first nodes
            गाँठ_2* स्थिर ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            अगर (!nodep->m_अग्र) {
                ap->m_अग्र.link(resultp);
                resultp = ap;
                अवरोध;
            }
            // Pop off the second nodes
            गाँठ_2* स्थिर bp = nodep->m_अग्र.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_अग्र.unlink();
            // Merge the current pair
            गाँठ_2* स्थिर mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_अग्र.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        जबतक (resultp->m_अग्र) {
            // Pop first two results
            गाँठ_2* स्थिर ap = resultp;
            गाँठ_2* स्थिर bp = resultp->m_अग्र.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_अग्र.unlink();
            // Merge the current pair
            गाँठ_2* स्थिर mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_अग्र.link(resultp);
            resultp = mergedp;
        }
        // Done
        वापस resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(माप(PairingHeap<पूर्णांक>) == माप(PairingHeap<पूर्णांक>::गाँठ_2*), "Should be a pointer");

#पूर्ण_अगर  // Guard
