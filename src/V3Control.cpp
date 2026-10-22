// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3InstrCount.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T>
ਵਰਗ V3ControlWildcardResolver final {
    ਬਦਲਣਯੋਗ V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ਅਚਲ std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ਅਚਲ std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

ਜਨਤਕ:
    V3ControlWildcardResolver() = ਮੂਲ;
    ~V3ControlWildcardResolver() = ਮੂਲ;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    ਖਾਲੀ update(ਅਚਲ V3ControlWildcardResolver& other) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        ਅਚਲ V3_ਤਾਲਾ_Guard other_ਤਾਲਾ{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : other.m_mapPatterns) m_mapPatterns[itr.ਪਹਿਲਾ].update(itr.ਦੂਜਾ);
    }

    // Access and create a pattern entry
    T& at(ਅਚਲ string& name) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        ਵਾਪਸ m_mapPatterns[V3Os::filename_Slash_ਰਸਤਾ(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ਅਚਲ string& name) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_mapResolved.emplace(name, ਖਾਲੀ_ਸੂਚਕ);
        std::unique_ptr<T>& entryr = pair.ਪਹਿਲਾ->ਦੂਜਾ;
        // Resolve entry when first requested, cache the result
        ਜੇ (pair.ਦੂਜਾ) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ਅਚਲ string slashName = V3Os::filename_Slash_ਰਸਤਾ(name);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& patEnt : m_mapPatterns) {
                ਜੇ (VString::wildmatch(slashName, patEnt.ਪਹਿਲਾ)) {
                    ਜੇ (!entryr) entryr.reset(ਨਵਾਂ T{});
                    entryr->update(patEnt.ਦੂਜਾ);
                }
            }
            // Perform final actions that needed all updates completed
            ਜੇ (entryr) entryr->updateFinalize();
        }
        ਵਾਪਸ entryr.get();
    }
};

// List of attributes for variables
ਵਰਗ V3ControlVar final {
    std::ਸਦਿਸ਼<VAttrType> m_attrs;  // The list of attributes
ਜਨਤਕ:
    // Add new attribugte
    ਖਾਲੀ ਜੋੜੋ(VAttrType attr) { m_attrs.emplace_ਪਿੱਠ(attr); }
    // Update from other by copying all attributes
    ਖਾਲੀ update(ਅਚਲ V3ControlVar& other) {
        m_attrs.reserve(m_attrs.ਮਾਪ() + other.m_attrs.ਮਾਪ());
        m_attrs.ਪਾਓ(m_attrs.ਸਿਰਾ(), other.m_attrs.begin(), other.m_attrs.ਸਿਰਾ());
    }
    ਖਾਲੀ updateFinalize() {}
    // Apply all attributes to the variable
    ਖਾਲੀ apply(AstVar* varp) ਅਚਲ {
        ਲਈ (ਅਚਲ VAttrType attr : m_attrs) {
            varp->ਜੋੜੋ_Attrsp(ਨਵਾਂ Ast_Attr_ਦਾ{varp->fileline(), attr});
        }
    }
};

ਵਰਤੋਂ V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

ਵਰਗ WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ਅਚਲ std::string, ਤਾਰਕਿਕ> m_mapPatterns;  // Pattern match results
    std::deque<string> m_ਸਤਰਾਂ;  // Source text lines

    // METHODS
    ਸਥਿਰ WildcardContents& s() {  // Singleton
        ਸਥਿਰ WildcardContents s_s;
        ਵਾਪਸ s_s;
    }
    ਖਾਲੀ clearCacheImp() { m_mapPatterns.clear(); }
    ਖਾਲੀ ਧੱਕੋ_ਪਾਠ_Imp(ਅਚਲ string& ਪਾਠ) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ਅਚਲ string leftover = m_ਸਤਰਾਂ.ਪਿੱਠ() + ਪਾਠ;
        m_ਸਤਰਾਂ.ਕੱਢੋ_ਪਿੱਠ();

        // Insert line-by-line
        string::ਮਾਪ_type ਸਤਰ_ਸ਼ੁਰੂ = 0;
        ਜਦੋਂ (ਸੱਚ) {
            ਅਚਲ string::ਮਾਪ_type ਸਤਰ_ਸਿਰਾ = leftover.ਲੱਭੋ('\n', ਸਤਰ_ਸ਼ੁਰੂ);
            ਜੇ (ਸਤਰ_ਸਿਰਾ != string::npos) {
                ਅਚਲ string oneline(leftover, ਸਤਰ_ਸ਼ੁਰੂ, ਸਤਰ_ਸਿਰਾ - ਸਤਰ_ਸ਼ੁਰੂ + 1);
                ਜੇ (oneline.ਮਾਪ() > 1) m_ਸਤਰਾਂ.ਧੱਕੋ_ਪਿੱਠ(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_ਸਤਰਾਂ.ਮਾਪ() - 1) << "]: " << oneline);
                ਸਤਰ_ਸ਼ੁਰੂ = ਸਤਰ_ਸਿਰਾ + 1;
            } ਨਹੀਂਤਾਂ {
                ਰੋਕੋ;
            }
        }
        // Keep leftover for next time
        m_ਸਤਰਾਂ.emplace_ਪਿੱਠ(string(leftover, ਸਤਰ_ਸ਼ੁਰੂ));  // Might be ""
        clearCacheImp();
    }

    ਤਾਰਕਿਕ resolveUncachedImp(ਅਚਲ string& contentsRegexp) {
        ਲਈ (ਅਚਲ string& i : m_ਸਤਰਾਂ) {
            ਜੇ (VString::wildmatch(i, contentsRegexp)) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਤਾਰਕਿਕ resolveCachedImp(ਅਚਲ string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        ਜੇ (contentsRegexp.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਸੱਚ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_mapPatterns.emplace(contentsRegexp, ਝੂਠ);
        ਤਾਰਕਿਕ& entryr = pair.ਪਹਿਲਾ->ਦੂਜਾ;
        // Resolve entry when first requested, cache the result
        ਜੇ (pair.ਦੂਜਾ) entryr = resolveUncachedImp(contentsRegexp);
        ਵਾਪਸ entryr;
    }

ਜਨਤਕ:
    WildcardContents() {
        m_ਸਤਰਾਂ.emplace_ਪਿੱਠ("");  // start with no leftover
    }
    ~WildcardContents() = ਮੂਲ;
    // Return true iff contentsRegexp in parsed contents
    ਸਥਿਰ ਤਾਰਕਿਕ resolve(ਅਚਲ string& contentsRegexp) {
        ਵਾਪਸ s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ਸਥਿਰ ਖਾਲੀ ਧੱਕੋ_ਪਾਠ(ਅਚਲ string& ਪਾਠ) { s().ਧੱਕੋ_ਪਾਠ_Imp(ਪਾਠ); }
};

//######################################################################
// Function or task: Have variables and properties

ਵਰਗ V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    ਤਾਰਕਿਕ m_noinline = ਝੂਠ;  // Don't inline function/task
    ਤਾਰਕਿਕ m_ਜਨਤਕ = ਝੂਠ;  // Public function/task

ਜਨਤਕ:
    V3ControlFTask() = ਮੂਲ;
    ਖਾਲੀ update(ਅਚਲ V3ControlFTask& f) {
        // Don't overwrite true with false
        ਜੇ (f.m_noinline) m_noinline = ਸੱਚ;
        ਜੇ (f.m_ਜਨਤਕ) m_ਜਨਤਕ = ਸੱਚ;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    ਖਾਲੀ updateFinalize() {}

    V3ControlVarResolver& params() { ਵਾਪਸ m_params; }
    V3ControlVarResolver& ports() { ਵਾਪਸ m_ports; }
    V3ControlVarResolver& vars() { ਵਾਪਸ m_vars; }

    ਖਾਲੀ ਸੈੱਟ_No_Inline(ਤਾਰਕਿਕ ਸੈੱਟ) { m_noinline = ਸੈੱਟ; }
    ਖਾਲੀ ਸੈੱਟ_ਜਨਤਕ(ਤਾਰਕਿਕ ਸੈੱਟ) { m_ਜਨਤਕ = ਸੈੱਟ; }

    ਖਾਲੀ apply(Ast_ਗੰਢ_FTask* ftaskp) ਅਚਲ {
        ਜੇ (m_noinline)
            ftaskp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        ਜੇ (m_ਜਨਤਕ)
            ftaskp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstPragma{ftaskp->fileline(), VPragmaType::ਜਨਤਕ_TASK});
    }
};

ਵਰਤੋਂ V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

ਵਰਗ V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_ਰਜਿਸਟਰ_Wrapper m_fsm_ਰਜਿਸਟਰ_Wrapper;  // FSM wrapper descriptor
    std::unordered_ਸੈੱਟ<std::string> m_coverage_Off_ਖੰਡਾਂ;  // List of block names for coverage_off
    std::ਸੈੱਟ<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    ਤਾਰਕਿਕ m_has_Fsm_ਰਜਿਸਟਰ_Wrapper = ਝੂਠ;  // Whether descriptor has been set
    ਤਾਰਕਿਕ m_inline = ਝੂਠ;  // Whether to force the inline
    ਤਾਰਕਿਕ m_inline_ਮੁੱਲ = ਝੂਠ;  // The inline value (on/off)

ਜਨਤਕ:
    V3ControlModule() = ਮੂਲ;

    ਖਾਲੀ update(ਅਚਲ V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        ਜੇ (m.m_has_Fsm_ਰਜਿਸਟਰ_Wrapper) {
            m_fsm_ਰਜਿਸਟਰ_Wrapper = m.m_fsm_ਰਜਿਸਟਰ_Wrapper;
            m_has_Fsm_ਰਜਿਸਟਰ_Wrapper = ਸੱਚ;
        }
        ਲਈ (ਅਚਲ string& i : m.m_coverage_Off_ਖੰਡਾਂ) m_coverage_Off_ਖੰਡਾਂ.ਪਾਓ(i);
        ਜੇ (!m_inline) {
            m_inline = m.m_inline;
            m_inline_ਮੁੱਲ = m.m_inline_ਮੁੱਲ;
        }
        ਲਈ (ਸਵੈਚਾਲਿਤ it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ਪਾਓ(*it);
        }
    }
    ਖਾਲੀ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { ਵਾਪਸ m_tasks; }
    V3ControlVarResolver& params() { ਵਾਪਸ m_params; }
    V3ControlVarResolver& ports() { ਵਾਪਸ m_ports; }
    V3ControlVarResolver& vars() { ਵਾਪਸ m_vars; }

    ਖਾਲੀ ਜੋੜੋ_Coverage_ਖੰਡ_Off(ਅਚਲ string& name) { m_coverage_Off_ਖੰਡਾਂ.ਪਾਓ(name); }
    ਖਾਲੀ ਸੈੱਟ_Inline(ਤਾਰਕਿਕ ਸੈੱਟ) {
        m_inline = ਸੱਚ;
        m_inline_ਮੁੱਲ = ਸੈੱਟ;
    }
    ਖਾਲੀ ਸੈੱਟ_Fsm_ਰਜਿਸਟਰ_Wrapper(File_ਸਤਰ* fl, ਅਚਲ V3Control::Fsm_ਰਜਿਸਟਰ_Wrapper& desc) {
        ਜੇ (m_has_Fsm_ਰਜਿਸਟਰ_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ਗੰਢ::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_ਰਜਿਸਟਰ_Wrapper = desc;
        m_has_Fsm_ਰਜਿਸਟਰ_Wrapper = ਸੱਚ;
    }
    ਅਚਲ V3Control::Fsm_ਰਜਿਸਟਰ_Wrapper* fsm_ਰਜਿਸਟਰ_Wrapperp() ਅਚਲ {
        ਵਾਪਸ m_has_Fsm_ਰਜਿਸਟਰ_Wrapper ? &m_fsm_ਰਜਿਸਟਰ_Wrapper : ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਜੋੜੋ_Module_Pragma(VPragmaType pragma) { m_modPragmas.ਪਾਓ(pragma); }

    ਖਾਲੀ apply(Ast_ਗੰਢ_Module* modp) {
        ਜੇ (m_inline) {
            ਅਚਲ VPragmaType type
                = m_inline_ਮੁੱਲ ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ਗੰਢ* ਅਚਲ nodep = ਨਵਾਂ AstPragma{modp->fileline(), type};
            modp->ਜੋੜੋ_Stmtsp(nodep);
        }
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            ਜੇ (itr == VPragmaType::HIER_PARAMS) modp->hierParams(ਸੱਚ);
            Ast_ਗੰਢ* ਅਚਲ nodep = ਨਵਾਂ AstPragma{modp->fileline(), itr};
            modp->ਜੋੜੋ_Stmtsp(nodep);
        }
    }

    ਖਾਲੀ apply_ਖੰਡ(Ast_Gen_ਖੰਡ* nodep) {
        ਅਚਲ VPragmaType pragma = VPragmaType::COVERAGE_ਖੰਡ_OFF;
        ਜੇ (!nodep->unnamed()) {
            ਲਈ (ਅਚਲ string& i : m_coverage_Off_ਖੰਡਾਂ) {
                ਜੇ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->ਜੋੜੋ_Itemsp(ਨਵਾਂ AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    ਖਾਲੀ apply_ਖੰਡ(Ast_ਗੰਢ_ਖੰਡ* nodep) {
        ਅਚਲ VPragmaType pragma = VPragmaType::COVERAGE_ਖੰਡ_OFF;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (!nodep->unnamed()) {
            ਲਈ (ਅਚਲ string& i : m_coverage_Off_ਖੰਡਾਂ) {
                ਜੇ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstStmtPragma{flp, ਨਵਾਂ AstPragma{flp, pragma}});
                }
            }
        }
    }
};

ਵਰਤੋਂ V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
ਵਰਗ V3_Control_Ignores_ਸਤਰ final {
ਜਨਤਕ:
    ਅਚਲ ਪੂਰਨ m_ਸਤਰ_Min;  // Minimum line number to make change at
    ਅਚਲ ਪੂਰਨ m_ਸਤਰ_Max;  // Maximum line number to make change at (inclusive)
    ਅਚਲ V3_Error_ਕੋਡ m_ਕੋਡ;  // Error code
    ਅਚਲ ਤਾਰਕਿਕ m_on;  // True to enable message
    V3_Control_Ignores_ਸਤਰ(V3_Error_ਕੋਡ ਕੋਡ, ਪੂਰਨ linemin, ਪੂਰਨ linemax, ਤਾਰਕਿਕ on)
        : m_ਸਤਰ_Min{linemin}
        , m_ਸਤਰ_Max{linemax}
        , m_ਕੋਡ{ਕੋਡ}
        , m_on{on} {}
    ~V3_Control_Ignores_ਸਤਰ() = ਮੂਲ;
    ਤਾਰਕਿਕ every_ਸਤਰ() ਅਚਲ { ਵਾਪਸ m_ਸਤਰ_Min == 0 && m_ਸਤਰ_Max == 0; }
    ਤਾਰਕਿਕ ਵਿੱਚ_Range(ਪੂਰਨ lineno) ਅਚਲ {
        ਵਾਪਸ (lineno >= m_ਸਤਰ_Min && lineno <= m_ਸਤਰ_Max) || every_ਸਤਰ();
    }
};
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ V3_Control_Ignores_ਸਤਰ& rhs) {
    ਵਾਪਸ os << rhs.m_ਸਤਰ_Min << "-" << rhs.m_ਸਤਰ_Max << ", " << rhs.m_ਕੋਡ << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ਸਥਿਰ std::ਸਦਿਸ਼<V3_Control_Ignores_ਸਤਰ> control_Ign_ਸਤਰਾਂ;

ਵਰਤੋਂ IgnIndices = std::ਸਦਿਸ਼<uint32_t>;  // List of {s_ignLines indces}

ਵਰਗ VInterval_ਰੁੱਖ final {
    ਬਣਤਰ ਇੰਦਰਾਜ_2 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        ਪੂਰਨ m_center;  // Line number of central point
        std::unique_ptr<ਇੰਦਰਾਜ_2> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<ਇੰਦਰਾਜ_2> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        ਸਪਸ਼ਟ ਇੰਦਰਾਜ_2(ਪੂਰਨ center)
            : m_center{center} {}
    };

    std::unique_ptr<ਇੰਦਰਾਜ_2> m_rootp;  // Root of interval tree
    IgnIndices m_every_ਸਤਰਾਂ;  // All-line disables

ਨਿਜੀ:
    std::unique_ptr<ਇੰਦਰਾਜ_2> build_ਰੁੱਖ(ਅਚਲ IgnIndices& points) {
        ਜੇ (points.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        ਪੂਰਨ minval = std::numeric_limits<ਪੂਰਨ>::max();
        ਪੂਰਨ maxval = std::numeric_limits<ਪੂਰਨ>::min();
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : points) {
            ਅਚਲ V3_Control_Ignores_ਸਤਰ& cign = control_Ign_ਸਤਰਾਂ[it];
            ਜੇ (cign.every_ਸਤਰ()) {
                m_every_ਸਤਰਾਂ.emplace_ਪਿੱਠ(it);
                ਜਾਰੀ;
            }
            minval = std::min(minval, cign.m_ਸਤਰ_Min);
            maxval = std::max(maxval, cign.m_ਸਤਰ_Max);
        }

        ਅਚਲ ਪੂਰਨ center = minval + (maxval - minval) / 2;
        ਸਵੈਚਾਲਿਤ entp = std::unique_ptr<ਇੰਦਰਾਜ_2>(ਨਵਾਂ ਇੰਦਰਾਜ_2{center});

        IgnIndices ਖੱਬਾ_Points;
        IgnIndices ਸੱਜਾ_Points;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : points) {
            ਅਚਲ V3_Control_Ignores_ਸਤਰ& cign = control_Ign_ਸਤਰਾਂ[it];
            ਜੇ (cign.every_ਸਤਰ()) ਜਾਰੀ;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            ਜੇ (cign.m_ਸਤਰ_Max < center)
                ਖੱਬਾ_Points.emplace_ਪਿੱਠ(it);
            ਨਹੀਂਤਾਂ ਜੇ (cign.m_ਸਤਰ_Min > center)
                ਸੱਜਾ_Points.emplace_ਪਿੱਠ(it);
            ਨਹੀਂਤਾਂ {
                entp->m_byMin.ਧੱਕੋ_ਪਿੱਠ(it);
                entp->m_byMax.ਧੱਕੋ_ਪਿੱਠ(it);
            }
        }

        std::ਛਾਂਟੋ(entp->m_byMin.begin(), entp->m_byMin.ਸਿਰਾ(), [](uint32_t a, uint32_t b) {
            ਵਾਪਸ control_Ign_ਸਤਰਾਂ[a].m_ਸਤਰ_Min < control_Ign_ਸਤਰਾਂ[b].m_ਸਤਰ_Min;
        });
        std::ਛਾਂਟੋ(entp->m_byMax.begin(), entp->m_byMax.ਸਿਰਾ(), [](uint32_t a, uint32_t b) {
            ਵਾਪਸ control_Ign_ਸਤਰਾਂ[a].m_ਸਤਰ_Max > control_Ign_ਸਤਰਾਂ[b].m_ਸਤਰ_Max;
        });

        entp->m_leftp = build_ਰੁੱਖ(ਖੱਬਾ_Points);
        entp->m_rightp = build_ਰੁੱਖ(ਸੱਜਾ_Points);
        ਵਾਪਸ entp;
    }

    ਖਾਲੀ ਲੱਭੋ_ਰੁੱਖ(ਪੂਰਨ lineno, ਇੰਦਰਾਜ_2* entp, IgnIndices& resultsr, ਪੂਰਨ& ਅਗਲਾ_Changer) ਅਚਲ {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        ਜੇ (lineno < entp->m_center) {
            ਅਗਲਾ_Changer = std::min(ਅਗਲਾ_Changer, entp->m_center);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : entp->m_byMin) {
                ਅਚਲ V3_Control_Ignores_ਸਤਰ& cign = control_Ign_ਸਤਰਾਂ[it];
                ਜੇ (cign.m_ਸਤਰ_Min <= lineno) {
                    resultsr.emplace_ਪਿੱਠ(it);
                } ਨਹੀਂਤਾਂ {
                    ਅਗਲਾ_Changer = std::min(ਅਗਲਾ_Changer, cign.m_ਸਤਰ_Min);
                    ਰੋਕੋ;
                }
            }
            ਜੇ (entp->m_leftp) ਲੱਭੋ_ਰੁੱਖ(lineno, entp->m_leftp.get(), resultsr, ਅਗਲਾ_Changer);
        } ਨਹੀਂਤਾਂ ਜੇ (lineno > entp->m_center) {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : entp->m_byMax) {
                ਅਚਲ V3_Control_Ignores_ਸਤਰ& cign = control_Ign_ਸਤਰਾਂ[it];
                ਜੇ (cign.m_ਸਤਰ_Max >= lineno) {
                    ਅਗਲਾ_Changer = std::min(ਅਗਲਾ_Changer, cign.m_ਸਤਰ_Max + 1);
                    resultsr.emplace_ਪਿੱਠ(it);
                } ਨਹੀਂਤਾਂ {
                    ਰੋਕੋ;
                }
            }
            ਜੇ (entp->m_rightp) ਲੱਭੋ_ਰੁੱਖ(lineno, entp->m_rightp.get(), resultsr, ਅਗਲਾ_Changer);
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : entp->m_byMin) {
                ਅਚਲ V3_Control_Ignores_ਸਤਰ& cign = control_Ign_ਸਤਰਾਂ[it];
                resultsr.emplace_ਪਿੱਠ(it);
                ਜੇ (cign.m_ਸਤਰ_Max >= lineno)
                    ਅਗਲਾ_Changer = std::min(ਅਗਲਾ_Changer, cign.m_ਸਤਰ_Max + 1);
            }
            ਜੇ (entp->m_leftp) ਲੱਭੋ_ਰੁੱਖ(lineno, entp->m_leftp.get(), resultsr, ਅਗਲਾ_Changer);
            ਜੇ (entp->m_rightp) ਲੱਭੋ_ਰੁੱਖ(lineno, entp->m_rightp.get(), resultsr, ਅਗਲਾ_Changer);
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    VInterval_ਰੁੱਖ() = ਮੂਲ;
    ~VInterval_ਰੁੱਖ() = ਮੂਲ;
    // METHODS
    ਖਾਲੀ clear() { m_rootp = ਖਾਲੀ_ਸੂਚਕ; }
    ਖਾਲੀ build(ਅਚਲ IgnIndices& points) {
        clear();
        m_rootp = build_ਰੁੱਖ(points);
    }
    ਖਾਲੀ ਲੱਭੋ(ਪੂਰਨ lineno, IgnIndices& resultsr, ਪੂਰਨ& ਅਗਲਾ_Changer) ਅਚਲ {
        resultsr = m_every_ਸਤਰਾਂ;
        ਅਗਲਾ_Changer = std::numeric_limits<ਪੂਰਨ>::max();

        ਜੇ (m_rootp) ਲੱਭੋ_ਰੁੱਖ(lineno, m_rootp.get(), resultsr, ਅਗਲਾ_Changer);
        // Sort indices, so can process in parse order
        std::ਛਾਂਟੋ(resultsr.begin(), resultsr.ਸਿਰਾ());
    }

    ਸਥਿਰ ਖਾਲੀ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices ਅੰਕੜੇ;
        std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : points) {
            control_Ign_ਸਤਰਾਂ.emplace_ਪਿੱਠ(
                V3_Control_Ignores_ਸਤਰ{V3_Error_ਕੋਡ::I_LINT, it.ਪਹਿਲਾ, it.ਦੂਜਾ, ਸੱਚ});
            ਅੰਕੜੇ.emplace_ਪਿੱਠ(static_cast<uint32_t>(control_Ign_ਸਤਰਾਂ.ਮਾਪ() - 1));
        }
        VInterval_ਰੁੱਖ ਰੁੱਖ;
        ਰੁੱਖ.build(ਅੰਕੜੇ);

        IgnIndices results;
        ਪੂਰਨ ਅਗਲਾ_Change = 0;
        ਰੁੱਖ.ਲੱਭੋ(0, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 0);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, 10);
        ਰੁੱਖ.ਲੱਭੋ(10, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, 11);
        ਰੁੱਖ.ਲੱਭੋ(11, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, 15);  // Center, or would be 20
        ਰੁੱਖ.ਲੱਭੋ(20, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, 21);
        ਰੁੱਖ.ਲੱਭੋ(21, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, 25);  // Center, or would be 30
        ਰੁੱਖ.ਲੱਭੋ(30, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, 31);
        ਰੁੱਖ.ਲੱਭੋ(40, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, 41);
        ਰੁੱਖ.ਲੱਭੋ(41, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 0);
        UASSERT_SELFTEST(ਅਗਲਾ_Change, std::numeric_limits<ਪੂਰਨ>::max());
        //
        points = {{0, 0}};
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : points) {
            control_Ign_ਸਤਰਾਂ.emplace_ਪਿੱਠ(
                V3_Control_Ignores_ਸਤਰ{V3_Error_ਕੋਡ::I_LINT, it.ਪਹਿਲਾ, it.ਦੂਜਾ, ਸੱਚ});
            ਅੰਕੜੇ.emplace_ਪਿੱਠ(static_cast<uint32_t>(control_Ign_ਸਤਰਾਂ.ਮਾਪ() - 1));
        }
        ਰੁੱਖ.build(ਅੰਕੜੇ);
        //
        ਰੁੱਖ.ਲੱਭੋ(50, results, ਅਗਲਾ_Change);
        UASSERT_SELFTEST(results.ਮਾਪ(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
ਵਰਤੋਂ V3_Control_ਸਤਰ_Attribute = std::bitset<VPragmaType::ENUM_ਮਾਪ>;

ਵਰਗ WaiverSetting final {
ਜਨਤਕ:
    V3_Error_ਕੋਡ m_ਕੋਡ;  // Error code
    string m_contents;  // --contents regexp
    string m_ਮੇਲ;  // --match regexp
    WaiverSetting(V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ string& contents, ਅਚਲ string& ਮੇਲ)
        : m_ਕੋਡ{ਕੋਡ}
        , m_contents{contents}
        , m_ਮੇਲ{ਮੇਲ} {}
    ~WaiverSetting() = ਮੂਲ;
    WaiverSetting& ਸੰਚਾਲਕ=(ਅਚਲ WaiverSetting& rhs) = ਮੂਲ;
};

// File entity
ਵਰਗ V3ControlFile final {
    ਵਰਤੋਂ ਸਤਰ_Attr_Map = std::map<ਪੂਰਨ, V3_Control_ਸਤਰ_Attribute>;  // Map line->bitset of attributes
    ਵਰਤੋਂ Waivers = std::ਸਦਿਸ਼<WaiverSetting>;  // List of {code,wildcard string}

    ਸਤਰ_Attr_Map m_ਸਤਰ_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_ਰੁੱਖ m_interval_ਰੁੱਖ;  // Tree of indices to IgnLines

    ਬਣਤਰ {
        ਪੂਰਨ filenameno = -1;  // Last filename
        ਪੂਰਨ lineno = -1;  // Last linenumber
        ਪੂਰਨ ਅਗਲਾ_Change = -1;  // Line number of next change
    } m_ਆਖਰੀ_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    ਤਾਰਕਿਕ ਸਤਰ_ਮੇਲ(ਪੂਰਨ lineno, VPragmaType type) {
        ਜੇ (m_ਸਤਰ_Attrs.ਲੱਭੋ(0) != m_ਸਤਰ_Attrs.ਸਿਰਾ() && m_ਸਤਰ_Attrs[0][type]) ਵਾਪਸ ਸੱਚ;
        ਜੇ (m_ਸਤਰ_Attrs.ਲੱਭੋ(lineno) == m_ਸਤਰ_Attrs.ਸਿਰਾ()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ m_ਸਤਰ_Attrs[lineno][type];
    }

ਜਨਤਕ:
    V3ControlFile() = ਮੂਲ;
    ਖਾਲੀ update(ਅਚਲ V3ControlFile& file) {
        // Copy in all attributes and waivers
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : file.m_ਸਤਰ_Attrs) m_ਸਤਰ_Attrs[itr.ਪਹਿਲਾ] |= itr.ਦੂਜਾ;
        m_waivers.reserve(m_waivers.ਮਾਪ() + file.m_waivers.ਮਾਪ());
        m_waivers.ਪਾਓ(m_waivers.ਸਿਰਾ(), file.m_waivers.begin(), file.m_waivers.ਸਿਰਾ());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.ਮਾਪ() + file.m_ignIndices.ਮਾਪ());
        m_ignIndices.ਪਾਓ(m_ignIndices.ਸਿਰਾ(), file.m_ignIndices.begin(),
                            file.m_ignIndices.ਸਿਰਾ());
        // updateFinalize() will soon build tree
    }
    ਖਾਲੀ updateFinalize() { m_interval_ਰੁੱਖ.build(m_ignIndices); }
    ਖਾਲੀ ਜੋੜੋ_ਸਤਰ_Attribute(ਪੂਰਨ lineno, VPragmaType attr) { m_ਸਤਰ_Attrs[lineno].ਸੈੱਟ(attr); }
    ਖਾਲੀ ਜੋੜੋ_Ignore(V3_Error_ਕੋਡ ਕੋਡ, ਪੂਰਨ min, ਪੂਰਨ max, ਤਾਰਕਿਕ on) {
        control_Ign_ਸਤਰਾਂ.emplace_ਪਿੱਠ(V3_Control_Ignores_ਸਤਰ{ਕੋਡ, min, max, on});
        m_ignIndices.emplace_ਪਿੱਠ(static_cast<uint32_t>(control_Ign_ਸਤਰਾਂ.ਮਾਪ() - 1));
        m_ਆਖਰੀ_Ignore.ਅਗਲਾ_Change = -1;
    }
    ਖਾਲੀ ਜੋੜੋ_Ignore_ਮੇਲ(V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ string& contents, ਅਚਲ string& ਮੇਲ) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_ਮੇਲ = ਮੇਲ;
        ਜੇ (new_ਮੇਲ.ਖਾਲੀ_ਥਾਵਾਂ() || new_ਮੇਲ.ਪਿੱਠ() != '*') new_ਮੇਲ += '*';
        m_waivers.emplace_ਪਿੱਠ(ਕੋਡ, contents, new_ਮੇਲ);
    }

    ਖਾਲੀ apply_ਖੰਡ(Ast_Gen_ਖੰਡ* nodep) {
        // Apply to block at this line
        ਅਚਲ VPragmaType pragma = VPragmaType::COVERAGE_ਖੰਡ_OFF;
        ਜੇ (ਸਤਰ_ਮੇਲ(nodep->fileline()->lineno(), pragma)) {
            nodep->ਜੋੜੋ_Itemsp(ਨਵਾਂ AstPragma{nodep->fileline(), pragma});
        }
    }
    ਖਾਲੀ apply_ਖੰਡ(Ast_ਗੰਢ_ਖੰਡ* nodep) {
        // Apply to block at this line
        ਅਚਲ VPragmaType pragma = VPragmaType::COVERAGE_ਖੰਡ_OFF;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (ਸਤਰ_ਮੇਲ(flp->lineno(), pragma)) {
            nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstStmtPragma{flp, ਨਵਾਂ AstPragma{flp, pragma}});
        }
    }
    ਖਾਲੀ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ਅਚਲ ਪੂਰਨ lineno = nodep->fileline()->lineno();
        ਜੇ (ਸਤਰ_ਮੇਲ(lineno, VPragmaType::ਭਰਿਆ_CASE)) nodep->ਭਰਿਆ_Pragma(ਸੱਚ);
        ਜੇ (ਸਤਰ_ਮੇਲ(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(ਸੱਚ);
    }
    ਖਾਲੀ applyIgnores(File_ਸਤਰ* filelinep) {
        // HOT routine, called each parsed token line of this filename
        ਜੇ (filelinep->filenameno() == m_ਆਖਰੀ_Ignore.filenameno
            && filelinep->lineno() == m_ਆਖਰੀ_Ignore.lineno)
            ਵਾਪਸ;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        ਜੇ (filelinep->filenameno() == m_ਆਖਰੀ_Ignore.filenameno
            && filelinep->lineno() < m_ਆਖਰੀ_Ignore.ਅਗਲਾ_Change) {
            m_ਆਖਰੀ_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            ਵਾਪਸ;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_ਆਖਰੀ_Ignore.filenameno = filelinep->filenameno();
        m_ਆਖਰੀ_Ignore.lineno = filelinep->lineno();
        m_ਆਖਰੀ_Ignore.ਅਗਲਾ_Change = std::numeric_limits<ਪੂਰਨ>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_ਰੁੱਖ.ਲੱਭੋ(filelinep->lineno(), results /*ref*/, m_ਆਖਰੀ_Ignore.ਅਗਲਾ_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ bitset{VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::AllOnes{}};
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : results) {
            ਅਚਲ V3_Control_Ignores_ਸਤਰ& cign = control_Ign_ਸਤਰਾਂ[it];
            UASSERT(cign.ਵਿੱਚ_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_ਕੋਡ.forDelegateCodes(
                [&](V3_Error_ਕੋਡ subcode) { bitset.ਸੈੱਟ(subcode, cign.m_on); });
        }

        filelinep->warn_ਸੈੱਟ_Ctrl_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(bitset);
        m_ਆਖਰੀ_Ignore.ਅਗਲਾ_Change = std::max(m_ਆਖਰੀ_Ignore.ਅਗਲਾ_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_ਆਖਰੀ_Ignore.ਅਗਲਾ_Change
                                       << " Messages: " << bitset.ascii());
    }
    ਤਾਰਕਿਕ waive(V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ string& ਸੁਨੇਹਾ) {
        ਜੇ (ਕੋਡ.hardError()) ਵਾਪਸ ਝੂਠ;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_waivers) {
            ਜੇ ((ਕੋਡ.isUnder(itr.m_ਕੋਡ) || (itr.m_ਕੋਡ == V3_Error_ਕੋਡ::I_LINT))
                && VString::wildmatch(ਸੁਨੇਹਾ, itr.m_ਮੇਲ)
                && WildcardContents::resolve(itr.m_contents)) {
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }
};

ਵਰਤੋਂ V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

ਵਰਗ V3_Control_Scope_Trace_ਇੰਦਰਾਜ final {
ਜਨਤਕ:
    ਅਚਲ string m_scope;  // Scope or regexp to match
    ਅਚਲ ਤਾਰਕਿਕ m_on;  // True to enable message
    ਪੂਰਨ m_ਪੱਧਰ = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_ਇੰਦਰਾਜ(ਅਚਲ string& scope, ਤਾਰਕਿਕ on, ਪੂਰਨ ਪੱਧਰ)
        : m_scope{scope}
        , m_on{on}
        , m_ਪੱਧਰ{ਪੱਧਰ} {}
    ~V3_Control_Scope_Trace_ਇੰਦਰਾਜ() = ਮੂਲ;
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ V3_Control_Scope_Trace_ਇੰਦਰਾਜ& other) ਅਚਲ {
        ਜੇ (m_on < other.m_on) ਵਾਪਸ ਸੱਚ;
        ਜੇ (m_on > other.m_on) ਵਾਪਸ ਝੂਠ;
        ਜੇ (m_ਪੱਧਰ < other.m_ਪੱਧਰ) ਵਾਪਸ ਸੱਚ;
        ਜੇ (m_ਪੱਧਰ > other.m_ਪੱਧਰ) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
ਵਰਗ V3_Control_Scope_Trace_ਇੰਦਰਾਜ_ਮੇਲ final {
ਜਨਤਕ:
    ਅਚਲ V3_Control_Scope_Trace_ਇੰਦਰਾਜ* m_entryp;  // Entry the match refers to
    ਅਚਲ string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_ਇੰਦਰਾਜ_ਮੇਲ(ਅਚਲ V3_Control_Scope_Trace_ਇੰਦਰਾਜ* entryp, ਅਚਲ string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ V3_Control_Scope_Trace_ਇੰਦਰਾਜ_ਮੇਲ& other) ਅਚਲ {
        ਜੇ (m_entryp < other.m_entryp) ਵਾਪਸ ਸੱਚ;
        ਜੇ (m_entryp > other.m_entryp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ m_scopepart < other.m_scopepart;
    }
};

ਵਰਗ V3ControlScopeTraceResolver final {
    std::ਸਦਿਸ਼<V3_Control_Scope_Trace_ਇੰਦਰਾਜ> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_ਇੰਦਰਾਜ_ਮੇਲ, ਤਾਰਕਿਕ> m_ਮੇਲ_Cache;  // Matching entries for speed

ਜਨਤਕ:
    ਖਾਲੀ ਜੋੜੋ_Scope_Trace_On(ਤਾਰਕਿਕ on, ਅਚਲ string& scope, ਪੂਰਨ ਪੱਧਰ) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << ਪੱਧਰ);
        m_entries.emplace_ਪਿੱਠ(V3_Control_Scope_Trace_ਇੰਦਰਾਜ{scope, on, ਪੱਧਰ});
        m_ਮੇਲ_Cache.clear();
    }

    ਤਾਰਕਿਕ get_ਇੰਦਰਾਜ_ਮੇਲ(ਅਚਲ V3_Control_Scope_Trace_ਇੰਦਰਾਜ* entp, ਅਚਲ string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ਅਚਲ V3_Control_Scope_Trace_ਇੰਦਰਾਜ_ਮੇਲ ਕੁੰਜੀ{entp, scopepart};
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_ਮੇਲ_Cache.emplace(ਕੁੰਜੀ, ਝੂਠ);
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ = VString::wildmatch(scopepart, entp->m_scope);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਤਾਰਕਿਕ getScopeTraceOn(ਅਚਲ string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        ਪੂਰਨ maxLevel = 1;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ch : scope) {
            ਜੇ (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        ਤਾਰਕਿਕ enabled = ਸੱਚ;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            ਪੂਰਨ partLevel = 1;
            ਲਈ (string::ਮਾਪ_type part_ਸਿਰਾ = 0; ਸੱਚ;) {
                part_ਸਿਰਾ = scope.ਲੱਭੋ('.', part_ਸਿਰਾ + 1);
                ਜੇ (part_ਸਿਰਾ == string::npos) part_ਸਿਰਾ = scope.ਲੰਮਾਈ();
                ਅਚਲ std::string scopepart = scope.substr(0, part_ਸਿਰਾ);
                ਜੇ (get_ਇੰਦਰਾਜ_ਮੇਲ(&ent, scopepart)) {
                    ਅਚਲ ਤਾਰਕਿਕ level_ਮੇਲ
                        = !ent.m_ਪੱਧਰ || (ent.m_ਪੱਧਰ >= maxLevel - partLevel);
                    ਜੇ (level_ਮੇਲ) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_ਮੇਲ ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    ਰੋਕੋ;
                }
                ਜੇ (part_ਸਿਰਾ == scope.ਲੰਮਾਈ()) ਰੋਕੋ;
                ++partLevel;
            }
        }
        ਵਾਪਸ enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

ਵਰਗ V3_Control_Resolver_Hier_ਕਾਮਾ_ਇੰਦਰਾਜ final {
    ਅਚਲ ਪੂਰਨ m_workers;  // Number of workers requested for the hierarchical block
    File_ਸਤਰ* ਅਚਲ m_flp;  // Location the worker count was specified at

ਜਨਤਕ:
    ਸਪਸ਼ਟ V3_Control_Resolver_Hier_ਕਾਮਾ_ਇੰਦਰਾਜ(ਪੂਰਨ workers, File_ਸਤਰ* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    ਪੂਰਨ workers() ਅਚਲ { ਵਾਪਸ m_workers; }
    File_ਸਤਰ* flp() ਅਚਲ { ਵਾਪਸ m_flp; }
};

ਵਰਗ V3ControlResolver final {
    ਗਿਣਤੀ Profile_ਅੰਕੜੇ_ਬਹੁਲਕ : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_ਅੰਕੜੇ;  // Access to profile_data records
    uint8_t m_ਬਹੁਲਕ = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_ਕਾਮਾ_ਇੰਦਰਾਜ> m_hierWorkers;
    File_ਸਤਰ* m_profile_File_ਸਤਰ = ਖਾਲੀ_ਸੂਚਕ;  // Location profile_data was read from

    V3ControlResolver() = ਮੂਲ;
    ~V3ControlResolver() = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ V3ControlResolver& s() {
        ਸਥਿਰ V3ControlResolver s_singleton;
        ਵਾਪਸ s_singleton;
    }
    V3ControlModuleResolver& modules() { ਵਾਪਸ m_modules; }
    V3ControlFileResolver& files() { ਵਾਪਸ m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { ਵਾਪਸ m_scopeTraces; }

    ਖਾਲੀ ਜੋੜੋ_Profile_ਅੰਕੜੇ(File_ਸਤਰ* fl, ਅਚਲ string& hierDpi, uint64_t ਲਾਗਤ) {
        // Empty key for hierarchical DPI wrapper costs.
        ਜੋੜੋ_Profile_ਅੰਕੜੇ(fl, hierDpi, "", ਲਾਗਤ, HIER_DPI);
    }
    ਖਾਲੀ ਜੋੜੋ_Profile_ਅੰਕੜੇ(File_ਸਤਰ* fl, ਅਚਲ string& model, ਅਚਲ string& ਕੁੰਜੀ, uint64_t ਲਾਗਤ,
                        Profile_ਅੰਕੜੇ_ਬਹੁਲਕ ਬਹੁਲਕ = MTASK) {
        ਜੇ (!m_profile_File_ਸਤਰ) m_profile_File_ਸਤਰ = fl;
        ਜੇ (ਲਾਗਤ == 0) ਲਾਗਤ = 1;  // Cost 0 means delete (or no data)
        ਜੇ (ਬਹੁਲਕ == MTASK) {
            m_profile_ਅੰਕੜੇ[model][ਕੁੰਜੀ] += ਲਾਗਤ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਬਹੁਲਕ == HIER_DPI) {
            m_profile_ਅੰਕੜੇ[model][ਕੁੰਜੀ] = std::max(m_profile_ਅੰਕੜੇ[model][ਕੁੰਜੀ], ਲਾਗਤ);
        }
        m_ਬਹੁਲਕ |= ਬਹੁਲਕ;
    }
    ਤਾਰਕਿਕ ਸ਼ਾਮਲ_ਹੈ_MTask_Profile_ਅੰਕੜੇ() ਅਚਲ { ਵਾਪਸ m_ਬਹੁਲਕ & MTASK; }
    uint64_t get_Profile_ਅੰਕੜੇ(ਅਚਲ string& hierDpi) ਅਚਲ {
        // Empty key for hierarchical DPI wrapper costs.
        ਵਾਪਸ get_Profile_ਅੰਕੜੇ(hierDpi, "");
    }
    ਖਾਲੀ ਜੋੜੋ_Hier_Workers(File_ਸਤਰ* flp, ਅਚਲ string& model, ਪੂਰਨ workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    ਪੂਰਨ getHierWorkers(ਅਚਲ string& model) ਅਚਲ {
        ਅਚਲ ਸਵੈਚਾਲਿਤ mit = m_hierWorkers.ਲੱਭੋ(model);
        // Assign a single worker if no specified.
        ਵਾਪਸ mit != m_hierWorkers.cend() ? mit->ਦੂਜਾ.workers() : 0;
    }
    File_ਸਤਰ* get_Hier_Workers_File_ਸਤਰ(ਅਚਲ string& model) ਅਚਲ {
        ਅਚਲ ਸਵੈਚਾਲਿਤ mit = m_hierWorkers.ਲੱਭੋ(model);
        ਵਾਪਸ mit != m_hierWorkers.cend() ? mit->ਦੂਜਾ.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_ਅੰਕੜੇ(ਅਚਲ string& model, ਅਚਲ string& ਕੁੰਜੀ) ਅਚਲ {
        ਅਚਲ ਸਵੈਚਾਲਿਤ mit = m_profile_ਅੰਕੜੇ.ਲੱਭੋ(model);
        ਜੇ (mit == m_profile_ਅੰਕੜੇ.cend()) ਵਾਪਸ 0;
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = mit->ਦੂਜਾ.ਲੱਭੋ(ਕੁੰਜੀ);
        ਜੇ (it == mit->ਦੂਜਾ.cend()) ਵਾਪਸ 0;
        ਵਾਪਸ it->ਦੂਜਾ;
    }
    File_ਸਤਰ* get_Profile_ਅੰਕੜੇ_File_ਸਤਰ() ਅਚਲ { ਵਾਪਸ m_profile_File_ਸਤਰ; }  // Maybe null
    ਸਥਿਰ uint64_t get_Current_Hier_ਖੰਡ_ਲਾਗਤ() {
        ਜੇ (uint64_t ਲਾਗਤ = V3Control::get_Profile_ਅੰਕੜੇ(v3Global.opt.ਅਗੇਤਰ())) {
            UINFO(9, "Fetching cost from profile info: " << ਲਾਗਤ);
            ਵਾਪਸ ਲਾਗਤ;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
        uint64_t ਲਾਗਤ = 0;
        ਲਈ (ਪੂਰਨ i = 0; i < VEval::ENUM_ਸਿਰਾ; ++i) {
            ਅਚਲ VEval eval{i};
            ਜੇ (eval.slow()) ਜਾਰੀ;
            ਲਾਗਤ += V3_Instr_ਗਿਣਤੀ::ਗਿਣਤੀ_2(netlistp->evalFuncp(eval), ਝੂਠ);
        }
        UINFO(9, "Evaluating cost: " << ਲਾਗਤ);
        ਵਾਪਸ ਲਾਗਤ;
    }
};

//######################################################################
// V3Control

ਖਾਲੀ V3Control::ਜੋੜੋ_Case_ਭਰਿਆ(ਅਚਲ string& filename, ਪੂਰਨ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.ਜੋੜੋ_ਸਤਰ_Attribute(lineno, VPragmaType::ਭਰਿਆ_CASE);
}
ਖਾਲੀ V3Control::ਜੋੜੋ_Case_Parallel(ਅਚਲ string& filename, ਪੂਰਨ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.ਜੋੜੋ_ਸਤਰ_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
ਖਾਲੀ V3Control::ਜੋੜੋ_Coverage_ਖੰਡ_Off(ਅਚਲ string& filename, ਪੂਰਨ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.ਜੋੜੋ_ਸਤਰ_Attribute(lineno, VPragmaType::COVERAGE_ਖੰਡ_OFF);
}
ਖਾਲੀ V3Control::ਜੋੜੋ_Coverage_ਖੰਡ_Off(ਅਚਲ string& module, ਅਚਲ string& blockname) {
    V3ControlResolver::s().modules().at(module).ਜੋੜੋ_Coverage_ਖੰਡ_Off(blockname);
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Hier_Workers(File_ਸਤਰ* fl, ਅਚਲ string& model, ਪੂਰਨ workers) {
    V3ControlResolver::s().ਜੋੜੋ_Hier_Workers(fl, model, workers);
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Fsm_ਰਜਿਸਟਰ_Wrapper(File_ਸਤਰ* fl, ਅਚਲ string& module, ਅਚਲ string& d,
                                      ਅਚਲ string& q, ਅਚਲ string& clock, ਅਚਲ string& reset,
                                      ਅਚਲ string& reset_ਮੁੱਲ) {
    string missing;
    ਜੇ (module.ਖਾਲੀ_ਥਾਵਾਂ()) missing += "-module";
    ਜੇ (d.ਖਾਲੀ_ਥਾਵਾਂ()) missing = VString::dot(missing, ", ", "-d");
    ਜੇ (q.ਖਾਲੀ_ਥਾਵਾਂ()) missing = VString::dot(missing, ", ", "-q");
    ਜੇ (clock.ਖਾਲੀ_ਥਾਵਾਂ()) missing = VString::dot(missing, ", ", "-clock");
    ਜੇ (!missing.ਖਾਲੀ_ਥਾਵਾਂ()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        ਵਾਪਸ;
    }

    ਅਚਲ Fsm_ਰਜਿਸਟਰ_Wrapper desc{module, d, q, clock, reset, reset_ਮੁੱਲ};
    V3ControlResolver::s().modules().at(module).ਸੈੱਟ_Fsm_ਰਜਿਸਟਰ_Wrapper(fl, desc);
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Ignore(V3_Error_ਕੋਡ ਕੋਡ, ਤਾਰਕਿਕ on, ਅਚਲ string& filename, ਪੂਰਨ min, ਪੂਰਨ max) {
    UINFO(9, "addIgnore " << ਕੋਡ << " " << min << "-" << max << " fn=" << filename);
    ਜੇ (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_ਸਤਰ::globalWarnOff(ਕੋਡ, !on);
    } ਨਹੀਂਤਾਂ {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).ਜੋੜੋ_Ignore(ਕੋਡ, min, max, on);
    }
}
ਖਾਲੀ V3Control::ਜੋੜੋ_Ignore_ਮੇਲ(V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ string& filename, ਅਚਲ string& contents,
                               ਅਚਲ string& ਮੇਲ) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).ਜੋੜੋ_Ignore_ਮੇਲ(ਕੋਡ, contents, ਮੇਲ);
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Inline(File_ਸਤਰ* fl, ਅਚਲ string& module, ਅਚਲ string& ftask, ਤਾਰਕਿਕ on) {
    ਜੇ (ftask.ਖਾਲੀ_ਥਾਵਾਂ()) {
        V3ControlResolver::s().modules().at(module).ਸੈੱਟ_Inline(on);
    } ਨਹੀਂਤਾਂ {
        ਜੇ (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } ਨਹੀਂਤਾਂ {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).ਸੈੱਟ_No_Inline(on);
        }
    }
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Module_Pragma(ਅਚਲ string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).ਜੋੜੋ_Module_Pragma(pragma);
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Profile_ਅੰਕੜੇ(File_ਸਤਰ* fl, ਅਚਲ string& hierDpi, uint64_t ਲਾਗਤ) {
    V3ControlResolver::s().ਜੋੜੋ_Profile_ਅੰਕੜੇ(fl, hierDpi, ਲਾਗਤ);
}
ਖਾਲੀ V3Control::ਜੋੜੋ_Profile_ਅੰਕੜੇ(File_ਸਤਰ* fl, ਅਚਲ string& model, ਅਚਲ string& ਕੁੰਜੀ,
                               uint64_t ਲਾਗਤ) {
    V3ControlResolver::s().ਜੋੜੋ_Profile_ਅੰਕੜੇ(fl, model, ਕੁੰਜੀ, ਲਾਗਤ);
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Scope_Trace_On(ਤਾਰਕਿਕ on, ਅਚਲ string& scope, ਪੂਰਨ ਪੱਧਰ) {
    V3ControlResolver::s().scopeTraces().ਜੋੜੋ_Scope_Trace_On(on, scope, ਪੱਧਰ);
}

ਖਾਲੀ V3Control::ਜੋੜੋ_Var_Attr(File_ਸਤਰ* fl, ਅਚਲ string& module, ਅਚਲ string& ftask,
                           Var_Spec_ਕਿਸਮ ਕਿਸਮ, ਅਚਲ string& ਨਮੂਨਾ_2, VAttrType attr,
                           Ast_Sen_ਰੁੱਖ* sensep) {
    ਜੇ (sensep) {
        File_ਸਤਰ* ਅਚਲ flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_ਰੁੱਖ(), sensep);
        // Used to be only accepted on public_flat_rw
        ਜੇ (attr != VAttrType::VAR_ਜਨਤਕ_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            ਵਾਪਸ;
        }
    }

    ਜੇ (ਕਿਸਮ != Var_Spec_ਕਿਸਮ::VAR) {
        ਚੋਣ (attr) {
        ਹਾਲਤ VAttrType::VAR_ਜਨਤਕ_FLAT:
        ਹਾਲਤ VAttrType::VAR_ਜਨਤਕ_FLAT_RD:
        ਹਾਲਤ VAttrType::VAR_ਜਨਤਕ_FLAT_RW: ਰੋਕੋ;
        ਮੂਲ:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            ਵਾਪਸ;
        }
    }

    // Semantics: Most of the attributes operate on signals
    ਜੇ (ਨਮੂਨਾ_2.ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (attr == VAttrType::VAR_ਜਨਤਕ) {
            ਜੇ (ftask.ਖਾਲੀ_ਥਾਵਾਂ()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).ਜੋੜੋ_Module_Pragma(
                    VPragmaType::ਜਨਤਕ_MODULE);
            } ਨਹੀਂਤਾਂ {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).ਸੈੱਟ_ਜਨਤਕ(ਸੱਚ);
            }
        } ਨਹੀਂਤਾਂ {
            fl->v3error("missing -var");
        }
    } ਨਹੀਂਤਾਂ {
        ਜੇ (attr == VAttrType::VAR_FORCEABLE) {
            ਜੇ (module.ਖਾਲੀ_ਥਾਵਾਂ()) {
                fl->v3error("forceable missing -module");
            } ਨਹੀਂਤਾਂ ਜੇ (!ftask.ਖਾਲੀ_ਥਾਵਾਂ()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } ਨਹੀਂਤਾਂ {
                V3ControlResolver::s().modules().at(module).vars().at(ਨਮੂਨਾ_2).ਜੋੜੋ(attr);
            }
        } ਨਹੀਂਤਾਂ {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                ਜੇ (ftask.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    ਜੇ (ਕਿਸਮ == Var_Spec_ਕਿਸਮ::PARAM) ਵਾਪਸ mod.params().at(ਨਮੂਨਾ_2);
                    ਜੇ (ਕਿਸਮ == Var_Spec_ਕਿਸਮ::ਪੋਰਟ) ਵਾਪਸ mod.ports().at(ਨਮੂਨਾ_2);
                    UASSERT_OBJ(ਕਿਸਮ == Var_Spec_ਕਿਸਮ::VAR, fl, "Unexpected VarSpecKind");
                    ਵਾਪਸ mod.vars().at(ਨਮੂਨਾ_2);
                }
                ਜੇ (ਕਿਸਮ == Var_Spec_ਕਿਸਮ::PARAM) ਵਾਪਸ mod.ftasks().at(ftask).params().at(ਨਮੂਨਾ_2);
                ਜੇ (ਕਿਸਮ == Var_Spec_ਕਿਸਮ::ਪੋਰਟ) ਵਾਪਸ mod.ftasks().at(ftask).ports().at(ਨਮੂਨਾ_2);
                UASSERT_OBJ(ਕਿਸਮ == Var_Spec_ਕਿਸਮ::VAR, fl, "Unexpected VarSpecKind");
                ਵਾਪਸ mod.ftasks().at(ftask).vars().at(ਨਮੂਨਾ_2);
            }();
            controlVar.ਜੋੜੋ(attr);
        }
    }
}

ਖਾਲੀ V3Control::applyCase(AstCase* nodep) {
    ਅਚਲ string& filename = nodep->fileline()->filename();
    V3ControlFile* ਅਚਲ filep = V3ControlResolver::s().files().resolve(filename);
    ਜੇ (filep) filep->applyCase(nodep);
}

ਖਾਲੀ V3Control::apply_Coverage_ਖੰਡ(Ast_ਗੰਢ_Module* modulep, AstBegin* nodep) {
    ਅਚਲ string& filename = nodep->fileline()->filename();
    V3ControlFile* ਅਚਲ filep = V3ControlResolver::s().files().resolve(filename);
    ਜੇ (filep) filep->apply_ਖੰਡ(nodep);
    ਅਚਲ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ਅਚਲ modp = V3ControlResolver::s().modules().resolve(modname);
    ਜੇ (modp) modp->apply_ਖੰਡ(nodep);
}
ਖਾਲੀ V3Control::apply_Coverage_ਖੰਡ(Ast_ਗੰਢ_Module* modulep, Ast_Gen_ਖੰਡ* nodep) {
    ਅਚਲ string& filename = nodep->fileline()->filename();
    V3ControlFile* ਅਚਲ filep = V3ControlResolver::s().files().resolve(filename);
    ਜੇ (filep) filep->apply_ਖੰਡ(nodep);
    ਅਚਲ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ਅਚਲ modp = V3ControlResolver::s().modules().resolve(modname);
    ਜੇ (modp) modp->apply_ਖੰਡ(nodep);
}

ਖਾਲੀ V3Control::applyIgnores(File_ਸਤਰ* filelinep) {
    ਅਚਲ string& filename = filelinep->filename();
    V3ControlFile* ਅਚਲ filep = V3ControlResolver::s().files().resolve(filename);
    ਜੇ (filep) filep->applyIgnores(filelinep);
}

ਖਾਲੀ V3Control::applyModule(Ast_ਗੰਢ_Module* modulep) {
    ਅਚਲ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ਅਚਲ modp = V3ControlResolver::s().modules().resolve(modname);
    ਜੇ (modp) modp->apply(modulep);
}

ਖਾਲੀ V3Control::applyFTask(Ast_ਗੰਢ_Module* modulep, Ast_ਗੰਢ_FTask* ftaskp) {
    ਅਚਲ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ਅਚਲ modp = V3ControlResolver::s().modules().resolve(modname);
    ਜੇ (!modp) ਵਾਪਸ;
    ਅਚਲ V3ControlFTask* ਅਚਲ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    ਜੇ (ftp) ftp->apply(ftaskp);
}

ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Resolver>
ਸਥਿਰ ਖਾਲੀ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ਅਚਲ std::string name = varp->prettyDehashOrigOrName();
    ਜੇ (ਅਚਲ V3ControlVar* ਅਚਲ vp = resolver.vars().resolve(name)) vp->apply(varp);
    ਜੇ (varp->isParam()) {
        ਜੇ (ਅਚਲ V3ControlVar* ਅਚਲ vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    ਜੇ (varp->isIO()) {
        ਜੇ (ਅਚਲ V3ControlVar* ਅਚਲ vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

ਖਾਲੀ V3Control::applyVarAttr(ਅਚਲ Ast_ਗੰਢ_Module* modulep, ਅਚਲ Ast_ਗੰਢ_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ਅਚਲ modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    ਜੇ (!modp) ਵਾਪਸ;
    ਜੇ (ftaskp) {
        V3ControlFTask* ਅਚਲ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        ਜੇ (!ftp) ਵਾਪਸ;
        resolveThenApply(*ftp, varp);
    } ਨਹੀਂਤਾਂ {
        resolveThenApply(*modp, varp);
    }
}

ਪੂਰਨ V3Control::getHierWorkers(ਅਚਲ string& model) {
    ਵਾਪਸ V3ControlResolver::s().getHierWorkers(model);
}
File_ਸਤਰ* V3Control::get_Hier_Workers_File_ਸਤਰ(ਅਚਲ string& model) {
    ਵਾਪਸ V3ControlResolver::s().get_Hier_Workers_File_ਸਤਰ(model);
}
ਅਚਲ V3Control::Fsm_ਰਜਿਸਟਰ_Wrapper* V3Control::get_Fsm_ਰਜਿਸਟਰ_Wrapper(ਅਚਲ string& module) {
    V3ControlModule* ਅਚਲ modp = V3ControlResolver::s().modules().resolve(module);
    ਵਾਪਸ modp ? modp->fsm_ਰਜਿਸਟਰ_Wrapperp() : ਖਾਲੀ_ਸੂਚਕ;
}
uint64_t V3Control::get_Profile_ਅੰਕੜੇ(ਅਚਲ string& hierDpi) {
    ਵਾਪਸ V3ControlResolver::s().get_Profile_ਅੰਕੜੇ(hierDpi);
}
uint64_t V3Control::get_Profile_ਅੰਕੜੇ(ਅਚਲ string& model, ਅਚਲ string& ਕੁੰਜੀ) {
    ਵਾਪਸ V3ControlResolver::s().get_Profile_ਅੰਕੜੇ(model, ਕੁੰਜੀ);
}
File_ਸਤਰ* V3Control::get_Profile_ਅੰਕੜੇ_File_ਸਤਰ() {
    ਵਾਪਸ V3ControlResolver::s().get_Profile_ਅੰਕੜੇ_File_ਸਤਰ();
}
ਤਾਰਕਿਕ V3Control::getScopeTraceOn(ਅਚਲ string& scope) {
    ਵਾਪਸ V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

ਖਾਲੀ V3Control::contents_ਧੱਕੋ_ਪਾਠ(ਅਚਲ string& ਪਾਠ) { WildcardContents::ਧੱਕੋ_ਪਾਠ(ਪਾਠ); }

ਤਾਰਕਿਕ V3Control::ਸ਼ਾਮਲ_ਹੈ_MTask_Profile_ਅੰਕੜੇ() {
    ਵਾਪਸ V3ControlResolver::s().ਸ਼ਾਮਲ_ਹੈ_MTask_Profile_ਅੰਕੜੇ();
}
uint64_t V3Control::get_Current_Hier_ਖੰਡ_ਲਾਗਤ() {
    ਵਾਪਸ V3ControlResolver::s().get_Current_Hier_ਖੰਡ_ਲਾਗਤ();
}

ਤਾਰਕਿਕ V3Control::waive(ਅਚਲ File_ਸਤਰ* filelinep, V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ string& ਸੁਨੇਹਾ) {
    V3ControlFile* ਅਚਲ filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    ਜੇ (!filep) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ filep->waive(ਕੋਡ, ਸੁਨੇਹਾ);
}

ਖਾਲੀ V3Control::selfTest() { VInterval_ਰੁੱਖ::selfTest(); }
