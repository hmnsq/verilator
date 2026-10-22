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

#यदि_अपरिभाषितम् VERILATOR_V3PAIRINGHEAP_H_
#परिभाषय VERILATOR_V3PAIRINGHEAP_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"

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

प्रतिरूपम् <typename T_कुञ्चिका>
class PairingHeap final {
सार्वजनिक:
    संरचना ग्रन्थिः_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    संरचना Link final {
        ग्रन्थिः_2* m_ptr = nullptr;  // The managed pointer

        Link() = स्वभाव;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE शून्य link(ग्रन्थिः_2* targetp) {
            m_ptr = targetp;
            यदि (!targetp) प्रत्यागम;
#यदि VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#यदि_अन्तः
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE शून्य linkNonNull(ग्रन्थिः_2* targetp) {
            m_ptr = targetp;
#यदि VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#यदि_अन्तः
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE ग्रन्थिः_2* unlink() {
            ग्रन्थिः_2* ध्रुव परिणाम = m_ptr;
#यदि VL_DEBUG
            यदि (परिणाम) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#यदि_अन्तः
            m_ptr = nullptr;
            प्रत्यागम परिणाम;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE ग्रन्थिः_2* ptr() ध्रुव { प्रत्यागम m_ptr; }
        VL_ATTR_ALWINLINE संक्रियकः bool() ध्रुव { प्रत्यागम m_ptr; }
        VL_ATTR_ALWINLINE bool संक्रियकः!() ध्रुव { प्रत्यागम !m_ptr; }
        VL_ATTR_ALWINLINE ग्रन्थिः_2* संक्रियकः->() ध्रुव { प्रत्यागम m_ptr; }
        VL_ATTR_ALWINLINE ग्रन्थिः_2& संक्रियकः*() ध्रुव { प्रत्यागम *m_ptr; }
    };

    // A single node in the pairing heap tree
    संरचना ग्रन्थिः_2 VL_NOT_FINAL {
        Link m_अग्र;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        ग्रन्थिः_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_कुञ्चिका m_कुञ्चिका{};  // The key in the heap

        // CONSTRUCTOR
        explicit ग्रन्थिः_2() = स्वभाव;
        VL_UNCOPYABLE(ग्रन्थिः_2);

        // METHODS
        VL_ATTR_ALWINLINE ध्रुव T_कुञ्चिका& कुञ्चिका() ध्रुव { प्रत्यागम m_कुञ्चिका; }
        VL_ATTR_ALWINLINE bool संक्रियकः<(ध्रुव ग्रन्थिः_2& that) ध्रुव { प्रत्यागम m_कुञ्चिका < that.m_कुञ्चिका; }
        VL_ATTR_ALWINLINE bool संक्रियकः>(ध्रुव ग्रन्थिः_2& that) ध्रुव { प्रत्यागम that.m_कुञ्चिका < m_कुञ्चिका; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE शून्य replaceWith(ग्रन्थिः_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            यदि (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE शून्य replaceWithNonNull(ग्रन्थिः_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE शून्य yank() {
            m_अग्र.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

गुप्त:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_मूलम्;

    // CONSTRUCTORS
    VL_UNCOPYABLE(PairingHeap);

सार्वजनिक:
    explicit PairingHeap() = स्वभाव;

    // METHODS
    bool रिक्तानि() ध्रुव { प्रत्यागम !m_मूलम्; }

    // Insert given node into this heap with given key.
    शून्य निवेशय(ग्रन्थिः_2* nodep, T_कुञ्चिका कुञ्चिका) {
        // Update key of node
        nodep->m_कुञ्चिका = कुञ्चिका;
        निवेशय(nodep);
    }

    // Insert given node into this heap with key already set in the node
    शून्य निवेशय(ग्रन्थिः_2* nodep) {
#यदि VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_अग्र && !nodep->m_kids, "Already linked");
#यदि_अन्तः
        // Just stick it at the front of the root list
        nodep->m_अग्र.link(m_मूलम्.unlink());
        m_मूलम्.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    शून्य remove(ग्रन्थिः_2* nodep) {
        यदि (!nodep->m_अग्र) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } अन्यथा यदि (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_अग्र.unlink());
        } अन्यथा {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            ग्रन्थिः_2* ध्रुव reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_अग्र.linkNonNull(nodep->m_अग्र.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    ग्रन्थिः_2* max() ध्रुव {
        // Heap might be empty
        यदि (!m_मूलम्) प्रत्यागम nullptr;
        // If the root have siblings reduce them
        यदि (m_मूलम्->m_अग्र) m_मूलम्.linkNonNull(reduce(m_मूलम्.unlink()));
        // The root element is the largest
        प्रत्यागम m_मूलम्.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    ग्रन्थिः_2* secondMax() ध्रुव {
#यदि VL_DEBUG
        UASSERT(m_मूलम्, "'max' would have returned nullptr");
        UASSERT(!m_मूलम्->m_अग्र, "'max' would have reduced");
#यदि_अन्तः
        // If there are no children, there is no second element
        यदि (!m_मूलम्->m_kids) प्रत्यागम nullptr;
        // If there are multiple children, reduce them
        यदि (m_मूलम्->m_kids->m_अग्र) m_मूलम्->m_kids.linkNonNull(reduce(m_मूलम्->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        प्रत्यागम m_मूलम्->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    प्रतिरूपम् <typename T_Update>
    शून्य increase_कुञ्चिका(ग्रन्थिः_2* nodep, T_Update मानम्) {
        // Update the key
        nodep->m_कुञ्चिका.increase(मानम्);
        // Increasing the key of the root is easy
        यदि (nodep == m_मूलम्.ptr()) प्रत्यागम;
        // Otherwise we do have a little work to do
        यदि (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_अग्र.unlink());
        } अन्यथा यदि (!nodep->m_अग्र) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } अन्यथा {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            ग्रन्थिः_2* ध्रुव kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_अग्र.unlink());
            kidsp->m_अग्र.linkNonNull(nodep->m_अग्र.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_अग्र.linkNonNull(m_मूलम्.unlink());
        m_मूलम्.linkNonNull(nodep);
    }

गुप्त:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE स्थिर ग्रन्थिः_2* merge(ग्रन्थिः_2* ap, ग्रन्थिः_2* bp) {
#यदि VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_अग्र, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_अग्र, "Not root b");
#यदि_अन्तः
        यदि (*ap > *bp) {  // bp goes under ap
            bp->m_अग्र.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            प्रत्यागम ap;
        }  // else ap goes under bp
        ap->m_अग्र.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        प्रत्यागम bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE स्थिर ग्रन्थिः_2* reduce(ग्रन्थिः_2* nodep) {
#यदि VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#यदि_अन्तः
        // If there is only one node in the list, then there is nothing to do
        यदि (!nodep->m_अग्र) प्रत्यागम nodep;
        // The result node
        ग्रन्थिः_2* resultp = nullptr;
        // Pairwise merge the child nodes
        यावत् (nodep) {
            // Pop off the first nodes
            ग्रन्थिः_2* ध्रुव ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            यदि (!nodep->m_अग्र) {
                ap->m_अग्र.link(resultp);
                resultp = ap;
                भङ्ग;
            }
            // Pop off the second nodes
            ग्रन्थिः_2* ध्रुव bp = nodep->m_अग्र.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_अग्र.unlink();
            // Merge the current pair
            ग्रन्थिः_2* ध्रुव mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_अग्र.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        यावत् (resultp->m_अग्र) {
            // Pop first two results
            ग्रन्थिः_2* ध्रुव ap = resultp;
            ग्रन्थिः_2* ध्रुव bp = resultp->m_अग्र.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_अग्र.unlink();
            // Merge the current pair
            ग्रन्थिः_2* ध्रुव mergedp = merge(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_अग्र.link(resultp);
            resultp = mergedp;
        }
        //समाप्तम्
        प्रत्यागम resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(परिमाण(PairingHeap<पूर्णाङ्क>) == परिमाण(PairingHeap<पूर्णाङ्क>::ग्रन्थिः_2*), "Should be a pointer");

#यदि_अन्तः  // Guard
