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
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_cache_ಒಳ_User4;  // Use user4 to cache each V3Hash?
    std::ಗಣ<Ast_ಗಂಟು*> m_ಭೇಟಿಯಾದ;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    ಶೂನ್ಯ guardRecursion(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, std::function<ಶೂನ್ಯ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        ಆದರೆ (!m_cache_ಒಳ_User4) {
            ಆದರೆ (m_ಭೇಟಿಯಾದ.ಕಂಡುಹಿಡಿ(nodep) != m_ಭೇಟಿಯಾದ.ಕೊನೆ()) {
                m_hash += V3Hash{nodep->name()};
                ಹಿಂತಿರುಗಿ;
            }
            m_ಭೇಟಿಯಾದ.ಸೇರಿಸು(nodep);
        }
        f();
    }

    V3Hash hash_ಗಂಟು_And_Iterate(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ hashDType, ತಾರ್ಕಿಕ hashChildren,
                              std::function<ಶೂನ್ಯ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        ಆದರೆ (m_cache_ಒಳ_User4 && nodep->user4()) {
            ಹಿಂತಿರುಗಿ V3Hash{static_cast<uint32_t>(nodep->user4())};
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            ಆದರೆ (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            ಆದರೆ (hashChildren) iterateChildrenConst(nodep);  // Children
            ಆದರೆ (m_cache_ಒಳ_User4) nodep->user4(m_hash.ಮೌಲ್ಯ());
            ಹಿಂತಿರುಗಿ m_hash;
        }
    }

    // VISITORS

    constexpr ಸ್ಥಿರ ತಾರ್ಕಿಕ HASH_DTYPE = ನಿಜ;
    constexpr ಸ್ಥಿರ ತಾರ್ಕಿಕ HASH_CHILDREN = ನಿಜ;

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

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
#ಆದರೆ VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#ಆದರೆ_ಅಂತ್ಯ
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Array_DType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->ಎಡ();
            m_hash += nodep->ಬಲ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, [ಇದು, nodep]() {  //
            m_hash += nodep->unique_ಅಂಕೆ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParamTypeDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefImplicitDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->unique_ಅಂಕೆ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBracketArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDynArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWildcardArrayDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBasicDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().ಎಡ();
            m_hash += nodep->nrange().ಬಲ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassRefDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರತಿ_DType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹರಿವು_DType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVoidDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEnumDType* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, [ಇದು, nodep]() {  //
            m_hash += nodep->unique_ಅಂಕೆ();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಅಗಲ_Const();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಅಂಕೆ().ಗಮ್ಯ_Hash();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNullCheck* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCast* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಗಾತ್ರ_2();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            ಆದರೆ (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_ಸೂಚಕ().asString();
            }
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFScanF* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಪಠ್ಯ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSScanF* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಪಠ್ಯ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Addr_ದ_CFunc* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTriggered* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint32_t ಸೂಚ್ಯಂಕ : nodep->indices()) m_hash += ಸೂಚ್ಯಂಕ;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ವಿಧಾನ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCLocalScope* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverInc* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMonitorOff* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceInc* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Range* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, ಸುಳ್ಳು, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Pre_Sel* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassExtends* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Foreach_ಶೀರ್ಷಿಕೆ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefParam* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArg* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParseRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಅಂಚು_Type();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಪಠ್ಯ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstElabDisplay* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons_ಅಂಶ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ ಸುಳ್ಳು,
                                     [ಇದು, nodep, dtypep]() {
                                         ಸ್ಥಿರಾಂಕ uint32_t ಗಾತ್ರ_2 = dtypep->elementsConst();
                                         ಗಾಗಿ (uint32_t n = 0; n < ಗಾತ್ರ_2; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPragma* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Attr_ದ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_File* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        guardRecursion(nodep, [ಇದು, nodep]() {  //
            m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                ಆದರೆ (m_cache_ಒಳ_User4) nodep->user4(V3Hash{1}.ಮೌಲ್ಯ());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, ಸುಳ್ಳು, [ಇದು, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedefFwd* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellInline* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [ಇದು, nodep]() { m_hash += nodep->name(); });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        guardRecursion(nodep, [ಇದು, nodep]() {  //
            m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                ಆದರೆ (m_cache_ಒಳ_User4) nodep->user4(V3Hash{1}.ಮೌಲ್ಯ());
            });
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModport* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportVarRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportFTaskRef* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->ತಕ್ಷಣ_ಆರಂಭ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ಇದು, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_ಅಂಕೆ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಪಠ್ಯ();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ_ಖಂಡ* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, [ಇದು, nodep]() {  //
            m_hash += nodep->ಪೂರ್ವಪ್ರತ್ಯಯ();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, ಸುಳ್ಳು, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExpr* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        m_hash += hash_ಗಂಟು_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ HasherVisitor(Ast_ಗಂಟು* nodep)
        : m_cache_ಒಳ_User4{ನಿಜ} {
        iterateConst(nodep);
    }
    ವರ್ಗ Uncached {};
    HasherVisitor(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, Uncached)
        : m_cache_ಒಳ_User4{ಸುಳ್ಳು} {
        iterateConst(const_cast<Ast_ಗಂಟು*>(nodep));
    }
    V3Hash finalHash() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hash; }
    ~HasherVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::ಕ್ರಿಯಾಕಾರಕ()(Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!nodep->user4()) HasherVisitor{nodep};
    ಹಿಂತಿರುಗಿ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    ಹಿಂತಿರುಗಿ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಸ್ಥಿರಾಂಕ HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    ಹಿಂತಿರುಗಿ visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

ಗಾತ್ರ_t V3HasherUncachedHash(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು& ಗಂಟು) {
    ಹಿಂತಿರುಗಿ static_cast<ಗಾತ್ರ_t>(V3Hasher::uncachedHash(&ಗಂಟು).ಮೌಲ್ಯ());
}
