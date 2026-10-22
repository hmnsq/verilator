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
    ثابت منطقي m_cache_وچ_User4;  // Use user4 to cache each V3Hash?
    std::سیٹ<Ast_گرہ*> m_ویکھیا;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    فراغ guardRecursion(Ast_گرہ* ثابت nodep, std::function<فراغ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        إذا (!m_cache_وچ_User4) {
            إذا (m_ویکھیا.لبھو(nodep) != m_ویکھیا.سرا()) {
                m_hash += V3Hash{nodep->name()};
                أرجع;
            }
            m_ویکھیا.داخل_کرو(nodep);
        }
        f();
    }

    V3Hash hash_گرہ_And_Iterate(Ast_گرہ* nodep, منطقي hashDType, منطقي hashChildren,
                              std::function<فراغ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        إذا (m_cache_وچ_User4 && nodep->user4()) {
            أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
        } وإلا {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            إذا (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            إذا (hashChildren) iterateChildrenConst(nodep);  // Children
            إذا (m_cache_وچ_User4) nodep->user4(m_hash.قدر());
            أرجع m_hash;
        }
    }

    // VISITORS

    constexpr ساكن منطقي HASH_DTYPE = سچ;
    constexpr ساكن منطقي HASH_CHILDREN = سچ;

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

    فراغ ویکھو(Ast_گرہ* nodep) override {
#إذا VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#جے_انت
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    فراغ ویکھو(Ast_گرہ_Array_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->کھبے();
            m_hash += nodep->سجے();
        });
    }
    فراغ ویکھو(Ast_گرہ_UOr_Struct_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }
    فراغ ویکھو(AstParamTypeDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ ویکھو(AstMemberDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstDefImplicitDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }
    فراغ ویکھو(AstAssocArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    فراغ ویکھو(AstBracketArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ ویکھو(AstDynArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ ویکھو(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ ویکھو(AstWildcardArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ ویکھو(AstBasicDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().کھبے();
            m_hash += nodep->nrange().سجے();
        });
    }
    فراغ ویکھو(AstCDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstConstDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ ویکھو(AstClassRefDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    فراغ ویکھو(AstIfaceRefDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    فراغ ویکھو(Ast_قطار_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ ویکھو(AstRefDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    فراغ ویکھو(Ast_دھارا_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstVoidDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstEnumDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    فراغ ویکھو(Ast_گرہ_اظہاریہ* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstSel* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->چوڑائی_Const();
        });
    }
    فراغ ویکھو(AstConst* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->عدد().منزل_Hash();
        });
    }
    فراغ ویکھو(AstNullCheck* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstCCast* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->ناپ();
        });
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            إذا (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } وإلا {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_اشارہ_گر().asString();
            }
        });
    }
    فراغ ویکھو(AstVarXRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstFScanF* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ ویکھو(AstSScanF* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ ویکھو(Ast_Addr_دا_CFunc* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ ویکھو(AstTriggered* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            لكل (ثابت uint32_t اشاریہ : nodep->indices()) m_hash += اشاریہ;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ ویکھو(Ast_Case_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    فراغ ویکھو(AstCMethodHard* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->طریقہ();
        });
    }
    فراغ ویکھو(AstCAwait* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ ویکھو(AstCLocalScope* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstCoverInc* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ ویکھو(AstDisplay* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ ویکھو(AstMonitorOff* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    فراغ ویکھو(AstJumpGo* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, خطأ, []() {});
    }
    فراغ ویکھو(AstTraceInc* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ ویکھو(Ast_گرہ_Cover_Or_Assert* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    فراغ ویکھو(Ast_گرہ_Range* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(Ast_گرہ_Pre_Sel* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstClassExtends* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(Ast_Foreach_سرخی* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstDefParam* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstArg* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstParseRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstClassOrPackageRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    فراغ ویکھو(Ast_Sen_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->کنارا_Type();
        });
    }
    فراغ ویکھو(Ast_Sen_درخت* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstSFormatF* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ ویکھو(AstElabDisplay* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ ویکھو(Ast_گرہ_Cons_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ خطأ,
                                     [هذا, nodep, dtypep]() {
                                         ثابت uint32_t ناپ = dtypep->elementsConst();
                                         لكل (uint32_t n = 0; n < ناپ; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    فراغ ویکھو(AstPragma* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    فراغ ویکھو(Ast_Attr_دا* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    فراغ ویکھو(Ast_گرہ_File* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                إذا (m_cache_وچ_User4) nodep->user4(V3Hash{1}.قدر());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    فراغ ویکھو(AstVar* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ ویکھو(AstScope* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    فراغ ویکھو(AstVarScope* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    فراغ ویکھو(Ast_Enum_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstTypedef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstTypedefFwd* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(Ast_سرگرم* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ ویکھو(AstCell* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    فراغ ویکھو(AstCellInline* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [هذا, nodep]() { m_hash += nodep->name(); });
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                إذا (m_cache_وچ_User4) nodep->user4(V3Hash{1}.قدر());
            });
        });
    }
    فراغ ویکھو(AstModport* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstModportVarRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    فراغ ویکھو(AstModportFTaskRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    فراغ ویکھو(Ast_گرہ_Procedure* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstBegin* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ ویکھو(AstFork* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->فوری_شروع();
        });
    }
    فراغ ویکھو(AstPin* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_عدد();
        });
    }
    فراغ ویکھو(Ast_متن* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ ویکھو(Ast_متن_بلاک* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->اگیتر();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    فراغ ویکھو(AstCStmt* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstCStmtUser* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstCExpr* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstCExprUser* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ ویکھو(AstWith* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

عام:
    // CONSTRUCTORS
    واضح HasherVisitor(Ast_گرہ* nodep)
        : m_cache_وچ_User4{سچ} {
        iterateConst(nodep);
    }
    صنف Uncached {};
    HasherVisitor(ثابت Ast_گرہ* nodep, Uncached)
        : m_cache_وچ_User4{خطأ} {
        iterateConst(const_cast<Ast_گرہ*>(nodep));
    }
    V3Hash finalHash() ثابت { أرجع m_hash; }
    ~HasherVisitor() override = افتراضي;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::عامل()(Ast_گرہ* nodep) ثابت {
    إذا (!nodep->user4()) HasherVisitor{nodep};
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_گرہ* nodep) ثابت {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ثابت Ast_گرہ* nodep) {
    ثابت HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    أرجع visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

ناپ_t V3HasherUncachedHash(ثابت Ast_گرہ& گرہ) {
    أرجع static_cast<ناپ_t>(V3Hasher::uncachedHash(&گرہ).قدر());
}
