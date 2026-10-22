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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3Hasher.h"

#অন্তর্ভাব <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

শ্রেণী HasherVisitor final : সর্বজনীন VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ধ্রুবক যৌক্তিক m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_গ্রন্থি*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    শূন্য guardRecursion(Ast_গ্রন্থি* ধ্রুবক nodep, std::function<শূন্য()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        যদি (!m_cacheInUser4) {
            যদি (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                ফেরত;
            }
            m_visited.ঢোকাও(nodep);
        }
        f();
    }

    V3Hash hash_গ্রন্থি_And_Iterate(Ast_গ্রন্থি* nodep, যৌক্তিক hashDType, যৌক্তিক hashChildren,
                              std::function<শূন্য()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        যদি (m_cacheInUser4 && nodep->user4()) {
            ফেরত V3Hash{static_cast<uint32_t>(nodep->user4())};
        } অন্যথা {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            যদি (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            যদি (hashChildren) iterateChildrenConst(nodep);  // Children
            যদি (m_cacheInUser4) nodep->user4(m_hash.মান());
            ফেরত m_hash;
        }
    }

    // VISITORS

    constexpr স্থির যৌক্তিক HASH_DTYPE = সত্য;
    constexpr স্থির যৌক্তিক HASH_CHILDREN = সত্য;

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

    শূন্য visit(Ast_গ্রন্থি* nodep) override {
#যদি VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    শূন্য visit(Ast_গ্রন্থি_Array_DType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->বাম();
            m_hash += nodep->ডান();
        });
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, মিথ্যা, [এই, nodep]() {  //
            m_hash += nodep->unique_অঙ্ক();
        });
    }
    শূন্য visit(AstParamTypeDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    শূন্য visit(AstMemberDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->unique_অঙ্ক();
        });
    }
    শূন্য visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    শূন্য visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    শূন্য visit(AstDynArrayDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    শূন্য visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    শূন্য visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    শূন্য visit(AstBasicDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().বাম();
            m_hash += nodep->nrange().ডান();
        });
    }
    শূন্য visit(AstCDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstConstDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    শূন্য visit(AstClassRefDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    শূন্য visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    শূন্য visit(AstQueueDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    শূন্য visit(AstRefDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    শূন্য visit(AstStreamDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstVoidDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstEnumDType* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, মিথ্যা, [এই, nodep]() {  //
            m_hash += nodep->unique_অঙ্ক();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstSel* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    শূন্য visit(AstConst* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->অঙ্ক().toHash();
        });
    }
    শূন্য visit(AstNullCheck* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstCCast* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->আকার_2();
        });
    }
    শূন্য visit(AstVarRef* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            যদি (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } অন্যথা {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    শূন্য visit(AstVarXRef* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    শূন্য visit(AstMemberSel* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstFScanF* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    শূন্য visit(AstSScanF* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    শূন্য visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    শূন্য visit(AstTriggered* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            জন্য (ধ্রুবক uint32_t সূচক : nodep->indices()) m_hash += সূচক;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, []() {});
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    শূন্য visit(AstCaseItem* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, []() {});
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    শূন্য visit(AstCAwait* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    শূন্য visit(AstCLocalScope* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstCoverInc* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    শূন্য visit(AstDisplay* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    শূন্য visit(AstMonitorOff* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    শূন্য visit(AstJumpGo* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, মিথ্যা, []() {});
    }
    শূন্য visit(AstTraceInc* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    শূন্য visit(Ast_গ্রন্থি_Cover_Or_Assert* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    শূন্য visit(Ast_গ্রন্থি_Range* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, মিথ্যা, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(Ast_গ্রন্থি_Pre_Sel* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstClassExtends* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstForeachHeader* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstDefParam* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstArg* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstParseRef* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    শূন্য visit(AstSenItem* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstSFormatF* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    শূন্য visit(AstElabDisplay* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    শূন্য visit(Ast_গ্রন্থি_Cons_Item* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ মিথ্যা,
                                     [এই, nodep, dtypep]() {
                                         ধ্রুবক uint32_t আকার_2 = dtypep->elementsConst();
                                         জন্য (uint32_t n = 0; n < আকার_2; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    শূন্য visit(AstPragma* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    শূন্য visit(AstAttrOf* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    শূন্য visit(Ast_গ্রন্থি_File* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [এই, nodep]() {  //
            m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                যদি (m_cacheInUser4) nodep->user4(V3Hash{1}.মান());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    শূন্য visit(AstVar* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    শূন্য visit(AstScope* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, মিথ্যা, [এই, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    শূন্য visit(AstVarScope* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    শূন্য visit(AstEnumItem* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstTypedef* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstTypedefFwd* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstActive* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    শূন্য visit(AstCell* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    শূন্য visit(AstCellInline* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [এই, nodep]() { m_hash += nodep->name(); });
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        guardRecursion(nodep, [এই, nodep]() {  //
            m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                যদি (m_cacheInUser4) nodep->user4(V3Hash{1}.মান());
            });
        });
    }
    শূন্য visit(AstModport* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstModportVarRef* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    শূন্য visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstBegin* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    শূন্য visit(AstFork* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    শূন্য visit(AstPin* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [এই, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_অঙ্ক();
        });
    }
    শূন্য visit(AstText* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    শূন্য visit(AstTextBlock* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, [এই, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    শূন্য visit(AstCStmt* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstCStmtUser* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, মিথ্যা, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstCExpr* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstCExprUser* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    শূন্য visit(AstWith* nodep) override {
        m_hash += hash_গ্রন্থি_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_গ্রন্থি* nodep)
        : m_cacheInUser4{সত্য} {
        iterateConst(nodep);
    }
    শ্রেণী Uncached {};
    HasherVisitor(ধ্রুবক Ast_গ্রন্থি* nodep, Uncached)
        : m_cacheInUser4{মিথ্যা} {
        iterateConst(const_cast<Ast_গ্রন্থি*>(nodep));
    }
    V3Hash finalHash() ধ্রুবক { ফেরত m_hash; }
    ~HasherVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_গ্রন্থি* nodep) ধ্রুবক {
    যদি (!nodep->user4()) HasherVisitor{nodep};
    ফেরত V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_গ্রন্থি* nodep) ধ্রুবক {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    ফেরত V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ধ্রুবক Ast_গ্রন্থি* nodep) {
    ধ্রুবক HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    ফেরত visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

আকার_t V3HasherUncachedHash(ধ্রুবক Ast_গ্রন্থি& গ্রন্থি) {
    ফেরত static_cast<আকার_t>(V3Hasher::uncachedHash(&গ্রন্থি).মান());
}
