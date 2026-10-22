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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Control.h"

#അന്തര്ഭാവ "V3InstrCount.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3String.h"

#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
വർഗ്ഗം V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<സ്ഥിരം std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<സ്ഥിരം std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

പൊതു:
    V3ControlWildcardResolver() = സ്ഥിരസ്ഥിതി;
    ~V3ControlWildcardResolver() = സ്ഥിരസ്ഥിതി;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    ശൂന്യം update(സ്ഥിരം V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        സ്ഥിരം V3LockGuard lock{m_mutex};
        സ്ഥിരം V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        വേണ്ടി (സ്ഥിരം auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(സ്ഥിരം string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        സ്ഥിരം V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        തിരികെ m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(സ്ഥിരം string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        സ്ഥിരം V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        സ്ഥിരം auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        എങ്കിൽ (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            സ്ഥിരം string slashName = V3Os::filenameSlashPath(name);
            വേണ്ടി (സ്ഥിരം auto& patEnt : m_mapPatterns) {
                എങ്കിൽ (VString::wildmatch(slashName, patEnt.first)) {
                    എങ്കിൽ (!entryr) entryr.reset(പുതിയ T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            എങ്കിൽ (entryr) entryr->updateFinalize();
        }
        തിരികെ entryr.get();
    }
};

// List of attributes for variables
വർഗ്ഗം V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
പൊതു:
    // Add new attribugte
    ശൂന്യം add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    ശൂന്യം update(സ്ഥിരം V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.തിരുകുക(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    ശൂന്യം updateFinalize() {}
    // Apply all attributes to the variable
    ശൂന്യം apply(AstVar* varp) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം VAttrType attr : m_attrs) {
            varp->addAttrsp(പുതിയ AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

വർഗ്ഗം WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<സ്ഥിരം std::string, തർക്കം> m_mapPatterns;  // Pattern match results
    std::deque<string> m_വരികൾ;  // Source text lines

    // METHODS
    സ്ഥിതം WildcardContents& s() {  // Singleton
        സ്ഥിതം WildcardContents s_s;
        തിരികെ s_s;
    }
    ശൂന്യം clearCacheImp() { m_mapPatterns.clear(); }
    ശൂന്യം തള്ളുക_Text_Imp(സ്ഥിരം string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        സ്ഥിരം string leftover = m_വരികൾ.back() + text;
        m_വരികൾ.എടുക്കുക_back();

        // Insert line-by-line
        string::size_type വരി_start = 0;
        ആകുമ്പോൾ (സത്യം) {
            സ്ഥിരം string::size_type വരി_end = leftover.find('\n', വരി_start);
            എങ്കിൽ (വരി_end != string::npos) {
                സ്ഥിരം string oneline(leftover, വരി_start, വരി_end - വരി_start + 1);
                എങ്കിൽ (oneline.size() > 1) m_വരികൾ.തള്ളുക_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_വരികൾ.size() - 1) << "]: " << oneline);
                വരി_start = വരി_end + 1;
            } അല്ലെങ്കിൽ {
                നിർത്തുക;
            }
        }
        // Keep leftover for next time
        m_വരികൾ.emplace_back(string(leftover, വരി_start));  // Might be ""
        clearCacheImp();
    }

    തർക്കം resolveUncachedImp(സ്ഥിരം string& contentsRegexp) {
        വേണ്ടി (സ്ഥിരം string& i : m_വരികൾ) {
            എങ്കിൽ (VString::wildmatch(i, contentsRegexp)) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    തർക്കം resolveCachedImp(സ്ഥിരം string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        എങ്കിൽ (contentsRegexp.empty()) തിരികെ സത്യം;
        സ്ഥിരം auto pair = m_mapPatterns.emplace(contentsRegexp, അസത്യം);
        തർക്കം& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        എങ്കിൽ (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        തിരികെ entryr;
    }

പൊതു:
    WildcardContents() {
        m_വരികൾ.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = സ്ഥിരസ്ഥിതി;
    // Return true iff contentsRegexp in parsed contents
    സ്ഥിതം തർക്കം resolve(സ്ഥിരം string& contentsRegexp) {
        തിരികെ s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    സ്ഥിതം ശൂന്യം തള്ളുക_Text(സ്ഥിരം string& text) { s().തള്ളുക_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

വർഗ്ഗം V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    തർക്കം m_noinline = അസത്യം;  // Don't inline function/task
    തർക്കം m_public = അസത്യം;  // Public function/task

പൊതു:
    V3ControlFTask() = സ്ഥിരസ്ഥിതി;
    ശൂന്യം update(സ്ഥിരം V3ControlFTask& f) {
        // Don't overwrite true with false
        എങ്കിൽ (f.m_noinline) m_noinline = സത്യം;
        എങ്കിൽ (f.m_public) m_public = സത്യം;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    ശൂന്യം updateFinalize() {}

    V3ControlVarResolver& params() { തിരികെ m_params; }
    V3ControlVarResolver& ports() { തിരികെ m_ports; }
    V3ControlVarResolver& vars() { തിരികെ m_vars; }

    ശൂന്യം setNoInline(തർക്കം set) { m_noinline = set; }
    ശൂന്യം setPublic(തർക്കം set) { m_public = set; }

    ശൂന്യം apply(Ast_കണ്ണി_FTask* ftaskp) സ്ഥിരം {
        എങ്കിൽ (m_noinline)
            ftaskp->addStmtsp(പുതിയ AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        എങ്കിൽ (m_public)
            ftaskp->addStmtsp(പുതിയ AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

വർഗ്ഗം V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    തർക്കം m_hasFsmRegisterWrapper = അസത്യം;  // Whether descriptor has been set
    തർക്കം m_inline = അസത്യം;  // Whether to force the inline
    തർക്കം m_inline_മൂല്യം = അസത്യം;  // The inline value (on/off)

പൊതു:
    V3ControlModule() = സ്ഥിരസ്ഥിതി;

    ശൂന്യം update(സ്ഥിരം V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        എങ്കിൽ (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = സത്യം;
        }
        വേണ്ടി (സ്ഥിരം string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.തിരുകുക(i);
        എങ്കിൽ (!m_inline) {
            m_inline = m.m_inline;
            m_inline_മൂല്യം = m.m_inline_മൂല്യം;
        }
        വേണ്ടി (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.തിരുകുക(*it);
        }
    }
    ശൂന്യം updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { തിരികെ m_tasks; }
    V3ControlVarResolver& params() { തിരികെ m_params; }
    V3ControlVarResolver& ports() { തിരികെ m_ports; }
    V3ControlVarResolver& vars() { തിരികെ m_vars; }

    ശൂന്യം addCoverageBlockOff(സ്ഥിരം string& name) { m_coverageOffBlocks.തിരുകുക(name); }
    ശൂന്യം setInline(തർക്കം set) {
        m_inline = സത്യം;
        m_inline_മൂല്യം = set;
    }
    ശൂന്യം setFsmRegisterWrapper(File_വരി* fl, സ്ഥിരം V3Control::FsmRegisterWrapper& desc) {
        എങ്കിൽ (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_കണ്ണി::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = സത്യം;
    }
    സ്ഥിരം V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() സ്ഥിരം {
        തിരികെ m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    ശൂന്യം addModulePragma(VPragmaType pragma) { m_modPragmas.തിരുകുക(pragma); }

    ശൂന്യം apply(Ast_കണ്ണി_Module* modp) {
        എങ്കിൽ (m_inline) {
            സ്ഥിരം VPragmaType type
                = m_inline_മൂല്യം ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_കണ്ണി* സ്ഥിരം nodep = പുതിയ AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        വേണ്ടി (സ്ഥിരം auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            എങ്കിൽ (itr == VPragmaType::HIER_PARAMS) modp->hierParams(സത്യം);
            Ast_കണ്ണി* സ്ഥിരം nodep = പുതിയ AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    ശൂന്യം applyBlock(AstGenBlock* nodep) {
        സ്ഥിരം VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        എങ്കിൽ (!nodep->unnamed()) {
            വേണ്ടി (സ്ഥിരം string& i : m_coverageOffBlocks) {
                എങ്കിൽ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(പുതിയ AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    ശൂന്യം applyBlock(Ast_കണ്ണി_Block* nodep) {
        സ്ഥിരം VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (!nodep->unnamed()) {
            വേണ്ടി (സ്ഥിരം string& i : m_coverageOffBlocks) {
                എങ്കിൽ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(പുതിയ AstStmtPragma{flp, പുതിയ AstPragma{flp, pragma}});
                }
            }
        }
    }
};

using V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
വർഗ്ഗം V3_Control_Ignores_വരി final {
പൊതു:
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_വരി_Min;  // Minimum line number to make change at
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_വരി_Max;  // Maximum line number to make change at (inclusive)
    സ്ഥിരം V3ErrorCode m_code;  // Error code
    സ്ഥിരം തർക്കം m_on;  // True to enable message
    V3_Control_Ignores_വരി(V3ErrorCode code, പൂർണ്ണസംഖ്യ linemin, പൂർണ്ണസംഖ്യ linemax, തർക്കം on)
        : m_വരി_Min{linemin}
        , m_വരി_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_വരി() = സ്ഥിരസ്ഥിതി;
    തർക്കം every_വരി() സ്ഥിരം { തിരികെ m_വരി_Min == 0 && m_വരി_Max == 0; }
    തർക്കം inRange(പൂർണ്ണസംഖ്യ lineno) സ്ഥിരം {
        തിരികെ (lineno >= m_വരി_Min && lineno <= m_വരി_Max) || every_വരി();
    }
};
std::ostream& operator<<(std::ostream& os, സ്ഥിരം V3_Control_Ignores_വരി& rhs) {
    തിരികെ os << rhs.m_വരി_Min << "-" << rhs.m_വരി_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
സ്ഥിതം std::vector<V3_Control_Ignores_വരി> control_Ign_വരികൾ;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

വർഗ്ഗം VInterval_വൃക്ഷം final {
    ഘടന Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        പൂർണ്ണസംഖ്യ m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(പൂർണ്ണസംഖ്യ center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_വരികൾ;  // All-line disables

സ്വകാര്യ:
    std::unique_ptr<Entry> build_വൃക്ഷം(സ്ഥിരം IgnIndices& points) {
        എങ്കിൽ (points.empty()) തിരികെ nullptr;

        പൂർണ്ണസംഖ്യ minval = std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();
        പൂർണ്ണസംഖ്യ maxval = std::numeric_limits<പൂർണ്ണസംഖ്യ>::min();
        വേണ്ടി (സ്ഥിരം auto& it : points) {
            സ്ഥിരം V3_Control_Ignores_വരി& cign = control_Ign_വരികൾ[it];
            എങ്കിൽ (cign.every_വരി()) {
                m_every_വരികൾ.emplace_back(it);
                തുടരുക;
            }
            minval = std::min(minval, cign.m_വരി_Min);
            maxval = std::max(maxval, cign.m_വരി_Max);
        }

        സ്ഥിരം പൂർണ്ണസംഖ്യ center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(പുതിയ Entry{center});

        IgnIndices ഇടത്_Points;
        IgnIndices വലത്_Points;
        വേണ്ടി (സ്ഥിരം auto& it : points) {
            സ്ഥിരം V3_Control_Ignores_വരി& cign = control_Ign_വരികൾ[it];
            എങ്കിൽ (cign.every_വരി()) തുടരുക;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            എങ്കിൽ (cign.m_വരി_Max < center)
                ഇടത്_Points.emplace_back(it);
            അല്ലെങ്കിൽ എങ്കിൽ (cign.m_വരി_Min > center)
                വലത്_Points.emplace_back(it);
            അല്ലെങ്കിൽ {
                entp->m_byMin.തള്ളുക_back(it);
                entp->m_byMax.തള്ളുക_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            തിരികെ control_Ign_വരികൾ[a].m_വരി_Min < control_Ign_വരികൾ[b].m_വരി_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            തിരികെ control_Ign_വരികൾ[a].m_വരി_Max > control_Ign_വരികൾ[b].m_വരി_Max;
        });

        entp->m_leftp = build_വൃക്ഷം(ഇടത്_Points);
        entp->m_rightp = build_വൃക്ഷം(വലത്_Points);
        തിരികെ entp;
    }

    ശൂന്യം find_വൃക്ഷം(പൂർണ്ണസംഖ്യ lineno, Entry* entp, IgnIndices& resultsr, പൂർണ്ണസംഖ്യ& nextChanger) സ്ഥിരം {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        എങ്കിൽ (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            വേണ്ടി (സ്ഥിരം auto& it : entp->m_byMin) {
                സ്ഥിരം V3_Control_Ignores_വരി& cign = control_Ign_വരികൾ[it];
                എങ്കിൽ (cign.m_വരി_Min <= lineno) {
                    resultsr.emplace_back(it);
                } അല്ലെങ്കിൽ {
                    nextChanger = std::min(nextChanger, cign.m_വരി_Min);
                    നിർത്തുക;
                }
            }
            എങ്കിൽ (entp->m_leftp) find_വൃക്ഷം(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } അല്ലെങ്കിൽ എങ്കിൽ (lineno > entp->m_center) {
            വേണ്ടി (സ്ഥിരം auto& it : entp->m_byMax) {
                സ്ഥിരം V3_Control_Ignores_വരി& cign = control_Ign_വരികൾ[it];
                എങ്കിൽ (cign.m_വരി_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_വരി_Max + 1);
                    resultsr.emplace_back(it);
                } അല്ലെങ്കിൽ {
                    നിർത്തുക;
                }
            }
            എങ്കിൽ (entp->m_rightp) find_വൃക്ഷം(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } അല്ലെങ്കിൽ {
            വേണ്ടി (സ്ഥിരം auto& it : entp->m_byMin) {
                സ്ഥിരം V3_Control_Ignores_വരി& cign = control_Ign_വരികൾ[it];
                resultsr.emplace_back(it);
                എങ്കിൽ (cign.m_വരി_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_വരി_Max + 1);
            }
            എങ്കിൽ (entp->m_leftp) find_വൃക്ഷം(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            എങ്കിൽ (entp->m_rightp) find_വൃക്ഷം(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

പൊതു:
    // CONSTRUCTORS
    VInterval_വൃക്ഷം() = സ്ഥിരസ്ഥിതി;
    ~VInterval_വൃക്ഷം() = സ്ഥിരസ്ഥിതി;
    // METHODS
    ശൂന്യം clear() { m_rootp = nullptr; }
    ശൂന്യം build(സ്ഥിരം IgnIndices& points) {
        clear();
        m_rootp = build_വൃക്ഷം(points);
    }
    ശൂന്യം find(പൂർണ്ണസംഖ്യ lineno, IgnIndices& resultsr, പൂർണ്ണസംഖ്യ& nextChanger) സ്ഥിരം {
        resultsr = m_every_വരികൾ;
        nextChanger = std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();

        എങ്കിൽ (m_rootp) find_വൃക്ഷം(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    സ്ഥിതം ശൂന്യം selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        വേണ്ടി (സ്ഥിരം auto& it : points) {
            control_Ign_വരികൾ.emplace_back(
                V3_Control_Ignores_വരി{V3ErrorCode::I_LINT, it.first, it.second, സത്യം});
            data.emplace_back(static_cast<uint32_t>(control_Ign_വരികൾ.size() - 1));
        }
        VInterval_വൃക്ഷം വൃക്ഷം;
        വൃക്ഷം.build(data);

        IgnIndices results;
        പൂർണ്ണസംഖ്യ nextChange = 0;
        വൃക്ഷം.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        വൃക്ഷം.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        വൃക്ഷം.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        വൃക്ഷം.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        വൃക്ഷം.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        വൃക്ഷം.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        വൃക്ഷം.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        വൃക്ഷം.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<പൂർണ്ണസംഖ്യ>::max());
        //
        points = {{0, 0}};
        വേണ്ടി (സ്ഥിരം auto& it : points) {
            control_Ign_വരികൾ.emplace_back(
                V3_Control_Ignores_വരി{V3ErrorCode::I_LINT, it.first, it.second, സത്യം});
            data.emplace_back(static_cast<uint32_t>(control_Ign_വരികൾ.size() - 1));
        }
        വൃക്ഷം.build(data);
        //
        വൃക്ഷം.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_വരി_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

വർഗ്ഗം WaiverSetting final {
പൊതു:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, സ്ഥിരം string& contents, സ്ഥിരം string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = സ്ഥിരസ്ഥിതി;
    WaiverSetting& operator=(സ്ഥിരം WaiverSetting& rhs) = സ്ഥിരസ്ഥിതി;
};

// File entity
വർഗ്ഗം V3ControlFile final {
    using വരി_Attr_Map = std::map<പൂർണ്ണസംഖ്യ, V3_Control_വരി_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    വരി_Attr_Map m_വരി_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_വൃക്ഷം m_interval_വൃക്ഷം;  // Tree of indices to IgnLines

    ഘടന {
        പൂർണ്ണസംഖ്യ filenameno = -1;  // Last filename
        പൂർണ്ണസംഖ്യ lineno = -1;  // Last linenumber
        പൂർണ്ണസംഖ്യ nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    തർക്കം വരി_Match(പൂർണ്ണസംഖ്യ lineno, VPragmaType type) {
        എങ്കിൽ (m_വരി_Attrs.find(0) != m_വരി_Attrs.end() && m_വരി_Attrs[0][type]) തിരികെ സത്യം;
        എങ്കിൽ (m_വരി_Attrs.find(lineno) == m_വരി_Attrs.end()) തിരികെ അസത്യം;
        തിരികെ m_വരി_Attrs[lineno][type];
    }

പൊതു:
    V3ControlFile() = സ്ഥിരസ്ഥിതി;
    ശൂന്യം update(സ്ഥിരം V3ControlFile& file) {
        // Copy in all attributes and waivers
        വേണ്ടി (സ്ഥിരം auto& itr : file.m_വരി_Attrs) m_വരി_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.തിരുകുക(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.തിരുകുക(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    ശൂന്യം updateFinalize() { m_interval_വൃക്ഷം.build(m_ignIndices); }
    ശൂന്യം add_വരി_Attribute(പൂർണ്ണസംഖ്യ lineno, VPragmaType attr) { m_വരി_Attrs[lineno].set(attr); }
    ശൂന്യം addIgnore(V3ErrorCode code, പൂർണ്ണസംഖ്യ min, പൂർണ്ണസംഖ്യ max, തർക്കം on) {
        control_Ign_വരികൾ.emplace_back(V3_Control_Ignores_വരി{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_വരികൾ.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    ശൂന്യം addIgnoreMatch(V3ErrorCode code, സ്ഥിരം string& contents, സ്ഥിരം string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        എങ്കിൽ (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    ശൂന്യം applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        സ്ഥിരം VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        എങ്കിൽ (വരി_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(പുതിയ AstPragma{nodep->fileline(), pragma});
        }
    }
    ശൂന്യം applyBlock(Ast_കണ്ണി_Block* nodep) {
        // Apply to block at this line
        സ്ഥിരം VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (വരി_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(പുതിയ AstStmtPragma{flp, പുതിയ AstPragma{flp, pragma}});
        }
    }
    ശൂന്യം applyCase(AstCase* nodep) {
        // Apply to this case at this line
        സ്ഥിരം പൂർണ്ണസംഖ്യ lineno = nodep->fileline()->lineno();
        എങ്കിൽ (വരി_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(സത്യം);
        എങ്കിൽ (വരി_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(സത്യം);
    }
    ശൂന്യം applyIgnores(File_വരി* filelinep) {
        // HOT routine, called each parsed token line of this filename
        എങ്കിൽ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            തിരികെ;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        എങ്കിൽ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            തിരികെ;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_വൃക്ഷം.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        വേണ്ടി (സ്ഥിരം auto& it : results) {
            സ്ഥിരം V3_Control_Ignores_വരി& cign = control_Ign_വരികൾ[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.nextChange = std::max(m_lastIgnore.nextChange, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_lastIgnore.nextChange
                                       << " Messages: " << bitset.ascii());
    }
    തർക്കം waive(V3ErrorCode code, സ്ഥിരം string& message) {
        എങ്കിൽ (code.hardError()) തിരികെ അസത്യം;
        വേണ്ടി (സ്ഥിരം auto& itr : m_waivers) {
            എങ്കിൽ ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

വർഗ്ഗം V3ControlScopeTraceEntry final {
പൊതു:
    സ്ഥിരം string m_scope;  // Scope or regexp to match
    സ്ഥിരം തർക്കം m_on;  // True to enable message
    പൂർണ്ണസംഖ്യ m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(സ്ഥിരം string& scope, തർക്കം on, പൂർണ്ണസംഖ്യ levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = സ്ഥിരസ്ഥിതി;
    തർക്കം operator<(സ്ഥിരം V3ControlScopeTraceEntry& other) സ്ഥിരം {
        എങ്കിൽ (m_on < other.m_on) തിരികെ സത്യം;
        എങ്കിൽ (m_on > other.m_on) തിരികെ അസത്യം;
        എങ്കിൽ (m_levels < other.m_levels) തിരികെ സത്യം;
        എങ്കിൽ (m_levels > other.m_levels) തിരികെ അസത്യം;
        തിരികെ m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
വർഗ്ഗം V3ControlScopeTraceEntryMatch final {
പൊതു:
    സ്ഥിരം V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    സ്ഥിരം string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(സ്ഥിരം V3ControlScopeTraceEntry* entryp, സ്ഥിരം string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    തർക്കം operator<(സ്ഥിരം V3ControlScopeTraceEntryMatch& other) സ്ഥിരം {
        എങ്കിൽ (m_entryp < other.m_entryp) തിരികെ സത്യം;
        എങ്കിൽ (m_entryp > other.m_entryp) തിരികെ അസത്യം;
        തിരികെ m_scopepart < other.m_scopepart;
    }
};

വർഗ്ഗം V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, തർക്കം> m_matchCache;  // Matching entries for speed

പൊതു:
    ശൂന്യം addScopeTraceOn(തർക്കം on, സ്ഥിരം string& scope, പൂർണ്ണസംഖ്യ levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    തർക്കം getEntryMatch(സ്ഥിരം V3ControlScopeTraceEntry* entp, സ്ഥിരം string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        സ്ഥിരം V3ControlScopeTraceEntryMatch താക്കോൽ{entp, scopepart};
        സ്ഥിരം auto pair = m_matchCache.emplace(താക്കോൽ, അസത്യം);
        എങ്കിൽ (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        തിരികെ pair.first->second;
    }

    തർക്കം getScopeTraceOn(സ്ഥിരം string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        പൂർണ്ണസംഖ്യ maxLevel = 1;
        വേണ്ടി (സ്ഥിരം auto& ch : scope) {
            എങ്കിൽ (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        തർക്കം enabled = സത്യം;
        വേണ്ടി (സ്ഥിരം auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            പൂർണ്ണസംഖ്യ partLevel = 1;
            വേണ്ടി (string::size_type partEnd = 0; സത്യം;) {
                partEnd = scope.find('.', partEnd + 1);
                എങ്കിൽ (partEnd == string::npos) partEnd = scope.length();
                സ്ഥിരം std::string scopepart = scope.substr(0, partEnd);
                എങ്കിൽ (getEntryMatch(&ent, scopepart)) {
                    സ്ഥിരം തർക്കം levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    എങ്കിൽ (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    നിർത്തുക;
                }
                എങ്കിൽ (partEnd == scope.length()) നിർത്തുക;
                ++partLevel;
            }
        }
        തിരികെ enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

വർഗ്ഗം V3ControlResolverHierWorkerEntry final {
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_workers;  // Number of workers requested for the hierarchical block
    File_വരി* സ്ഥിരം m_flp;  // Location the worker count was specified at

പൊതു:
    explicit V3ControlResolverHierWorkerEntry(പൂർണ്ണസംഖ്യ workers, File_വരി* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    പൂർണ്ണസംഖ്യ workers() സ്ഥിരം { തിരികെ m_workers; }
    File_വരി* flp() സ്ഥിരം { തിരികെ m_flp; }
};

വർഗ്ഗം V3ControlResolver final {
    എണ്ണൽ ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_വരി* m_profile_File_വരി = nullptr;  // Location profile_data was read from

    V3ControlResolver() = സ്ഥിരസ്ഥിതി;
    ~V3ControlResolver() = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം V3ControlResolver& s() {
        സ്ഥിതം V3ControlResolver s_singleton;
        തിരികെ s_singleton;
    }
    V3ControlModuleResolver& modules() { തിരികെ m_modules; }
    V3ControlFileResolver& files() { തിരികെ m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { തിരികെ m_scopeTraces; }

    ശൂന്യം addProfileData(File_വരി* fl, സ്ഥിരം string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    ശൂന്യം addProfileData(File_വരി* fl, സ്ഥിരം string& model, സ്ഥിരം string& താക്കോൽ, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        എങ്കിൽ (!m_profile_File_വരി) m_profile_File_വരി = fl;
        എങ്കിൽ (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        എങ്കിൽ (mode == MTASK) {
            m_profileData[model][താക്കോൽ] += cost;
        } അല്ലെങ്കിൽ എങ്കിൽ (mode == HIER_DPI) {
            m_profileData[model][താക്കോൽ] = std::max(m_profileData[model][താക്കോൽ], cost);
        }
        m_mode |= mode;
    }
    തർക്കം containsMTaskProfileData() സ്ഥിരം { തിരികെ m_mode & MTASK; }
    uint64_t getProfileData(സ്ഥിരം string& hierDpi) സ്ഥിരം {
        // Empty key for hierarchical DPI wrapper costs.
        തിരികെ getProfileData(hierDpi, "");
    }
    ശൂന്യം addHierWorkers(File_വരി* flp, സ്ഥിരം string& model, പൂർണ്ണസംഖ്യ workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    പൂർണ്ണസംഖ്യ getHierWorkers(സ്ഥിരം string& model) സ്ഥിരം {
        സ്ഥിരം auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        തിരികെ mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_വരി* get_Hier_Workers_File_വരി(സ്ഥിരം string& model) സ്ഥിരം {
        സ്ഥിരം auto mit = m_hierWorkers.find(model);
        തിരികെ mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(സ്ഥിരം string& model, സ്ഥിരം string& താക്കോൽ) സ്ഥിരം {
        സ്ഥിരം auto mit = m_profileData.find(model);
        എങ്കിൽ (mit == m_profileData.cend()) തിരികെ 0;
        സ്ഥിരം auto it = mit->second.find(താക്കോൽ);
        എങ്കിൽ (it == mit->second.cend()) തിരികെ 0;
        തിരികെ it->second;
    }
    File_വരി* get_Profile_Data_File_വരി() സ്ഥിരം { തിരികെ m_profile_File_വരി; }  // Maybe null
    സ്ഥിതം uint64_t getCurrentHierBlockCost() {
        എങ്കിൽ (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            തിരികെ cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
        uint64_t cost = 0;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < VEval::_ENUM_END; ++i) {
            സ്ഥിരം VEval eval{i};
            എങ്കിൽ (eval.slow()) തുടരുക;
            cost += V3_Instr_എണ്ണം::എണ്ണം(netlistp->evalFuncp(eval), അസത്യം);
        }
        UINFO(9, "Evaluating cost: " << cost);
        തിരികെ cost;
    }
};

//######################################################################
// V3Control

ശൂന്യം V3Control::addCaseFull(സ്ഥിരം string& filename, പൂർണ്ണസംഖ്യ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_വരി_Attribute(lineno, VPragmaType::FULL_CASE);
}
ശൂന്യം V3Control::addCaseParallel(സ്ഥിരം string& filename, പൂർണ്ണസംഖ്യ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_വരി_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
ശൂന്യം V3Control::addCoverageBlockOff(സ്ഥിരം string& filename, പൂർണ്ണസംഖ്യ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_വരി_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
ശൂന്യം V3Control::addCoverageBlockOff(സ്ഥിരം string& module, സ്ഥിരം string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

ശൂന്യം V3Control::addHierWorkers(File_വരി* fl, സ്ഥിരം string& model, പൂർണ്ണസംഖ്യ workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

ശൂന്യം V3Control::addFsmRegisterWrapper(File_വരി* fl, സ്ഥിരം string& module, സ്ഥിരം string& d,
                                      സ്ഥിരം string& q, സ്ഥിരം string& clock, സ്ഥിരം string& reset,
                                      സ്ഥിരം string& reset_മൂല്യം) {
    string missing;
    എങ്കിൽ (module.empty()) missing += "-module";
    എങ്കിൽ (d.empty()) missing = VString::dot(missing, ", ", "-d");
    എങ്കിൽ (q.empty()) missing = VString::dot(missing, ", ", "-q");
    എങ്കിൽ (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    എങ്കിൽ (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        തിരികെ;
    }

    സ്ഥിരം FsmRegisterWrapper desc{module, d, q, clock, reset, reset_മൂല്യം};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

ശൂന്യം V3Control::addIgnore(V3ErrorCode code, തർക്കം on, സ്ഥിരം string& filename, പൂർണ്ണസംഖ്യ min, പൂർണ്ണസംഖ്യ max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    എങ്കിൽ (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_വരി::globalWarnOff(code, !on);
    } അല്ലെങ്കിൽ {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
ശൂന്യം V3Control::addIgnoreMatch(V3ErrorCode code, സ്ഥിരം string& filename, സ്ഥിരം string& contents,
                               സ്ഥിരം string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

ശൂന്യം V3Control::addInline(File_വരി* fl, സ്ഥിരം string& module, സ്ഥിരം string& ftask, തർക്കം on) {
    എങ്കിൽ (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } അല്ലെങ്കിൽ {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

ശൂന്യം V3Control::addModulePragma(സ്ഥിരം string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

ശൂന്യം V3Control::addProfileData(File_വരി* fl, സ്ഥിരം string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
ശൂന്യം V3Control::addProfileData(File_വരി* fl, സ്ഥിരം string& model, സ്ഥിരം string& താക്കോൽ,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, താക്കോൽ, cost);
}

ശൂന്യം V3Control::addScopeTraceOn(തർക്കം on, സ്ഥിരം string& scope, പൂർണ്ണസംഖ്യ levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

ശൂന്യം V3Control::addVarAttr(File_വരി* fl, സ്ഥിരം string& module, സ്ഥിരം string& ftask,
                           VarSpecKind kind, സ്ഥിരം string& pattern, VAttrType attr,
                           Ast_Sen_വൃക്ഷം* sensep) {
    എങ്കിൽ (sensep) {
        File_വരി* സ്ഥിരം flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_വൃക്ഷം(), sensep);
        // Used to be only accepted on public_flat_rw
        എങ്കിൽ (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            തിരികെ;
        }
    }

    എങ്കിൽ (kind != VarSpecKind::VAR) {
        തിരഞ്ഞെടുപ്പ് (attr) {
        അവസ്ഥ VAttrType::VAR_PUBLIC_FLAT:
        അവസ്ഥ VAttrType::VAR_PUBLIC_FLAT_RD:
        അവസ്ഥ VAttrType::VAR_PUBLIC_FLAT_RW: നിർത്തുക;
        സ്ഥിരസ്ഥിതി:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            തിരികെ;
        }
    }

    // Semantics: Most of the attributes operate on signals
    എങ്കിൽ (pattern.empty()) {
        എങ്കിൽ (attr == VAttrType::VAR_PUBLIC) {
            എങ്കിൽ (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } അല്ലെങ്കിൽ {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(സത്യം);
            }
        } അല്ലെങ്കിൽ {
            fl->v3error("missing -var");
        }
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (attr == VAttrType::VAR_FORCEABLE) {
            എങ്കിൽ (module.empty()) {
                fl->v3error("forceable missing -module");
            } അല്ലെങ്കിൽ എങ്കിൽ (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } അല്ലെങ്കിൽ {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } അല്ലെങ്കിൽ {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                എങ്കിൽ (ftask.empty()) {
                    എങ്കിൽ (kind == VarSpecKind::PARAM) തിരികെ mod.params().at(pattern);
                    എങ്കിൽ (kind == VarSpecKind::PORT) തിരികെ mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    തിരികെ mod.vars().at(pattern);
                }
                എങ്കിൽ (kind == VarSpecKind::PARAM) തിരികെ mod.ftasks().at(ftask).params().at(pattern);
                എങ്കിൽ (kind == VarSpecKind::PORT) തിരികെ mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                തിരികെ mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

ശൂന്യം V3Control::applyCase(AstCase* nodep) {
    സ്ഥിരം string& filename = nodep->fileline()->filename();
    V3ControlFile* സ്ഥിരം filep = V3ControlResolver::s().files().resolve(filename);
    എങ്കിൽ (filep) filep->applyCase(nodep);
}

ശൂന്യം V3Control::applyCoverageBlock(Ast_കണ്ണി_Module* modulep, AstBegin* nodep) {
    സ്ഥിരം string& filename = nodep->fileline()->filename();
    V3ControlFile* സ്ഥിരം filep = V3ControlResolver::s().files().resolve(filename);
    എങ്കിൽ (filep) filep->applyBlock(nodep);
    സ്ഥിരം string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* സ്ഥിരം modp = V3ControlResolver::s().modules().resolve(modname);
    എങ്കിൽ (modp) modp->applyBlock(nodep);
}
ശൂന്യം V3Control::applyCoverageBlock(Ast_കണ്ണി_Module* modulep, AstGenBlock* nodep) {
    സ്ഥിരം string& filename = nodep->fileline()->filename();
    V3ControlFile* സ്ഥിരം filep = V3ControlResolver::s().files().resolve(filename);
    എങ്കിൽ (filep) filep->applyBlock(nodep);
    സ്ഥിരം string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* സ്ഥിരം modp = V3ControlResolver::s().modules().resolve(modname);
    എങ്കിൽ (modp) modp->applyBlock(nodep);
}

ശൂന്യം V3Control::applyIgnores(File_വരി* filelinep) {
    സ്ഥിരം string& filename = filelinep->filename();
    V3ControlFile* സ്ഥിരം filep = V3ControlResolver::s().files().resolve(filename);
    എങ്കിൽ (filep) filep->applyIgnores(filelinep);
}

ശൂന്യം V3Control::applyModule(Ast_കണ്ണി_Module* modulep) {
    സ്ഥിരം string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* സ്ഥിരം modp = V3ControlResolver::s().modules().resolve(modname);
    എങ്കിൽ (modp) modp->apply(modulep);
}

ശൂന്യം V3Control::applyFTask(Ast_കണ്ണി_Module* modulep, Ast_കണ്ണി_FTask* ftaskp) {
    സ്ഥിരം string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* സ്ഥിരം modp = V3ControlResolver::s().modules().resolve(modname);
    എങ്കിൽ (!modp) തിരികെ;
    സ്ഥിരം V3ControlFTask* സ്ഥിരം ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    എങ്കിൽ (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
സ്ഥിതം ശൂന്യം resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    സ്ഥിരം std::string name = varp->prettyDehashOrigOrName();
    എങ്കിൽ (സ്ഥിരം V3ControlVar* സ്ഥിരം vp = resolver.vars().resolve(name)) vp->apply(varp);
    എങ്കിൽ (varp->isParam()) {
        എങ്കിൽ (സ്ഥിരം V3ControlVar* സ്ഥിരം vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    എങ്കിൽ (varp->isIO()) {
        എങ്കിൽ (സ്ഥിരം V3ControlVar* സ്ഥിരം vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

ശൂന്യം V3Control::applyVarAttr(സ്ഥിരം Ast_കണ്ണി_Module* modulep, സ്ഥിരം Ast_കണ്ണി_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* സ്ഥിരം modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    എങ്കിൽ (!modp) തിരികെ;
    എങ്കിൽ (ftaskp) {
        V3ControlFTask* സ്ഥിരം ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        എങ്കിൽ (!ftp) തിരികെ;
        resolveThenApply(*ftp, varp);
    } അല്ലെങ്കിൽ {
        resolveThenApply(*modp, varp);
    }
}

പൂർണ്ണസംഖ്യ V3Control::getHierWorkers(സ്ഥിരം string& model) {
    തിരികെ V3ControlResolver::s().getHierWorkers(model);
}
File_വരി* V3Control::get_Hier_Workers_File_വരി(സ്ഥിരം string& model) {
    തിരികെ V3ControlResolver::s().get_Hier_Workers_File_വരി(model);
}
സ്ഥിരം V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(സ്ഥിരം string& module) {
    V3ControlModule* സ്ഥിരം modp = V3ControlResolver::s().modules().resolve(module);
    തിരികെ modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(സ്ഥിരം string& hierDpi) {
    തിരികെ V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(സ്ഥിരം string& model, സ്ഥിരം string& താക്കോൽ) {
    തിരികെ V3ControlResolver::s().getProfileData(model, താക്കോൽ);
}
File_വരി* V3Control::get_Profile_Data_File_വരി() {
    തിരികെ V3ControlResolver::s().get_Profile_Data_File_വരി();
}
തർക്കം V3Control::getScopeTraceOn(സ്ഥിരം string& scope) {
    തിരികെ V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

ശൂന്യം V3Control::contents_തള്ളുക_Text(സ്ഥിരം string& text) { WildcardContents::തള്ളുക_Text(text); }

തർക്കം V3Control::containsMTaskProfileData() {
    തിരികെ V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    തിരികെ V3ControlResolver::s().getCurrentHierBlockCost();
}

തർക്കം V3Control::waive(സ്ഥിരം File_വരി* filelinep, V3ErrorCode code, സ്ഥിരം string& message) {
    V3ControlFile* സ്ഥിരം filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    എങ്കിൽ (!filep) തിരികെ അസത്യം;
    തിരികെ filep->waive(code, message);
}

ശൂന്യം V3Control::selfTest() { VInterval_വൃക്ഷം::selfTest(); }
