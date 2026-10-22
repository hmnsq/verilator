// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Plan for Hierarchical Verilation
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

#اگر_تعریف_نشده VERILATOR_V3HIERBLOCK_H_
#تعریف_کن VERILATOR_V3HIERBLOCK_H_

#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"

#ضمن <map>
#ضمن <set>
#ضمن <string>
#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <utility>
#ضمن <vector>

class AstNetlist;
class Ast_گره_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3_Hier_گراف final : عام V3_گراف {
عام:
    V3_Hier_گراف() = افتراضي;
    ~V3_Hier_گراف() = افتراضي;
    VL_UNCOPYABLE(V3_Hier_گراف);
    VL_UNMOVABLE(V3_Hier_گراف);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    ساكن VStringList منبع_Files(ثابت string& بالاترین_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    فراغ write_Command_آرگومانها_Files(bool forMkJson) ثابت VL_MT_DISABLED;
    فراغ writeParametersFiles() ثابت VL_MT_DISABLED;
    ساكن string بالاترین_Command_آرگومانها_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3_Hier_بلوک final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(V3_Hier_بلوک, V3_گراف_رأس)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::وکتور<StrGParam>;

    // MEMBERS
    ثابت AstModule* ثابت m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ثابت std::وکتور<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ثابت std::وکتور<AstParamTypeDType*> m_typeParams;

    // METHODS
    ساكن StrGParams stringifyParams(ثابت std::وکتور<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

عام:
    // CONSTRUCTORs
    V3_Hier_بلوک(V3_Hier_گراف* graphp, ثابت AstModule* modp, ثابت std::وکتور<AstVar*>& params,
                ثابت std::وکتور<AstParamTypeDType*>& typeParams)
        : V3_گراف_رأس{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3_Hier_بلوک() VL_MT_DISABLED = افتراضي;
    VL_UNCOPYABLE(V3_Hier_بلوک);
    VL_UNMOVABLE(V3_Hier_بلوک);

    ثابت AstModule* modp() ثابت { أرجع m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    ساكن bool stringParamPassable(ثابت string& مقدار) VL_خالص;

    // For emitting Makefile and build definition JSON
    VStringList command_آرگومانها(bool forMkJson) ثابت VL_MT_DISABLED;
    VStringList hier_بلوک_آرگومانها() ثابت VL_MT_DISABLED;
    string hier_پیشوند() ثابت VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ثابت حرف* پیشوند,
                            ثابت حرف* suffix) ثابت VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ثابت VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ثابت VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    فراغ write_Command_آرگومانها_File(bool forMkJson) ثابت VL_MT_DISABLED;
    فراغ writeParametersFile() ثابت VL_MT_DISABLED;
    string command_آرگومانها_Filename(bool forMkJson) ثابت VL_MT_DISABLED;
    string typeParametersFilename() ثابت VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ثابت override { أرجع m_modp->prettyNameQ(); }
    std::string dotShape() ثابت override { أرجع "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3_سلسلهمراتبی final {
عام:
    ساكن فراغ create_گراف(AstNetlist* nodep) VL_MT_DISABLED;
};

#پایان_اگر  // guard
