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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3PAIRINGHEAP_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3PAIRINGHEAP_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

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

ਸਾਂਚਾ <typename T_ਕੁੰਜੀ>
class Pairing_ਢੇਰ final {
ਜਨਤਕ:
    ਬਣਤਰ ਗੰਢ_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    ਬਣਤਰ Link final {
        ਗੰਢ_2* m_ptr = nullptr;  // The managed pointer

        Link() = ਮੂਲ;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ਖਾਲੀ link(ਗੰਢ_2* targetp) {
            m_ptr = targetp;
            ਜੇ (!targetp) ਵਾਪਸ;
#ਜੇ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#ਜੇ_ਅੰਤ
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE ਖਾਲੀ linkNonNull(ਗੰਢ_2* targetp) {
            m_ptr = targetp;
#ਜੇ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#ਜੇ_ਅੰਤ
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE ਗੰਢ_2* unlink() {
            ਗੰਢ_2* ਅਚਲ ਨਤੀਜਾ = m_ptr;
#ਜੇ VL_DEBUG
            ਜੇ (ਨਤੀਜਾ) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#ਜੇ_ਅੰਤ
            m_ptr = nullptr;
            ਵਾਪਸ ਨਤੀਜਾ;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE ਗੰਢ_2* ptr() ਅਚਲ { ਵਾਪਸ m_ptr; }
        VL_ATTR_ALWINLINE ਸੰਚਾਲਕ bool() ਅਚਲ { ਵਾਪਸ m_ptr; }
        VL_ATTR_ALWINLINE bool ਸੰਚਾਲਕ!() ਅਚਲ { ਵਾਪਸ !m_ptr; }
        VL_ATTR_ALWINLINE ਗੰਢ_2* ਸੰਚਾਲਕ->() ਅਚਲ { ਵਾਪਸ m_ptr; }
        VL_ATTR_ALWINLINE ਗੰਢ_2& ਸੰਚਾਲਕ*() ਅਚਲ { ਵਾਪਸ *m_ptr; }
    };

    // A single node in the pairing heap tree
    ਬਣਤਰ ਗੰਢ_2 VL_NOT_FINAL {
        Link m_ਅਗਲਾ;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        ਗੰਢ_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_ਕੁੰਜੀ m_ਕੁੰਜੀ{};  // The key in the heap

        // CONSTRUCTOR
        explicit ਗੰਢ_2() = ਮੂਲ;
        VL_UNCOPYABLE(ਗੰਢ_2);

        // METHODS
        VL_ATTR_ALWINLINE ਅਚਲ T_ਕੁੰਜੀ& ਕੁੰਜੀ() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀ; }
        VL_ATTR_ALWINLINE bool ਸੰਚਾਲਕ<(ਅਚਲ ਗੰਢ_2& that) ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀ < that.m_ਕੁੰਜੀ; }
        VL_ATTR_ALWINLINE bool ਸੰਚਾਲਕ>(ਅਚਲ ਗੰਢ_2& that) ਅਚਲ { ਵਾਪਸ that.m_ਕੁੰਜੀ < m_ਕੁੰਜੀ; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ਖਾਲੀ replaceWith(ਗੰਢ_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            ਜੇ (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE ਖਾਲੀ replaceWithNonNull(ਗੰਢ_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE ਖਾਲੀ yank() {
            m_ਅਗਲਾ.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

ਨਿਜੀ:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_ਜੜ੍ਹ;

    // CONSTRUCTORS
    VL_UNCOPYABLE(Pairing_ਢੇਰ);

ਜਨਤਕ:
    explicit Pairing_ਢੇਰ() = ਮੂਲ;

    // METHODS
    bool ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ !m_ਜੜ੍ਹ; }

    // Insert given node into this heap with given key.
    ਖਾਲੀ ਪਾਓ(ਗੰਢ_2* nodep, T_ਕੁੰਜੀ ਕੁੰਜੀ) {
        // Update key of node
        nodep->m_ਕੁੰਜੀ = ਕੁੰਜੀ;
        ਪਾਓ(nodep);
    }

    // Insert given node into this heap with key already set in the node
    ਖਾਲੀ ਪਾਓ(ਗੰਢ_2* nodep) {
#ਜੇ VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_ਅਗਲਾ && !nodep->m_kids, "Already linked");
#ਜੇ_ਅੰਤ
        // Just stick it at the front of the root list
        nodep->m_ਅਗਲਾ.link(m_ਜੜ੍ਹ.unlink());
        m_ਜੜ੍ਹ.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    ਖਾਲੀ remove(ਗੰਢ_2* nodep) {
        ਜੇ (!nodep->m_ਅਗਲਾ) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_ਅਗਲਾ.unlink());
        } ਨਹੀਂਤਾਂ {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            ਗੰਢ_2* ਅਚਲ reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_ਅਗਲਾ.linkNonNull(nodep->m_ਅਗਲਾ.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    ਗੰਢ_2* max() ਅਚਲ {
        // Heap might be empty
        ਜੇ (!m_ਜੜ੍ਹ) ਵਾਪਸ nullptr;
        // If the root have siblings reduce them
        ਜੇ (m_ਜੜ੍ਹ->m_ਅਗਲਾ) m_ਜੜ੍ਹ.linkNonNull(reduce(m_ਜੜ੍ਹ.unlink()));
        // The root element is the largest
        ਵਾਪਸ m_ਜੜ੍ਹ.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    ਗੰਢ_2* ਦੂਜਾ_Max() ਅਚਲ {
#ਜੇ VL_DEBUG
        UASSERT(m_ਜੜ੍ਹ, "'max' would have returned nullptr");
        UASSERT(!m_ਜੜ੍ਹ->m_ਅਗਲਾ, "'max' would have reduced");
#ਜੇ_ਅੰਤ
        // If there are no children, there is no second element
        ਜੇ (!m_ਜੜ੍ਹ->m_kids) ਵਾਪਸ nullptr;
        // If there are multiple children, reduce them
        ਜੇ (m_ਜੜ੍ਹ->m_kids->m_ਅਗਲਾ) m_ਜੜ੍ਹ->m_kids.linkNonNull(reduce(m_ਜੜ੍ਹ->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        ਵਾਪਸ m_ਜੜ੍ਹ->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    ਸਾਂਚਾ <typename T_Update>
    ਖਾਲੀ increase_ਕੁੰਜੀ(ਗੰਢ_2* nodep, T_Update ਮੁੱਲ) {
        // Update the key
        nodep->m_ਕੁੰਜੀ.increase(ਮੁੱਲ);
        // Increasing the key of the root is easy
        ਜੇ (nodep == m_ਜੜ੍ਹ.ptr()) ਵਾਪਸ;
        // Otherwise we do have a little work to do
        ਜੇ (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_ਅਗਲਾ.unlink());
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->m_ਅਗਲਾ) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } ਨਹੀਂਤਾਂ {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            ਗੰਢ_2* ਅਚਲ kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_ਅਗਲਾ.unlink());
            kidsp->m_ਅਗਲਾ.linkNonNull(nodep->m_ਅਗਲਾ.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_ਅਗਲਾ.linkNonNull(m_ਜੜ੍ਹ.unlink());
        m_ਜੜ੍ਹ.linkNonNull(nodep);
    }

ਨਿਜੀ:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE ਸਥਿਰ ਗੰਢ_2* ਮਿਲਾਓ(ਗੰਢ_2* ap, ਗੰਢ_2* bp) {
#ਜੇ VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_ਅਗਲਾ, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_ਅਗਲਾ, "Not root b");
#ਜੇ_ਅੰਤ
        ਜੇ (*ap > *bp) {  // bp goes under ap
            bp->m_ਅਗਲਾ.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            ਵਾਪਸ ap;
        }  // else ap goes under bp
        ap->m_ਅਗਲਾ.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        ਵਾਪਸ bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE ਸਥਿਰ ਗੰਢ_2* reduce(ਗੰਢ_2* nodep) {
#ਜੇ VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#ਜੇ_ਅੰਤ
        // If there is only one node in the list, then there is nothing to do
        ਜੇ (!nodep->m_ਅਗਲਾ) ਵਾਪਸ nodep;
        // The result node
        ਗੰਢ_2* resultp = nullptr;
        // Pairwise merge the child nodes
        ਜਦੋਂ (nodep) {
            // Pop off the first nodes
            ਗੰਢ_2* ਅਚਲ ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            ਜੇ (!nodep->m_ਅਗਲਾ) {
                ap->m_ਅਗਲਾ.link(resultp);
                resultp = ap;
                ਰੋਕੋ;
            }
            // Pop off the second nodes
            ਗੰਢ_2* ਅਚਲ bp = nodep->m_ਅਗਲਾ.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_ਅਗਲਾ.unlink();
            // Merge the current pair
            ਗੰਢ_2* ਅਚਲ mergedp = ਮਿਲਾਓ(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_ਅਗਲਾ.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        ਜਦੋਂ (resultp->m_ਅਗਲਾ) {
            // Pop first two results
            ਗੰਢ_2* ਅਚਲ ap = resultp;
            ਗੰਢ_2* ਅਚਲ bp = resultp->m_ਅਗਲਾ.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_ਅਗਲਾ.unlink();
            // Merge the current pair
            ਗੰਢ_2* ਅਚਲ mergedp = ਮਿਲਾਓ(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_ਅਗਲਾ.link(resultp);
            resultp = mergedp;
        }
        //ਪੂਰਾ
        ਵਾਪਸ resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(ਆਕਾਰ(Pairing_ਢੇਰ<ਪੂਰਨ>) == ਆਕਾਰ(Pairing_ਢੇਰ<ਪੂਰਨ>::ਗੰਢ_2*), "Should be a pointer");

#ਜੇ_ਅੰਤ  // Guard
