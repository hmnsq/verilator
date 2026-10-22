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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Control.h"

#અન્તર્ભાવ "V3InstrCount.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
વર્ગ V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<અચલ std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<અચલ std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

જાહેર:
    V3ControlWildcardResolver() = મૂળભૂત;
    ~V3ControlWildcardResolver() = મૂળભૂત;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    શૂન્ય update(અચલ V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        અચલ V3LockGuard lock{m_mutex};
        અચલ V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        માટે (અચલ auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(અચલ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        અચલ V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        પાછા m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(અચલ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        અચલ V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        અચલ auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        જો (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            અચલ string slashName = V3Os::filenameSlashPath(name);
            માટે (અચલ auto& patEnt : m_mapPatterns) {
                જો (VString::wildmatch(slashName, patEnt.first)) {
                    જો (!entryr) entryr.reset(નવું T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            જો (entryr) entryr->updateFinalize();
        }
        પાછા entryr.get();
    }
};

// List of attributes for variables
વર્ગ V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
જાહેર:
    // Add new attribugte
    શૂન્ય add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    શૂન્ય update(અચલ V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.ઉમેરો(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    શૂન્ય updateFinalize() {}
    // Apply all attributes to the variable
    શૂન્ય apply(AstVar* varp) અચલ {
        માટે (અચલ VAttrType attr : m_attrs) {
            varp->addAttrsp(નવું AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

વર્ગ WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<અચલ std::string, તાર્કિક> m_mapPatterns;  // Pattern match results
    std::deque<string> m_લીટીઓ;  // Source text lines

    // METHODS
    સ્થિર WildcardContents& s() {  // Singleton
        સ્થિર WildcardContents s_s;
        પાછા s_s;
    }
    શૂન્ય clearCacheImp() { m_mapPatterns.clear(); }
    શૂન્ય ધકેલો_Text_Imp(અચલ string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        અચલ string leftover = m_લીટીઓ.back() + text;
        m_લીટીઓ.કાઢો_back();

        // Insert line-by-line
        string::size_type લીટી_start = 0;
        જ્યારે (સાચું) {
            અચલ string::size_type લીટી_end = leftover.find('\n', લીટી_start);
            જો (લીટી_end != string::npos) {
                અચલ string oneline(leftover, લીટી_start, લીટી_end - લીટી_start + 1);
                જો (oneline.size() > 1) m_લીટીઓ.ધકેલો_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_લીટીઓ.size() - 1) << "]: " << oneline);
                લીટી_start = લીટી_end + 1;
            } નહીંતર {
                તોડો;
            }
        }
        // Keep leftover for next time
        m_લીટીઓ.emplace_back(string(leftover, લીટી_start));  // Might be ""
        clearCacheImp();
    }

    તાર્કિક resolveUncachedImp(અચલ string& contentsRegexp) {
        માટે (અચલ string& i : m_લીટીઓ) {
            જો (VString::wildmatch(i, contentsRegexp)) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    તાર્કિક resolveCachedImp(અચલ string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        જો (contentsRegexp.empty()) પાછા સાચું;
        અચલ auto pair = m_mapPatterns.emplace(contentsRegexp, ખોટું);
        તાર્કિક& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        જો (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        પાછા entryr;
    }

જાહેર:
    WildcardContents() {
        m_લીટીઓ.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = મૂળભૂત;
    // Return true iff contentsRegexp in parsed contents
    સ્થિર તાર્કિક resolve(અચલ string& contentsRegexp) {
        પાછા s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    સ્થિર શૂન્ય ધકેલો_Text(અચલ string& text) { s().ધકેલો_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

વર્ગ V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    તાર્કિક m_noinline = ખોટું;  // Don't inline function/task
    તાર્કિક m_public = ખોટું;  // Public function/task

જાહેર:
    V3ControlFTask() = મૂળભૂત;
    શૂન્ય update(અચલ V3ControlFTask& f) {
        // Don't overwrite true with false
        જો (f.m_noinline) m_noinline = સાચું;
        જો (f.m_public) m_public = સાચું;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    શૂન્ય updateFinalize() {}

    V3ControlVarResolver& params() { પાછા m_params; }
    V3ControlVarResolver& ports() { પાછા m_ports; }
    V3ControlVarResolver& vars() { પાછા m_vars; }

    શૂન્ય setNoInline(તાર્કિક set) { m_noinline = set; }
    શૂન્ય setPublic(તાર્કિક set) { m_public = set; }

    શૂન્ય apply(Ast_ગાંઠ_FTask* ftaskp) અચલ {
        જો (m_noinline)
            ftaskp->addStmtsp(નવું AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        જો (m_public)
            ftaskp->addStmtsp(નવું AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

વર્ગ V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    તાર્કિક m_hasFsmRegisterWrapper = ખોટું;  // Whether descriptor has been set
    તાર્કિક m_inline = ખોટું;  // Whether to force the inline
    તાર્કિક m_inline_કિંમત = ખોટું;  // The inline value (on/off)

જાહેર:
    V3ControlModule() = મૂળભૂત;

    શૂન્ય update(અચલ V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        જો (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = સાચું;
        }
        માટે (અચલ string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.ઉમેરો(i);
        જો (!m_inline) {
            m_inline = m.m_inline;
            m_inline_કિંમત = m.m_inline_કિંમત;
        }
        માટે (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ઉમેરો(*it);
        }
    }
    શૂન્ય updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { પાછા m_tasks; }
    V3ControlVarResolver& params() { પાછા m_params; }
    V3ControlVarResolver& ports() { પાછા m_ports; }
    V3ControlVarResolver& vars() { પાછા m_vars; }

    શૂન્ય addCoverageBlockOff(અચલ string& name) { m_coverageOffBlocks.ઉમેરો(name); }
    શૂન્ય setInline(તાર્કિક set) {
        m_inline = સાચું;
        m_inline_કિંમત = set;
    }
    શૂન્ય setFsmRegisterWrapper(File_લીટી* fl, અચલ V3Control::FsmRegisterWrapper& desc) {
        જો (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ગાંઠ::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = સાચું;
    }
    અચલ V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() અચલ {
        પાછા m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    શૂન્ય addModulePragma(VPragmaType pragma) { m_modPragmas.ઉમેરો(pragma); }

    શૂન્ય apply(Ast_ગાંઠ_Module* modp) {
        જો (m_inline) {
            અચલ VPragmaType type
                = m_inline_કિંમત ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ગાંઠ* અચલ nodep = નવું AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        માટે (અચલ auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            જો (itr == VPragmaType::HIER_PARAMS) modp->hierParams(સાચું);
            Ast_ગાંઠ* અચલ nodep = નવું AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    શૂન્ય applyBlock(AstGenBlock* nodep) {
        અચલ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        જો (!nodep->unnamed()) {
            માટે (અચલ string& i : m_coverageOffBlocks) {
                જો (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(નવું AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    શૂન્ય applyBlock(Ast_ગાંઠ_Block* nodep) {
        અચલ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_લીટી* અચલ flp = nodep->fileline();
        જો (!nodep->unnamed()) {
            માટે (અચલ string& i : m_coverageOffBlocks) {
                જો (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(નવું AstStmtPragma{flp, નવું AstPragma{flp, pragma}});
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
વર્ગ V3_Control_Ignores_લીટી final {
જાહેર:
    અચલ પૂર્ણાંક m_લીટી_Min;  // Minimum line number to make change at
    અચલ પૂર્ણાંક m_લીટી_Max;  // Maximum line number to make change at (inclusive)
    અચલ V3ErrorCode m_code;  // Error code
    અચલ તાર્કિક m_on;  // True to enable message
    V3_Control_Ignores_લીટી(V3ErrorCode code, પૂર્ણાંક linemin, પૂર્ણાંક linemax, તાર્કિક on)
        : m_લીટી_Min{linemin}
        , m_લીટી_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_લીટી() = મૂળભૂત;
    તાર્કિક every_લીટી() અચલ { પાછા m_લીટી_Min == 0 && m_લીટી_Max == 0; }
    તાર્કિક inRange(પૂર્ણાંક lineno) અચલ {
        પાછા (lineno >= m_લીટી_Min && lineno <= m_લીટી_Max) || every_લીટી();
    }
};
std::ostream& operator<<(std::ostream& os, અચલ V3_Control_Ignores_લીટી& rhs) {
    પાછા os << rhs.m_લીટી_Min << "-" << rhs.m_લીટી_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
સ્થિર std::vector<V3_Control_Ignores_લીટી> control_Ign_લીટીઓ;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

વર્ગ VInterval_વૃક્ષ final {
    સંરચના Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        પૂર્ણાંક m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(પૂર્ણાંક center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_લીટીઓ;  // All-line disables

ખાનગી:
    std::unique_ptr<Entry> build_વૃક્ષ(અચલ IgnIndices& points) {
        જો (points.empty()) પાછા nullptr;

        પૂર્ણાંક minval = std::numeric_limits<પૂર્ણાંક>::max();
        પૂર્ણાંક maxval = std::numeric_limits<પૂર્ણાંક>::min();
        માટે (અચલ auto& it : points) {
            અચલ V3_Control_Ignores_લીટી& cign = control_Ign_લીટીઓ[it];
            જો (cign.every_લીટી()) {
                m_every_લીટીઓ.emplace_back(it);
                ચાલુ;
            }
            minval = std::min(minval, cign.m_લીટી_Min);
            maxval = std::max(maxval, cign.m_લીટી_Max);
        }

        અચલ પૂર્ણાંક center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(નવું Entry{center});

        IgnIndices ડાબું_Points;
        IgnIndices જમણું_Points;
        માટે (અચલ auto& it : points) {
            અચલ V3_Control_Ignores_લીટી& cign = control_Ign_લીટીઓ[it];
            જો (cign.every_લીટી()) ચાલુ;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            જો (cign.m_લીટી_Max < center)
                ડાબું_Points.emplace_back(it);
            નહીંતર જો (cign.m_લીટી_Min > center)
                જમણું_Points.emplace_back(it);
            નહીંતર {
                entp->m_byMin.ધકેલો_back(it);
                entp->m_byMax.ધકેલો_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            પાછા control_Ign_લીટીઓ[a].m_લીટી_Min < control_Ign_લીટીઓ[b].m_લીટી_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            પાછા control_Ign_લીટીઓ[a].m_લીટી_Max > control_Ign_લીટીઓ[b].m_લીટી_Max;
        });

        entp->m_leftp = build_વૃક્ષ(ડાબું_Points);
        entp->m_rightp = build_વૃક્ષ(જમણું_Points);
        પાછા entp;
    }

    શૂન્ય find_વૃક્ષ(પૂર્ણાંક lineno, Entry* entp, IgnIndices& resultsr, પૂર્ણાંક& nextChanger) અચલ {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        જો (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            માટે (અચલ auto& it : entp->m_byMin) {
                અચલ V3_Control_Ignores_લીટી& cign = control_Ign_લીટીઓ[it];
                જો (cign.m_લીટી_Min <= lineno) {
                    resultsr.emplace_back(it);
                } નહીંતર {
                    nextChanger = std::min(nextChanger, cign.m_લીટી_Min);
                    તોડો;
                }
            }
            જો (entp->m_leftp) find_વૃક્ષ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } નહીંતર જો (lineno > entp->m_center) {
            માટે (અચલ auto& it : entp->m_byMax) {
                અચલ V3_Control_Ignores_લીટી& cign = control_Ign_લીટીઓ[it];
                જો (cign.m_લીટી_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_લીટી_Max + 1);
                    resultsr.emplace_back(it);
                } નહીંતર {
                    તોડો;
                }
            }
            જો (entp->m_rightp) find_વૃક્ષ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } નહીંતર {
            માટે (અચલ auto& it : entp->m_byMin) {
                અચલ V3_Control_Ignores_લીટી& cign = control_Ign_લીટીઓ[it];
                resultsr.emplace_back(it);
                જો (cign.m_લીટી_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_લીટી_Max + 1);
            }
            જો (entp->m_leftp) find_વૃક્ષ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            જો (entp->m_rightp) find_વૃક્ષ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

જાહેર:
    // CONSTRUCTORS
    VInterval_વૃક્ષ() = મૂળભૂત;
    ~VInterval_વૃક્ષ() = મૂળભૂત;
    // METHODS
    શૂન્ય clear() { m_rootp = nullptr; }
    શૂન્ય build(અચલ IgnIndices& points) {
        clear();
        m_rootp = build_વૃક્ષ(points);
    }
    શૂન્ય find(પૂર્ણાંક lineno, IgnIndices& resultsr, પૂર્ણાંક& nextChanger) અચલ {
        resultsr = m_every_લીટીઓ;
        nextChanger = std::numeric_limits<પૂર્ણાંક>::max();

        જો (m_rootp) find_વૃક્ષ(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    સ્થિર શૂન્ય selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        માટે (અચલ auto& it : points) {
            control_Ign_લીટીઓ.emplace_back(
                V3_Control_Ignores_લીટી{V3ErrorCode::I_LINT, it.first, it.second, સાચું});
            data.emplace_back(static_cast<uint32_t>(control_Ign_લીટીઓ.size() - 1));
        }
        VInterval_વૃક્ષ વૃક્ષ;
        વૃક્ષ.build(data);

        IgnIndices results;
        પૂર્ણાંક nextChange = 0;
        વૃક્ષ.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        વૃક્ષ.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        વૃક્ષ.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        વૃક્ષ.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        વૃક્ષ.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        વૃક્ષ.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        વૃક્ષ.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        વૃક્ષ.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<પૂર્ણાંક>::max());
        //
        points = {{0, 0}};
        માટે (અચલ auto& it : points) {
            control_Ign_લીટીઓ.emplace_back(
                V3_Control_Ignores_લીટી{V3ErrorCode::I_LINT, it.first, it.second, સાચું});
            data.emplace_back(static_cast<uint32_t>(control_Ign_લીટીઓ.size() - 1));
        }
        વૃક્ષ.build(data);
        //
        વૃક્ષ.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_લીટી_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

વર્ગ WaiverSetting final {
જાહેર:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, અચલ string& contents, અચલ string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = મૂળભૂત;
    WaiverSetting& operator=(અચલ WaiverSetting& rhs) = મૂળભૂત;
};

// File entity
વર્ગ V3ControlFile final {
    using લીટી_Attr_Map = std::map<પૂર્ણાંક, V3_Control_લીટી_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    લીટી_Attr_Map m_લીટી_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_વૃક્ષ m_interval_વૃક્ષ;  // Tree of indices to IgnLines

    સંરચના {
        પૂર્ણાંક filenameno = -1;  // Last filename
        પૂર્ણાંક lineno = -1;  // Last linenumber
        પૂર્ણાંક nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    તાર્કિક લીટી_Match(પૂર્ણાંક lineno, VPragmaType type) {
        જો (m_લીટી_Attrs.find(0) != m_લીટી_Attrs.end() && m_લીટી_Attrs[0][type]) પાછા સાચું;
        જો (m_લીટી_Attrs.find(lineno) == m_લીટી_Attrs.end()) પાછા ખોટું;
        પાછા m_લીટી_Attrs[lineno][type];
    }

જાહેર:
    V3ControlFile() = મૂળભૂત;
    શૂન્ય update(અચલ V3ControlFile& file) {
        // Copy in all attributes and waivers
        માટે (અચલ auto& itr : file.m_લીટી_Attrs) m_લીટી_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.ઉમેરો(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.ઉમેરો(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    શૂન્ય updateFinalize() { m_interval_વૃક્ષ.build(m_ignIndices); }
    શૂન્ય add_લીટી_Attribute(પૂર્ણાંક lineno, VPragmaType attr) { m_લીટી_Attrs[lineno].set(attr); }
    શૂન્ય addIgnore(V3ErrorCode code, પૂર્ણાંક min, પૂર્ણાંક max, તાર્કિક on) {
        control_Ign_લીટીઓ.emplace_back(V3_Control_Ignores_લીટી{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_લીટીઓ.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    શૂન્ય addIgnoreMatch(V3ErrorCode code, અચલ string& contents, અચલ string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        જો (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    શૂન્ય applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        અચલ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        જો (લીટી_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(નવું AstPragma{nodep->fileline(), pragma});
        }
    }
    શૂન્ય applyBlock(Ast_ગાંઠ_Block* nodep) {
        // Apply to block at this line
        અચલ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_લીટી* અચલ flp = nodep->fileline();
        જો (લીટી_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(નવું AstStmtPragma{flp, નવું AstPragma{flp, pragma}});
        }
    }
    શૂન્ય applyCase(AstCase* nodep) {
        // Apply to this case at this line
        અચલ પૂર્ણાંક lineno = nodep->fileline()->lineno();
        જો (લીટી_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(સાચું);
        જો (લીટી_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(સાચું);
    }
    શૂન્ય applyIgnores(File_લીટી* filelinep) {
        // HOT routine, called each parsed token line of this filename
        જો (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            પાછા;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        જો (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            પાછા;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<પૂર્ણાંક>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_વૃક્ષ.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        માટે (અચલ auto& it : results) {
            અચલ V3_Control_Ignores_લીટી& cign = control_Ign_લીટીઓ[it];
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
    તાર્કિક waive(V3ErrorCode code, અચલ string& message) {
        જો (code.hardError()) પાછા ખોટું;
        માટે (અચલ auto& itr : m_waivers) {
            જો ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

વર્ગ V3ControlScopeTraceEntry final {
જાહેર:
    અચલ string m_scope;  // Scope or regexp to match
    અચલ તાર્કિક m_on;  // True to enable message
    પૂર્ણાંક m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(અચલ string& scope, તાર્કિક on, પૂર્ણાંક levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = મૂળભૂત;
    તાર્કિક operator<(અચલ V3ControlScopeTraceEntry& other) અચલ {
        જો (m_on < other.m_on) પાછા સાચું;
        જો (m_on > other.m_on) પાછા ખોટું;
        જો (m_levels < other.m_levels) પાછા સાચું;
        જો (m_levels > other.m_levels) પાછા ખોટું;
        પાછા m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
વર્ગ V3ControlScopeTraceEntryMatch final {
જાહેર:
    અચલ V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    અચલ string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(અચલ V3ControlScopeTraceEntry* entryp, અચલ string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    તાર્કિક operator<(અચલ V3ControlScopeTraceEntryMatch& other) અચલ {
        જો (m_entryp < other.m_entryp) પાછા સાચું;
        જો (m_entryp > other.m_entryp) પાછા ખોટું;
        પાછા m_scopepart < other.m_scopepart;
    }
};

વર્ગ V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, તાર્કિક> m_matchCache;  // Matching entries for speed

જાહેર:
    શૂન્ય addScopeTraceOn(તાર્કિક on, અચલ string& scope, પૂર્ણાંક levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    તાર્કિક getEntryMatch(અચલ V3ControlScopeTraceEntry* entp, અચલ string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        અચલ V3ControlScopeTraceEntryMatch ચાવી{entp, scopepart};
        અચલ auto pair = m_matchCache.emplace(ચાવી, ખોટું);
        જો (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        પાછા pair.first->second;
    }

    તાર્કિક getScopeTraceOn(અચલ string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        પૂર્ણાંક maxLevel = 1;
        માટે (અચલ auto& ch : scope) {
            જો (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        તાર્કિક enabled = સાચું;
        માટે (અચલ auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            પૂર્ણાંક partLevel = 1;
            માટે (string::size_type partEnd = 0; સાચું;) {
                partEnd = scope.find('.', partEnd + 1);
                જો (partEnd == string::npos) partEnd = scope.length();
                અચલ std::string scopepart = scope.substr(0, partEnd);
                જો (getEntryMatch(&ent, scopepart)) {
                    અચલ તાર્કિક levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    જો (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    તોડો;
                }
                જો (partEnd == scope.length()) તોડો;
                ++partLevel;
            }
        }
        પાછા enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

વર્ગ V3ControlResolverHierWorkerEntry final {
    અચલ પૂર્ણાંક m_workers;  // Number of workers requested for the hierarchical block
    File_લીટી* અચલ m_flp;  // Location the worker count was specified at

જાહેર:
    explicit V3ControlResolverHierWorkerEntry(પૂર્ણાંક workers, File_લીટી* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    પૂર્ણાંક workers() અચલ { પાછા m_workers; }
    File_લીટી* flp() અચલ { પાછા m_flp; }
};

વર્ગ V3ControlResolver final {
    ગણના ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_લીટી* m_profile_File_લીટી = nullptr;  // Location profile_data was read from

    V3ControlResolver() = મૂળભૂત;
    ~V3ControlResolver() = મૂળભૂત;

જાહેર:
    સ્થિર V3ControlResolver& s() {
        સ્થિર V3ControlResolver s_singleton;
        પાછા s_singleton;
    }
    V3ControlModuleResolver& modules() { પાછા m_modules; }
    V3ControlFileResolver& files() { પાછા m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { પાછા m_scopeTraces; }

    શૂન્ય addProfileData(File_લીટી* fl, અચલ string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    શૂન્ય addProfileData(File_લીટી* fl, અચલ string& model, અચલ string& ચાવી, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        જો (!m_profile_File_લીટી) m_profile_File_લીટી = fl;
        જો (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        જો (mode == MTASK) {
            m_profileData[model][ચાવી] += cost;
        } નહીંતર જો (mode == HIER_DPI) {
            m_profileData[model][ચાવી] = std::max(m_profileData[model][ચાવી], cost);
        }
        m_mode |= mode;
    }
    તાર્કિક containsMTaskProfileData() અચલ { પાછા m_mode & MTASK; }
    uint64_t getProfileData(અચલ string& hierDpi) અચલ {
        // Empty key for hierarchical DPI wrapper costs.
        પાછા getProfileData(hierDpi, "");
    }
    શૂન્ય addHierWorkers(File_લીટી* flp, અચલ string& model, પૂર્ણાંક workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    પૂર્ણાંક getHierWorkers(અચલ string& model) અચલ {
        અચલ auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        પાછા mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_લીટી* get_Hier_Workers_File_લીટી(અચલ string& model) અચલ {
        અચલ auto mit = m_hierWorkers.find(model);
        પાછા mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(અચલ string& model, અચલ string& ચાવી) અચલ {
        અચલ auto mit = m_profileData.find(model);
        જો (mit == m_profileData.cend()) પાછા 0;
        અચલ auto it = mit->second.find(ચાવી);
        જો (it == mit->second.cend()) પાછા 0;
        પાછા it->second;
    }
    File_લીટી* get_Profile_Data_File_લીટી() અચલ { પાછા m_profile_File_લીટી; }  // Maybe null
    સ્થિર uint64_t getCurrentHierBlockCost() {
        જો (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            પાછા cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* અચલ netlistp = v3Global.rootp();
        uint64_t cost = 0;
        માટે (પૂર્ણાંક i = 0; i < VEval::_ENUM_END; ++i) {
            અચલ VEval eval{i};
            જો (eval.slow()) ચાલુ;
            cost += V3_Instr_ગણતરી::ગણતરી(netlistp->evalFuncp(eval), ખોટું);
        }
        UINFO(9, "Evaluating cost: " << cost);
        પાછા cost;
    }
};

//######################################################################
// V3Control

શૂન્ય V3Control::addCaseFull(અચલ string& filename, પૂર્ણાંક lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_લીટી_Attribute(lineno, VPragmaType::FULL_CASE);
}
શૂન્ય V3Control::addCaseParallel(અચલ string& filename, પૂર્ણાંક lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_લીટી_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
શૂન્ય V3Control::addCoverageBlockOff(અચલ string& filename, પૂર્ણાંક lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_લીટી_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
શૂન્ય V3Control::addCoverageBlockOff(અચલ string& module, અચલ string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

શૂન્ય V3Control::addHierWorkers(File_લીટી* fl, અચલ string& model, પૂર્ણાંક workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

શૂન્ય V3Control::addFsmRegisterWrapper(File_લીટી* fl, અચલ string& module, અચલ string& d,
                                      અચલ string& q, અચલ string& clock, અચલ string& reset,
                                      અચલ string& reset_કિંમત) {
    string missing;
    જો (module.empty()) missing += "-module";
    જો (d.empty()) missing = VString::dot(missing, ", ", "-d");
    જો (q.empty()) missing = VString::dot(missing, ", ", "-q");
    જો (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    જો (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        પાછા;
    }

    અચલ FsmRegisterWrapper desc{module, d, q, clock, reset, reset_કિંમત};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

શૂન્ય V3Control::addIgnore(V3ErrorCode code, તાર્કિક on, અચલ string& filename, પૂર્ણાંક min, પૂર્ણાંક max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    જો (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_લીટી::globalWarnOff(code, !on);
    } નહીંતર {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
શૂન્ય V3Control::addIgnoreMatch(V3ErrorCode code, અચલ string& filename, અચલ string& contents,
                               અચલ string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

શૂન્ય V3Control::addInline(File_લીટી* fl, અચલ string& module, અચલ string& ftask, તાર્કિક on) {
    જો (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } નહીંતર {
        જો (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } નહીંતર {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

શૂન્ય V3Control::addModulePragma(અચલ string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

શૂન્ય V3Control::addProfileData(File_લીટી* fl, અચલ string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
શૂન્ય V3Control::addProfileData(File_લીટી* fl, અચલ string& model, અચલ string& ચાવી,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, ચાવી, cost);
}

શૂન્ય V3Control::addScopeTraceOn(તાર્કિક on, અચલ string& scope, પૂર્ણાંક levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

શૂન્ય V3Control::addVarAttr(File_લીટી* fl, અચલ string& module, અચલ string& ftask,
                           VarSpecKind kind, અચલ string& pattern, VAttrType attr,
                           Ast_Sen_વૃક્ષ* sensep) {
    જો (sensep) {
        File_લીટી* અચલ flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_વૃક્ષ(), sensep);
        // Used to be only accepted on public_flat_rw
        જો (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            પાછા;
        }
    }

    જો (kind != VarSpecKind::VAR) {
        પસંદગી (attr) {
        સ્થિતિ VAttrType::VAR_PUBLIC_FLAT:
        સ્થિતિ VAttrType::VAR_PUBLIC_FLAT_RD:
        સ્થિતિ VAttrType::VAR_PUBLIC_FLAT_RW: તોડો;
        મૂળભૂત:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            પાછા;
        }
    }

    // Semantics: Most of the attributes operate on signals
    જો (pattern.empty()) {
        જો (attr == VAttrType::VAR_PUBLIC) {
            જો (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } નહીંતર {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(સાચું);
            }
        } નહીંતર {
            fl->v3error("missing -var");
        }
    } નહીંતર {
        જો (attr == VAttrType::VAR_FORCEABLE) {
            જો (module.empty()) {
                fl->v3error("forceable missing -module");
            } નહીંતર જો (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } નહીંતર {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } નહીંતર {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                જો (ftask.empty()) {
                    જો (kind == VarSpecKind::PARAM) પાછા mod.params().at(pattern);
                    જો (kind == VarSpecKind::PORT) પાછા mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    પાછા mod.vars().at(pattern);
                }
                જો (kind == VarSpecKind::PARAM) પાછા mod.ftasks().at(ftask).params().at(pattern);
                જો (kind == VarSpecKind::PORT) પાછા mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                પાછા mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

શૂન્ય V3Control::applyCase(AstCase* nodep) {
    અચલ string& filename = nodep->fileline()->filename();
    V3ControlFile* અચલ filep = V3ControlResolver::s().files().resolve(filename);
    જો (filep) filep->applyCase(nodep);
}

શૂન્ય V3Control::applyCoverageBlock(Ast_ગાંઠ_Module* modulep, AstBegin* nodep) {
    અચલ string& filename = nodep->fileline()->filename();
    V3ControlFile* અચલ filep = V3ControlResolver::s().files().resolve(filename);
    જો (filep) filep->applyBlock(nodep);
    અચલ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* અચલ modp = V3ControlResolver::s().modules().resolve(modname);
    જો (modp) modp->applyBlock(nodep);
}
શૂન્ય V3Control::applyCoverageBlock(Ast_ગાંઠ_Module* modulep, AstGenBlock* nodep) {
    અચલ string& filename = nodep->fileline()->filename();
    V3ControlFile* અચલ filep = V3ControlResolver::s().files().resolve(filename);
    જો (filep) filep->applyBlock(nodep);
    અચલ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* અચલ modp = V3ControlResolver::s().modules().resolve(modname);
    જો (modp) modp->applyBlock(nodep);
}

શૂન્ય V3Control::applyIgnores(File_લીટી* filelinep) {
    અચલ string& filename = filelinep->filename();
    V3ControlFile* અચલ filep = V3ControlResolver::s().files().resolve(filename);
    જો (filep) filep->applyIgnores(filelinep);
}

શૂન્ય V3Control::applyModule(Ast_ગાંઠ_Module* modulep) {
    અચલ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* અચલ modp = V3ControlResolver::s().modules().resolve(modname);
    જો (modp) modp->apply(modulep);
}

શૂન્ય V3Control::applyFTask(Ast_ગાંઠ_Module* modulep, Ast_ગાંઠ_FTask* ftaskp) {
    અચલ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* અચલ modp = V3ControlResolver::s().modules().resolve(modname);
    જો (!modp) પાછા;
    અચલ V3ControlFTask* અચલ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    જો (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
સ્થિર શૂન્ય resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    અચલ std::string name = varp->prettyDehashOrigOrName();
    જો (અચલ V3ControlVar* અચલ vp = resolver.vars().resolve(name)) vp->apply(varp);
    જો (varp->isParam()) {
        જો (અચલ V3ControlVar* અચલ vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    જો (varp->isIO()) {
        જો (અચલ V3ControlVar* અચલ vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

શૂન્ય V3Control::applyVarAttr(અચલ Ast_ગાંઠ_Module* modulep, અચલ Ast_ગાંઠ_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* અચલ modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    જો (!modp) પાછા;
    જો (ftaskp) {
        V3ControlFTask* અચલ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        જો (!ftp) પાછા;
        resolveThenApply(*ftp, varp);
    } નહીંતર {
        resolveThenApply(*modp, varp);
    }
}

પૂર્ણાંક V3Control::getHierWorkers(અચલ string& model) {
    પાછા V3ControlResolver::s().getHierWorkers(model);
}
File_લીટી* V3Control::get_Hier_Workers_File_લીટી(અચલ string& model) {
    પાછા V3ControlResolver::s().get_Hier_Workers_File_લીટી(model);
}
અચલ V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(અચલ string& module) {
    V3ControlModule* અચલ modp = V3ControlResolver::s().modules().resolve(module);
    પાછા modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(અચલ string& hierDpi) {
    પાછા V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(અચલ string& model, અચલ string& ચાવી) {
    પાછા V3ControlResolver::s().getProfileData(model, ચાવી);
}
File_લીટી* V3Control::get_Profile_Data_File_લીટી() {
    પાછા V3ControlResolver::s().get_Profile_Data_File_લીટી();
}
તાર્કિક V3Control::getScopeTraceOn(અચલ string& scope) {
    પાછા V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

શૂન્ય V3Control::contents_ધકેલો_Text(અચલ string& text) { WildcardContents::ધકેલો_Text(text); }

તાર્કિક V3Control::containsMTaskProfileData() {
    પાછા V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    પાછા V3ControlResolver::s().getCurrentHierBlockCost();
}

તાર્કિક V3Control::waive(અચલ File_લીટી* filelinep, V3ErrorCode code, અચલ string& message) {
    V3ControlFile* અચલ filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    જો (!filep) પાછા ખોટું;
    પાછા filep->waive(code, message);
}

શૂન્ય V3Control::selfTest() { VInterval_વૃક્ષ::selfTest(); }
