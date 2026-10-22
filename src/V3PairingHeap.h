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

#నిర్వచితం_కాకపోతే VERILATOR_V3PAIRINGHEAP_H_
#నిర్వచించు VERILATOR_V3PAIRINGHEAP_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"

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

మూస <typename T_కీలకం>
class Pairing_కుప్ప final {
బహిరంగ:
    నిర్మాణం కణుపు_2;

    // Just a pointer to a heap Node, but with special accessors to help keep back pointers
    // consistent.
    నిర్మాణం Link final {
        కణుపు_2* m_ptr = nullptr;  // The managed pointer

        Link() = అప్రమేయ;
        VL_UNCOPYABLE(Link);

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE శూన్య link(కణుపు_2* targetp) {
            m_ptr = targetp;
            ఒకవేళ (!targetp) తిరిగి;
#ఒకవేళ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#అయితే_ముగింపు
            targetp->m_ownerpp = &m_ptr;
        }

        // Make the pointer point to the target, and the target's owner pointer to this pointer
        VL_ATTR_ALWINLINE శూన్య linkNonNull(కణుపు_2* targetp) {
            m_ptr = targetp;
#ఒకవేళ VL_DEBUG
            UASSERT(!targetp->m_ownerpp, "Already linked");
#అయితే_ముగింపు
            targetp->m_ownerpp = &m_ptr;
        }

        // Clear the pointer and return it's previous value
        VL_ATTR_ALWINLINE కణుపు_2* unlink() {
            కణుపు_2* స్థిరరాశి ఫలితం = m_ptr;
#ఒకవేళ VL_DEBUG
            ఒకవేళ (ఫలితం) {
                UASSERT(m_ptr->m_ownerpp == &m_ptr, "Bad back link");
                // Not strictly necessary to clear this, but helps debugging
                m_ptr->m_ownerpp = nullptr;
            }
#అయితే_ముగింపు
            m_ptr = nullptr;
            తిరిగి ఫలితం;
        }

        // Minimal convenience accessors and operators
        VL_ATTR_ALWINLINE కణుపు_2* ptr() స్థిరరాశి { తిరిగి m_ptr; }
        VL_ATTR_ALWINLINE క్రియాకారకం bool() స్థిరరాశి { తిరిగి m_ptr; }
        VL_ATTR_ALWINLINE bool క్రియాకారకం!() స్థిరరాశి { తిరిగి !m_ptr; }
        VL_ATTR_ALWINLINE కణుపు_2* క్రియాకారకం->() స్థిరరాశి { తిరిగి m_ptr; }
        VL_ATTR_ALWINLINE కణుపు_2& క్రియాకారకం*() స్థిరరాశి { తిరిగి *m_ptr; }
    };

    // A single node in the pairing heap tree
    నిర్మాణం కణుపు_2 VL_NOT_FINAL {
        Link m_తదుపరి;  // Next in list of sibling heaps
        Link m_kids;  // Head of list of child heaps
        కణుపు_2** m_ownerpp = nullptr;  // Pointer to the Link pointer pointing to this heap
        T_కీలకం m_కీలకం{};  // The key in the heap

        // CONSTRUCTOR
        explicit కణుపు_2() = అప్రమేయ;
        VL_UNCOPYABLE(కణుపు_2);

        // METHODS
        VL_ATTR_ALWINLINE స్థిరరాశి T_కీలకం& కీలకం() స్థిరరాశి { తిరిగి m_కీలకం; }
        VL_ATTR_ALWINLINE bool క్రియాకారకం<(స్థిరరాశి కణుపు_2& that) స్థిరరాశి { తిరిగి m_కీలకం < that.m_కీలకం; }
        VL_ATTR_ALWINLINE bool క్రియాకారకం>(స్థిరరాశి కణుపు_2& that) స్థిరరాశి { తిరిగి that.m_కీలకం < m_కీలకం; }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE శూన్య replaceWith(కణుపు_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            ఒకవేళ (newp) newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Make newp take the place of this in the tree
        VL_ATTR_ALWINLINE శూన్య replaceWithNonNull(కణుపు_2* newp) {
            *m_ownerpp = newp;  // The owner pointer needs to point to the new node
            newp->m_ownerpp = m_ownerpp;  // The new node needs to point to its owner
            m_ownerpp = nullptr;  // This node has no owner anymore
        }

        // Yank this node out of the heap it currently is in. This node can then be safely inserted
        // into another heap. Note that this leaves the heap the node is currently under in an
        // inconsistent state, so you cannot access it anymore. Still this can save a remove if we
        // don't care about the state of the source heap.
        VL_ATTR_ALWINLINE శూన్య yank() {
            m_తదుపరి.link(nullptr);
            m_kids.link(nullptr);
            m_ownerpp = nullptr;
        }
    };

అంతరంగ:
    // MEMBERS

    // The root of the heap. Note: We do not reduce lists during insertion/removal etc, unless we
    // absolutely have to. This means the root can become a list. This is ok, we will reduce
    // lazily when requesting the minimum element.
    mutable Link m_మూలం;

    // CONSTRUCTORS
    VL_UNCOPYABLE(Pairing_కుప్ప);

బహిరంగ:
    explicit Pairing_కుప్ప() = అప్రమేయ;

    // METHODS
    bool ఖాళీలు() స్థిరరాశి { తిరిగి !m_మూలం; }

    // Insert given node into this heap with given key.
    శూన్య చొప్పించు(కణుపు_2* nodep, T_కీలకం కీలకం) {
        // Update key of node
        nodep->m_కీలకం = కీలకం;
        చొప్పించు(nodep);
    }

    // Insert given node into this heap with key already set in the node
    శూన్య చొప్పించు(కణుపు_2* nodep) {
#ఒకవేళ VL_DEBUG
        UASSERT(!nodep->m_ownerpp && !nodep->m_తదుపరి && !nodep->m_kids, "Already linked");
#అయితే_ముగింపు
        // Just stick it at the front of the root list
        nodep->m_తదుపరి.link(m_మూలం.unlink());
        m_మూలం.linkNonNull(nodep);
    }

    // Remove given node only from the heap it is contained in
    శూన్య remove(కణుపు_2* nodep) {
        ఒకవేళ (!nodep->m_తదుపరి) {
            // If the node does not have siblings, replace it with its children (might be empty).
            nodep->replaceWith(nodep->m_kids.unlink());
        } లేకపోతే ఒకవేళ (!nodep->m_kids) {
            // If it has siblings but no children, replace it with the siblings.
            nodep->replaceWithNonNull(nodep->m_తదుపరి.unlink());
        } లేకపోతే {
            // If it has both siblings and children, reduce the children and splice that
            // reduced heap in place of this node
            కణుపు_2* స్థిరరాశి reducedKidsp = reduce(nodep->m_kids.unlink());
            reducedKidsp->m_తదుపరి.linkNonNull(nodep->m_తదుపరి.unlink());
            nodep->replaceWithNonNull(reducedKidsp);
        }
    }

    // Returns the largest element in the heap
    కణుపు_2* max() స్థిరరాశి {
        // Heap might be empty
        ఒకవేళ (!m_మూలం) తిరిగి nullptr;
        // If the root have siblings reduce them
        ఒకవేళ (m_మూలం->m_తదుపరి) m_మూలం.linkNonNull(reduce(m_మూలం.unlink()));
        // The root element is the largest
        తిరిగి m_మూలం.ptr();
    }

    // Returns the second-largest element in the heap.
    // This is only valid to call if 'max' returned a valid element.
    కణుపు_2* రెండవ_Max() స్థిరరాశి {
#ఒకవేళ VL_DEBUG
        UASSERT(m_మూలం, "'max' would have returned nullptr");
        UASSERT(!m_మూలం->m_తదుపరి, "'max' would have reduced");
#అయితే_ముగింపు
        // If there are no children, there is no second element
        ఒకవేళ (!m_మూలం->m_kids) తిరిగి nullptr;
        // If there are multiple children, reduce them
        ఒకవేళ (m_మూలం->m_kids->m_తదుపరి) m_మూలం->m_kids.linkNonNull(reduce(m_మూలం->m_kids.unlink()));
        // Return the now singular child, which is the second-largest element
        తిరిగి m_మూలం->m_kids.ptr();
    }

    // Increase the key of the given node to the given new value
    మూస <typename T_Update>
    శూన్య increase_కీలకం(కణుపు_2* nodep, T_Update విలువ) {
        // Update the key
        nodep->m_కీలకం.increase(విలువ);
        // Increasing the key of the root is easy
        ఒకవేళ (nodep == m_మూలం.ptr()) తిరిగి;
        // Otherwise we do have a little work to do
        ఒకవేళ (!nodep->m_kids) {
            // If the node has no children, replace it with its siblings (might be null)
            nodep->replaceWith(nodep->m_తదుపరి.unlink());
        } లేకపోతే ఒకవేళ (!nodep->m_తదుపరి) {
            // If the node has no siblings, replace it with its children
            nodep->replaceWithNonNull(nodep->m_kids.unlink());
        } లేకపోతే {
            // The node has both children and siblings. Splice the first child in the place of the
            // node, and extract the rest of the children with the node
            కణుపు_2* స్థిరరాశి kidsp = nodep->m_kids.unlink();
            nodep->m_kids.link(kidsp->m_తదుపరి.unlink());
            kidsp->m_తదుపరి.linkNonNull(nodep->m_తదుపరి.unlink());
            nodep->replaceWithNonNull(kidsp);
        }
        // Just stick the increased node at the front of the root list
        nodep->m_తదుపరి.linkNonNull(m_మూలం.unlink());
        m_మూలం.linkNonNull(nodep);
    }

అంతరంగ:
    // Meld (merge) two heaps rooted at the given nodes, return the root of the new heap
    VL_ATTR_ALWINLINE స్థిర కణుపు_2* విలీనం(కణుపు_2* ap, కణుపు_2* bp) {
#ఒకవేళ VL_DEBUG
        UASSERT(!ap->m_ownerpp && !ap->m_తదుపరి, "Not root a");
        UASSERT(!bp->m_ownerpp && !bp->m_తదుపరి, "Not root b");
#అయితే_ముగింపు
        ఒకవేళ (*ap > *bp) {  // bp goes under ap
            bp->m_తదుపరి.link(ap->m_kids.unlink());
            ap->m_kids.linkNonNull(bp);
            తిరిగి ap;
        }  // else ap goes under bp
        ap->m_తదుపరి.link(bp->m_kids.unlink());
        bp->m_kids.linkNonNull(ap);
        తిరిగి bp;
    }

    // Reduces the list of nodes starting at the given node into a single node that is returned
    VL_ATTR_NOINLINE స్థిర కణుపు_2* reduce(కణుపు_2* nodep) {
#ఒకవేళ VL_DEBUG
        UASSERT(!nodep->m_ownerpp, "Node is linked");
#అయితే_ముగింపు
        // If there is only one node in the list, then there is nothing to do
        ఒకవేళ (!nodep->m_తదుపరి) తిరిగి nodep;
        // The result node
        కణుపు_2* resultp = nullptr;
        // Pairwise merge the child nodes
        అయితే (nodep) {
            // Pop off the first nodes
            కణుపు_2* స్థిరరాశి ap = nodep;
            // If we have an odd number of nodes, prepend the unpaired one onto the result list
            ఒకవేళ (!nodep->m_తదుపరి) {
                ap->m_తదుపరి.link(resultp);
                resultp = ap;
                ఆపు;
            }
            // Pop off the second nodes
            కణుపు_2* స్థిరరాశి bp = nodep->m_తదుపరి.unlink();
            // Keep hold of the rest of the list
            nodep = bp->m_తదుపరి.unlink();
            // Merge the current pair
            కణుపు_2* స్థిరరాశి mergedp = విలీనం(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_తదుపరి.link(resultp);
            resultp = mergedp;
        }
        // Now merge-reduce the merged pairs
        అయితే (resultp->m_తదుపరి) {
            // Pop first two results
            కణుపు_2* స్థిరరాశి ap = resultp;
            కణుపు_2* స్థిరరాశి bp = resultp->m_తదుపరి.unlink();
            // Keep hold of the rest of the list
            resultp = bp->m_తదుపరి.unlink();
            // Merge the current pair
            కణుపు_2* స్థిరరాశి mergedp = విలీనం(ap, bp);
            // Prepend the merged pair to the result list
            mergedp->m_తదుపరి.link(resultp);
            resultp = mergedp;
        }
        //పూర్తి
        తిరిగి resultp;
    }
};

// The PairingHeap itself should be a simple pointer and nothing more
static_assert(పరిమాణం(Pairing_కుప్ప<పూర్ణాంకం>) == పరిమాణం(Pairing_కుప్ప<పూర్ణాంకం>::కణుపు_2*), "Should be a pointer");

#అయితే_ముగింపు  // Guard
