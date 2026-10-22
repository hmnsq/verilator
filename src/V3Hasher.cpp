// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode hash computation
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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Hasher.h"

#ಅನ್ತರ್ಭಾವ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

ವರ್ಗ HasherVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ಧ್ರುವ ತಾರ್ಕಿಕ m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_ಗಂಟು*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    ಶೂನ್ಯ guardRecursion(Ast_ಗಂಟು* ಧ್ರುವ nodep, std::function<ಶೂನ್ಯ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        ಯದಿ (!m_cacheInUser4) {
            ಯದಿ (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                ಪ್ರತ್ಯಾಗಮ;
            }
            m_visited.ಸೇರಿಸು(nodep);
        }
        f();
    }

    V3Hash hash_ಗಂಟು_And_Iterate(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ hashDType, ತಾರ್ಕಿಕ hashChildren,
                              std::function<ಶೂನ್ಯ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        ಯದಿ (m_cacheInUser4 && nodep->user4()) {
            ಪ್ರತ್ಯಾಗಮ V3Hash{static_cast<uint32_t>(nodep->user4())};
        } ಅನ್ಯಥಾ {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            ಯದಿ (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            ಯದಿ (hashChildren) iterateChildrenConst(nodep);  // Children
            ಯದಿ (m_cacheInUser4) nodep->user4(m_hash.ಮೌಲ್ಯ());
            ಪ್ರತ್ಯಾಗಮ m_hash;
        }
    }

    // VISITORS

    constexpr ಸ್ಥಿರ ತಾರ್ಕಿಕ HASH_DTYPE = ಸತ್ಯ;
    constexpr ಸ್ಥಿರ ತಾರ್ಕಿಕ HASH_CHILDREN = ಸತ್ಯ;

    // Each visitor below contributes to the hash any node-specific content
    // that is not dependent on either of the following, as these are
    // included by default by hashNode:
    // - Node type (as given by AstNode::type().hash())
    // - Node dtype (unless !hashDType)
    // - child nodes (unless !hashChildren)
    //
    // The hash must be stable, which means in particular it cannot rely on
    // pointer values, or any other value that might differ between separate
    // invocations of Verilator over the same design.
    //
    // Note there is a circularity problem where some child nodes can back
    // to their ancestral nodes via member pointers, which can lead to an
    // infinite traversal. To break this, nodes that are subject to such
    // referencing and represent code which can reasonably be assumed not to
    // be equivalent to any other code, are hashed either by name (e.g.:
    // AstNodeModule), or by unique identifier (e.g.: AstNodeUOrStructDType).

    //------------------------------------------------------------
    // AstNode - Warns to help find missing cases

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
#ಯದಿ VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Array_DType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->ಎಡ();
            m_hash += nodep->ಬಲ();
        });
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, [ಏತತ್, nodep]() {  //
            m_hash += nodep->unique_ಅಂಕೆ();
        });
    }
    ಶೂನ್ಯ visit(AstParamTypeDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ಶೂನ್ಯ visit(AstMemberDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->unique_ಅಂಕೆ();
        });
    }
    ಶೂನ್ಯ visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    ಶೂನ್ಯ visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ visit(AstDynArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ visit(AstBasicDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().ಎಡ();
            m_hash += nodep->nrange().ಬಲ();
        });
    }
    ಶೂನ್ಯ visit(AstCDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstConstDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ visit(AstClassRefDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    ಶೂನ್ಯ visit(AstQueueDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    ಶೂನ್ಯ visit(AstStreamDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstVoidDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstEnumDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, [ಏತತ್, nodep]() {  //
            m_hash += nodep->unique_ಅಂಕೆ();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->ಅಂಕೆ().toHash();
        });
    }
    ಶೂನ್ಯ visit(AstNullCheck* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstCCast* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            ಯದಿ (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } ಅನ್ಯಥಾ {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    ಶೂನ್ಯ visit(AstVarXRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstFScanF* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ಶೂನ್ಯ visit(AstSScanF* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ಶೂನ್ಯ visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ಶೂನ್ಯ visit(AstTriggered* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            ಪ್ರತಿ (ಧ್ರುವ uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ಶೂನ್ಯ visit(AstCLocalScope* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstCoverInc* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ಶೂನ್ಯ visit(AstMonitorOff* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    ಶೂನ್ಯ visit(AstJumpGo* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, []() {});
    }
    ಶೂನ್ಯ visit(AstTraceInc* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Range* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, ಅಸತ್ಯ, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Pre_Sel* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstClassExtends* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstForeachHeader* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstDefParam* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstArg* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstParseRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ಶೂನ್ಯ visit(AstElabDisplay* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons_Item* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ ಅಸತ್ಯ,
                                     [ಏತತ್, nodep, dtypep]() {
                                         ಧ್ರುವ uint32_t size = dtypep->elementsConst();
                                         ಪ್ರತಿ (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    ಶೂನ್ಯ visit(AstPragma* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    ಶೂನ್ಯ visit(AstAttrOf* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_File* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [ಏತತ್, nodep]() {  //
            m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                ಯದಿ (m_cacheInUser4) nodep->user4(V3Hash{1}.ಮೌಲ್ಯ());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, ಅಸತ್ಯ, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    ಶೂನ್ಯ visit(AstEnumItem* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstTypedefFwd* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    ಶೂನ್ಯ visit(AstCellInline* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [ಏತತ್, nodep]() { m_hash += nodep->name(); });
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        guardRecursion(nodep, [ಏತತ್, nodep]() {  //
            m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                ಯದಿ (m_cacheInUser4) nodep->user4(V3Hash{1}.ಮೌಲ್ಯ());
            });
        });
    }
    ಶೂನ್ಯ visit(AstModport* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstModportVarRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    ಶೂನ್ಯ visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    ಶೂನ್ಯ visit(AstPin* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಏತತ್, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_ಅಂಕೆ();
        });
    }
    ಶೂನ್ಯ visit(AstText* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ಶೂನ್ಯ visit(AstTextBlock* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, [ಏತತ್, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    ಶೂನ್ಯ visit(AstCStmt* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstCStmtUser* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಅಸತ್ಯ, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstCExpr* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_ಗಂಟು* nodep)
        : m_cacheInUser4{ಸತ್ಯ} {
        iterateConst(nodep);
    }
    ವರ್ಗ Uncached {};
    HasherVisitor(ಧ್ರುವ Ast_ಗಂಟು* nodep, Uncached)
        : m_cacheInUser4{ಅಸತ್ಯ} {
        iterateConst(const_cast<Ast_ಗಂಟು*>(nodep));
    }
    V3Hash finalHash() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hash; }
    ~HasherVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_ಗಂಟು* nodep) ಧ್ರುವ {
    ಯದಿ (!nodep->user4()) HasherVisitor{nodep};
    ಪ್ರತ್ಯಾಗಮ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_ಗಂಟು* nodep) ಧ್ರುವ {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    ಪ್ರತ್ಯಾಗಮ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
    ಧ್ರುವ HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    ಪ್ರತ್ಯಾಗಮ visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

size_t V3HasherUncachedHash(ಧ್ರುವ Ast_ಗಂಟು& ಗಂಟು) {
    ಪ್ರತ್ಯಾಗಮ static_cast<size_t>(V3Hasher::uncachedHash(&ಗಂಟು).ಮೌಲ್ಯ());
}
