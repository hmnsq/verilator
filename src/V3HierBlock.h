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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3HIERBLOCK_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3HIERBLOCK_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class AstNetlist;
class Ast_ਗੰਢ_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3_Hier_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
ਜਨਤਕ:
    V3_Hier_ਗ੍ਰਾਫ() = ਮੂਲ;
    ~V3_Hier_ਗ੍ਰਾਫ() = ਮੂਲ;
    VL_UNCOPYABLE(V3_Hier_ਗ੍ਰਾਫ);
    VL_UNMOVABLE(V3_Hier_ਗ੍ਰਾਫ);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    ਸਥਿਰ VStringList ਸਰੋਤ_Files(ਅਚਲ string& ਸਿਖਰ_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    ਖਾਲੀ write_Command_ਦਲੀਲਾਂ_Files(bool forMkJson) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ writeParametersFiles() ਅਚਲ VL_MT_DISABLED;
    ਸਥਿਰ string ਸਿਖਰ_Command_ਦਲੀਲਾਂ_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3_Hier_ਖੰਡ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(V3_Hier_ਖੰਡ, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::ਸਦਿਸ਼<StrGParam>;

    // MEMBERS
    ਅਚਲ AstModule* ਅਚਲ m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ਅਚਲ std::ਸਦਿਸ਼<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ਅਚਲ std::ਸਦਿਸ਼<AstParamTypeDType*> m_typeParams;

    // METHODS
    ਸਥਿਰ StrGParams stringifyParams(ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

ਜਨਤਕ:
    // CONSTRUCTORs
    V3_Hier_ਖੰਡ(V3_Hier_ਗ੍ਰਾਫ* graphp, ਅਚਲ AstModule* modp, ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& params,
                ਅਚਲ std::ਸਦਿਸ਼<AstParamTypeDType*>& typeParams)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3_Hier_ਖੰਡ() VL_MT_DISABLED = ਮੂਲ;
    VL_UNCOPYABLE(V3_Hier_ਖੰਡ);
    VL_UNMOVABLE(V3_Hier_ਖੰਡ);

    ਅਚਲ AstModule* modp() ਅਚਲ { ਵਾਪਸ m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    ਸਥਿਰ bool stringParamPassable(ਅਚਲ string& ਮੁੱਲ) VL_ਸ਼ੁੱਧ;

    // For emitting Makefile and build definition JSON
    VStringList command_ਦਲੀਲਾਂ(bool forMkJson) ਅਚਲ VL_MT_DISABLED;
    VStringList hier_ਖੰਡ_ਦਲੀਲਾਂ() ਅਚਲ VL_MT_DISABLED;
    string hier_ਅਗੇਤਰ() ਅਚਲ VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ਅਚਲ ਅੱਖਰ* ਅਗੇਤਰ,
                            ਅਚਲ ਅੱਖਰ* suffix) ਅਚਲ VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ਅਚਲ VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ਅਚਲ VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ਅਚਲ VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ਅਚਲ VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ਅਚਲ VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    ਖਾਲੀ write_Command_ਦਲੀਲਾਂ_File(bool forMkJson) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ writeParametersFile() ਅਚਲ VL_MT_DISABLED;
    string command_ਦਲੀਲਾਂ_Filename(bool forMkJson) ਅਚਲ VL_MT_DISABLED;
    string typeParametersFilename() ਅਚਲ VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ਅਚਲ override { ਵਾਪਸ m_modp->prettyNameQ(); }
    std::string dotShape() ਅਚਲ override { ਵਾਪਸ "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3_ਦਰਜਾਵਾਰ final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ create_ਗ੍ਰਾਫ(AstNetlist* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // guard
