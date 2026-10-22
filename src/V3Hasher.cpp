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

#ضمن "V3PchAstMT.h"

#ضمن "V3Hasher.h"

#ضمن <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

صنف HasherVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ثابت منطقي m_cache_دننه_User4;  // Use user4 to cache each V3Hash?
    std::سېټ<Ast_غوټه*> m_لیدل_شوی;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    فراغ guardRecursion(Ast_غوټه* ثابت nodep, std::function<فراغ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        إذا (!m_cache_دننه_User4) {
            إذا (m_لیدل_شوی.پیدا_کړه(nodep) != m_لیدل_شوی.پای_ټکی()) {
                m_hash += V3Hash{nodep->name()};
                أرجع;
            }
            m_لیدل_شوی.ورننباسه(nodep);
        }
        f();
    }

    V3Hash hash_غوټه_And_Iterate(Ast_غوټه* nodep, منطقي hashDType, منطقي hashChildren,
                              std::function<فراغ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        إذا (m_cache_دننه_User4 && nodep->user4()) {
            أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
        } وإلا {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            إذا (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            إذا (hashChildren) iterateChildrenConst(nodep);  // Children
            إذا (m_cache_دننه_User4) nodep->user4(m_hash.ارزښت());
            أرجع m_hash;
        }
    }

    // VISITORS

    constexpr ساكن منطقي HASH_DTYPE = ریښتیا;
    constexpr ساكن منطقي HASH_CHILDREN = ریښتیا;

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

    فراغ لیدنه(Ast_غوټه* nodep) override {
#إذا VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#د_که_پای
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    فراغ لیدنه(Ast_غوټه_Array_DType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->کیڼ();
            m_hash += nodep->ښی();
        });
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }
    فراغ لیدنه(AstParamTypeDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ لیدنه(AstMemberDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstDefImplicitDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }
    فراغ لیدنه(AstAssocArrayDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    فراغ لیدنه(AstBracketArrayDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ لیدنه(AstDynArrayDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ لیدنه(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ لیدنه(AstWildcardArrayDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ لیدنه(AstBasicDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().کیڼ();
            m_hash += nodep->nrange().ښی();
        });
    }
    فراغ لیدنه(AstCDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstConstDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ لیدنه(AstClassRefDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    فراغ لیدنه(AstIfaceRefDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    فراغ لیدنه(Ast_کتار_DType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ لیدنه(AstRefDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    فراغ لیدنه(Ast_بهیر_DType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstVoidDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstEnumDType* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstSel* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->پلنوالی_Const();
        });
    }
    فراغ لیدنه(AstConst* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->عدد().منزل_Hash();
        });
    }
    فراغ لیدنه(AstNullCheck* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstCCast* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->اندازه();
        });
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            إذا (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } وإلا {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_اشاره_ګر().asString();
            }
        });
    }
    فراغ لیدنه(AstVarXRef* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstFScanF* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ لیدنه(AstSScanF* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ لیدنه(Ast_Addr_د_CFunc* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ لیدنه(AstTriggered* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            لكل (ثابت uint32_t شاخص : nodep->indices()) m_hash += شاخص;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ لیدنه(Ast_Case_توکی* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    فراغ لیدنه(AstCMethodHard* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->لاره();
        });
    }
    فراغ لیدنه(AstCAwait* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ لیدنه(AstCLocalScope* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstCoverInc* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ لیدنه(AstDisplay* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ لیدنه(AstMonitorOff* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    فراغ لیدنه(AstJumpGo* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, خطأ, []() {});
    }
    فراغ لیدنه(AstTraceInc* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ لیدنه(Ast_غوټه_Cover_Or_Assert* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    فراغ لیدنه(Ast_غوټه_Range* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(Ast_غوټه_Pre_Sel* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstClassExtends* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(Ast_Foreach_سرلیک* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstDefParam* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstArg* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstParseRef* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->څنډه_Type();
        });
    }
    فراغ لیدنه(Ast_Sen_ونه* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstSFormatF* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ لیدنه(AstElabDisplay* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ لیدنه(Ast_غوټه_Cons_توکی* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ خطأ,
                                     [هذا, nodep, dtypep]() {
                                         ثابت uint32_t اندازه = dtypep->elementsConst();
                                         لكل (uint32_t n = 0; n < اندازه; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    فراغ لیدنه(AstPragma* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    فراغ لیدنه(Ast_Attr_د* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    فراغ لیدنه(Ast_غوټه_File* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                إذا (m_cache_دننه_User4) nodep->user4(V3Hash{1}.ارزښت());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    فراغ لیدنه(AstVar* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ لیدنه(AstScope* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    فراغ لیدنه(AstVarScope* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    فراغ لیدنه(Ast_Enum_توکی* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstTypedef* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstTypedefFwd* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(Ast_فعال* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ لیدنه(AstCell* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    فراغ لیدنه(AstCellInline* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [هذا, nodep]() { m_hash += nodep->name(); });
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                إذا (m_cache_دننه_User4) nodep->user4(V3Hash{1}.ارزښت());
            });
        });
    }
    فراغ لیدنه(AstModport* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstModportVarRef* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    فراغ لیدنه(AstModportFTaskRef* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstBegin* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ لیدنه(AstFork* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->سمدستي_پیل();
        });
    }
    فراغ لیدنه(AstPin* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_عدد();
        });
    }
    فراغ لیدنه(Ast_متن* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ لیدنه(Ast_متن_بلاک* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->مختاړی();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    فراغ لیدنه(AstCStmt* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstCStmtUser* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstCExpr* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstCExprUser* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ لیدنه(AstWith* nodep) override {
        m_hash += hash_غوټه_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

عام:
    // CONSTRUCTORS
    څرګند HasherVisitor(Ast_غوټه* nodep)
        : m_cache_دننه_User4{ریښتیا} {
        iterateConst(nodep);
    }
    صنف Uncached {};
    HasherVisitor(ثابت Ast_غوټه* nodep, Uncached)
        : m_cache_دننه_User4{خطأ} {
        iterateConst(const_cast<Ast_غوټه*>(nodep));
    }
    V3Hash finalHash() ثابت { أرجع m_hash; }
    ~HasherVisitor() override = افتراضي;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::چلوونکی()(Ast_غوټه* nodep) ثابت {
    إذا (!nodep->user4()) HasherVisitor{nodep};
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_غوټه* nodep) ثابت {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ثابت Ast_غوټه* nodep) {
    ثابت HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    أرجع visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

اندازه_t V3HasherUncachedHash(ثابت Ast_غوټه& غوټه) {
    أرجع static_cast<اندازه_t>(V3Hasher::uncachedHash(&غوټه).ارزښت());
}
