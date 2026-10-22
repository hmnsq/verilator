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

#ضمن "V3PchAstMT.h"

#ضمن "V3Control.h"

#ضمن "V3InstrCount.h"
#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <memory>
#ضمن <set>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
کينډۍ <ډول_نوم T>
صنف V3ControlWildcardResolver final {
    بدلیدونکی V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ثابت std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ثابت std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

عام:
    V3ControlWildcardResolver() = افتراضي;
    ~V3ControlWildcardResolver() = افتراضي;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    فراغ update(ثابت V3ControlWildcardResolver& other) VL_MT_خوندي_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        ثابت V3_قلف_Guard other_قلف{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        لكل (ثابت خپلکاری& itr : other.m_mapPatterns) m_mapPatterns[itr.لومړی].update(itr.دوهم);
    }

    // Access and create a pattern entry
    T& at(ثابت string& name) VL_MT_خوندي_EXCLUDES(m_mutex) {
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        أرجع m_mapPatterns[V3Os::filename_Slash_لار(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ثابت string& name) VL_MT_خوندي_EXCLUDES(m_mutex) {
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ثابت خپلکاری pair = m_mapResolved.emplace(name, تش_اشاره);
        std::unique_ptr<T>& entryr = pair.لومړی->دوهم;
        // Resolve entry when first requested, cache the result
        إذا (pair.دوهم) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ثابت string slashName = V3Os::filename_Slash_لار(name);
            لكل (ثابت خپلکاری& patEnt : m_mapPatterns) {
                إذا (VString::wildmatch(slashName, patEnt.لومړی)) {
                    إذا (!entryr) entryr.reset(جديد T{});
                    entryr->update(patEnt.دوهم);
                }
            }
            // Perform final actions that needed all updates completed
            إذا (entryr) entryr->updateFinalize();
        }
        أرجع entryr.get();
    }
};

// List of attributes for variables
صنف V3ControlVar final {
    std::ویکټور<VAttrType> m_attrs;  // The list of attributes
عام:
    // Add new attribugte
    فراغ جمع_کړه(VAttrType attr) { m_attrs.emplace_شا(attr); }
    // Update from other by copying all attributes
    فراغ update(ثابت V3ControlVar& other) {
        m_attrs.reserve(m_attrs.اندازه() + other.m_attrs.اندازه());
        m_attrs.ورننباسه(m_attrs.پای_ټکی(), other.m_attrs.begin(), other.m_attrs.پای_ټکی());
    }
    فراغ updateFinalize() {}
    // Apply all attributes to the variable
    فراغ apply(AstVar* varp) ثابت {
        لكل (ثابت VAttrType attr : m_attrs) {
            varp->جمع_کړه_Attrsp(جديد Ast_Attr_د{varp->fileline(), attr});
        }
    }
};

کارول V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

صنف WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ثابت std::string, منطقي> m_mapPatterns;  // Pattern match results
    std::deque<string> m_کرښې;  // Source text lines

    // METHODS
    ساكن WildcardContents& s() {  // Singleton
        ساكن WildcardContents s_s;
        أرجع s_s;
    }
    فراغ clearCacheImp() { m_mapPatterns.clear(); }
    فراغ ټېل_متن_Imp(ثابت string& متن) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ثابت string leftover = m_کرښې.شا() + متن;
        m_کرښې.راوباسه_شا();

        // Insert line-by-line
        string::اندازه_type کرښه_پیل = 0;
        طالما (ریښتیا) {
            ثابت string::اندازه_type کرښه_پای_ټکی = leftover.پیدا_کړه('\n', کرښه_پیل);
            إذا (کرښه_پای_ټکی != string::npos) {
                ثابت string oneline(leftover, کرښه_پیل, کرښه_پای_ټکی - کرښه_پیل + 1);
                إذا (oneline.اندازه() > 1) m_کرښې.ټېل_شا(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_کرښې.اندازه() - 1) << "]: " << oneline);
                کرښه_پیل = کرښه_پای_ټکی + 1;
            } وإلا {
                اكسر;
            }
        }
        // Keep leftover for next time
        m_کرښې.emplace_شا(string(leftover, کرښه_پیل));  // Might be ""
        clearCacheImp();
    }

    منطقي resolveUncachedImp(ثابت string& contentsRegexp) {
        لكل (ثابت string& i : m_کرښې) {
            إذا (VString::wildmatch(i, contentsRegexp)) أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    منطقي resolveCachedImp(ثابت string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        إذا (contentsRegexp.تشه()) أرجع ریښتیا;
        ثابت خپلکاری pair = m_mapPatterns.emplace(contentsRegexp, خطأ);
        منطقي& entryr = pair.لومړی->دوهم;
        // Resolve entry when first requested, cache the result
        إذا (pair.دوهم) entryr = resolveUncachedImp(contentsRegexp);
        أرجع entryr;
    }

عام:
    WildcardContents() {
        m_کرښې.emplace_شا("");  // start with no leftover
    }
    ~WildcardContents() = افتراضي;
    // Return true iff contentsRegexp in parsed contents
    ساكن منطقي resolve(ثابت string& contentsRegexp) {
        أرجع s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ساكن فراغ ټېل_متن(ثابت string& متن) { s().ټېل_متن_Imp(متن); }
};

//######################################################################
// Function or task: Have variables and properties

صنف V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    منطقي m_noinline = خطأ;  // Don't inline function/task
    منطقي m_عامه = خطأ;  // Public function/task

عام:
    V3ControlFTask() = افتراضي;
    فراغ update(ثابت V3ControlFTask& f) {
        // Don't overwrite true with false
        إذا (f.m_noinline) m_noinline = ریښتیا;
        إذا (f.m_عامه) m_عامه = ریښتیا;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    فراغ updateFinalize() {}

    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ سېټ_No_Inline(منطقي سېټ) { m_noinline = سېټ; }
    فراغ سېټ_عامه(منطقي سېټ) { m_عامه = سېټ; }

    فراغ apply(Ast_غوټه_FTask* ftaskp) ثابت {
        إذا (m_noinline)
            ftaskp->جمع_کړه_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        إذا (m_عامه)
            ftaskp->جمع_کړه_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::عامه_TASK});
    }
};

کارول V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

صنف V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_ثبتوونکی_Wrapper m_fsm_ثبتوونکی_Wrapper;  // FSM wrapper descriptor
    std::unordered_سېټ<std::string> m_coverage_Off_بلاکونه;  // List of block names for coverage_off
    std::سېټ<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    منطقي m_has_Fsm_ثبتوونکی_Wrapper = خطأ;  // Whether descriptor has been set
    منطقي m_inline = خطأ;  // Whether to force the inline
    منطقي m_inline_ارزښت = خطأ;  // The inline value (on/off)

عام:
    V3ControlModule() = افتراضي;

    فراغ update(ثابت V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        إذا (m.m_has_Fsm_ثبتوونکی_Wrapper) {
            m_fsm_ثبتوونکی_Wrapper = m.m_fsm_ثبتوونکی_Wrapper;
            m_has_Fsm_ثبتوونکی_Wrapper = ریښتیا;
        }
        لكل (ثابت string& i : m.m_coverage_Off_بلاکونه) m_coverage_Off_بلاکونه.ورننباسه(i);
        إذا (!m_inline) {
            m_inline = m.m_inline;
            m_inline_ارزښت = m.m_inline_ارزښت;
        }
        لكل (خپلکاری it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ورننباسه(*it);
        }
    }
    فراغ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { أرجع m_tasks; }
    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ جمع_کړه_Coverage_بلاک_Off(ثابت string& name) { m_coverage_Off_بلاکونه.ورننباسه(name); }
    فراغ سېټ_Inline(منطقي سېټ) {
        m_inline = ریښتیا;
        m_inline_ارزښت = سېټ;
    }
    فراغ سېټ_Fsm_ثبتوونکی_Wrapper(File_کرښه* fl, ثابت V3Control::Fsm_ثبتوونکی_Wrapper& desc) {
        إذا (m_has_Fsm_ثبتوونکی_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_غوټه::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_ثبتوونکی_Wrapper = desc;
        m_has_Fsm_ثبتوونکی_Wrapper = ریښتیا;
    }
    ثابت V3Control::Fsm_ثبتوونکی_Wrapper* fsm_ثبتوونکی_Wrapperp() ثابت {
        أرجع m_has_Fsm_ثبتوونکی_Wrapper ? &m_fsm_ثبتوونکی_Wrapper : تش_اشاره;
    }
    فراغ جمع_کړه_Module_Pragma(VPragmaType pragma) { m_modPragmas.ورننباسه(pragma); }

    فراغ apply(Ast_غوټه_Module* modp) {
        إذا (m_inline) {
            ثابت VPragmaType type
                = m_inline_ارزښت ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_غوټه* ثابت nodep = جديد AstPragma{modp->fileline(), type};
            modp->جمع_کړه_Stmtsp(nodep);
        }
        لكل (ثابت خپلکاری& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            إذا (itr == VPragmaType::HIER_PARAMS) modp->hierParams(ریښتیا);
            Ast_غوټه* ثابت nodep = جديد AstPragma{modp->fileline(), itr};
            modp->جمع_کړه_Stmtsp(nodep);
        }
    }

    فراغ apply_بلاک(Ast_Gen_بلاک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلاکونه) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جمع_کړه_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    فراغ apply_بلاک(Ast_غوټه_بلاک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        File_کرښه* ثابت flp = nodep->fileline();
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلاکونه) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جمع_کړه_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
                }
            }
        }
    }
};

کارول V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
صنف V3_Control_Ignores_کرښه final {
عام:
    ثابت صحيح m_کرښه_Min;  // Minimum line number to make change at
    ثابت صحيح m_کرښه_Max;  // Maximum line number to make change at (inclusive)
    ثابت V3_Error_کوډ m_کوډ;  // Error code
    ثابت منطقي m_on;  // True to enable message
    V3_Control_Ignores_کرښه(V3_Error_کوډ کوډ, صحيح linemin, صحيح linemax, منطقي on)
        : m_کرښه_Min{linemin}
        , m_کرښه_Max{linemax}
        , m_کوډ{کوډ}
        , m_on{on} {}
    ~V3_Control_Ignores_کرښه() = افتراضي;
    منطقي every_کرښه() ثابت { أرجع m_کرښه_Min == 0 && m_کرښه_Max == 0; }
    منطقي دننه_Range(صحيح lineno) ثابت {
        أرجع (lineno >= m_کرښه_Min && lineno <= m_کرښه_Max) || every_کرښه();
    }
};
std::ostream& چلوونکی<<(std::ostream& os, ثابت V3_Control_Ignores_کرښه& rhs) {
    أرجع os << rhs.m_کرښه_Min << "-" << rhs.m_کرښه_Max << ", " << rhs.m_کوډ << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ساكن std::ویکټور<V3_Control_Ignores_کرښه> control_Ign_کرښې;

کارول IgnIndices = std::ویکټور<uint32_t>;  // List of {s_ignLines indces}

صنف VInterval_ونه final {
    بنية ثبت_2 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        صحيح m_center;  // Line number of central point
        std::unique_ptr<ثبت_2> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<ثبت_2> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        څرګند ثبت_2(صحيح center)
            : m_center{center} {}
    };

    std::unique_ptr<ثبت_2> m_rootp;  // Root of interval tree
    IgnIndices m_every_کرښې;  // All-line disables

خاص:
    std::unique_ptr<ثبت_2> build_ونه(ثابت IgnIndices& points) {
        إذا (points.تشه()) أرجع تش_اشاره;

        صحيح minval = std::numeric_limits<صحيح>::max();
        صحيح maxval = std::numeric_limits<صحيح>::min();
        لكل (ثابت خپلکاری& it : points) {
            ثابت V3_Control_Ignores_کرښه& cign = control_Ign_کرښې[it];
            إذا (cign.every_کرښه()) {
                m_every_کرښې.emplace_شا(it);
                استمر;
            }
            minval = std::min(minval, cign.m_کرښه_Min);
            maxval = std::max(maxval, cign.m_کرښه_Max);
        }

        ثابت صحيح center = minval + (maxval - minval) / 2;
        خپلکاری entp = std::unique_ptr<ثبت_2>(جديد ثبت_2{center});

        IgnIndices کیڼ_Points;
        IgnIndices ښی_Points;
        لكل (ثابت خپلکاری& it : points) {
            ثابت V3_Control_Ignores_کرښه& cign = control_Ign_کرښې[it];
            إذا (cign.every_کرښه()) استمر;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            إذا (cign.m_کرښه_Max < center)
                کیڼ_Points.emplace_شا(it);
            وإلا إذا (cign.m_کرښه_Min > center)
                ښی_Points.emplace_شا(it);
            وإلا {
                entp->m_byMin.ټېل_شا(it);
                entp->m_byMax.ټېل_شا(it);
            }
        }

        std::ترتیبول(entp->m_byMin.begin(), entp->m_byMin.پای_ټکی(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_کرښې[a].m_کرښه_Min < control_Ign_کرښې[b].m_کرښه_Min;
        });
        std::ترتیبول(entp->m_byMax.begin(), entp->m_byMax.پای_ټکی(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_کرښې[a].m_کرښه_Max > control_Ign_کرښې[b].m_کرښه_Max;
        });

        entp->m_leftp = build_ونه(کیڼ_Points);
        entp->m_rightp = build_ونه(ښی_Points);
        أرجع entp;
    }

    فراغ پیدا_کړه_ونه(صحيح lineno, ثبت_2* entp, IgnIndices& resultsr, صحيح& بل_Changer) ثابت {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        إذا (lineno < entp->m_center) {
            بل_Changer = std::min(بل_Changer, entp->m_center);
            لكل (ثابت خپلکاری& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_کرښه& cign = control_Ign_کرښې[it];
                إذا (cign.m_کرښه_Min <= lineno) {
                    resultsr.emplace_شا(it);
                } وإلا {
                    بل_Changer = std::min(بل_Changer, cign.m_کرښه_Min);
                    اكسر;
                }
            }
            إذا (entp->m_leftp) پیدا_کړه_ونه(lineno, entp->m_leftp.get(), resultsr, بل_Changer);
        } وإلا إذا (lineno > entp->m_center) {
            لكل (ثابت خپلکاری& it : entp->m_byMax) {
                ثابت V3_Control_Ignores_کرښه& cign = control_Ign_کرښې[it];
                إذا (cign.m_کرښه_Max >= lineno) {
                    بل_Changer = std::min(بل_Changer, cign.m_کرښه_Max + 1);
                    resultsr.emplace_شا(it);
                } وإلا {
                    اكسر;
                }
            }
            إذا (entp->m_rightp) پیدا_کړه_ونه(lineno, entp->m_rightp.get(), resultsr, بل_Changer);
        } وإلا {
            لكل (ثابت خپلکاری& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_کرښه& cign = control_Ign_کرښې[it];
                resultsr.emplace_شا(it);
                إذا (cign.m_کرښه_Max >= lineno)
                    بل_Changer = std::min(بل_Changer, cign.m_کرښه_Max + 1);
            }
            إذا (entp->m_leftp) پیدا_کړه_ونه(lineno, entp->m_leftp.get(), resultsr, بل_Changer);
            إذا (entp->m_rightp) پیدا_کړه_ونه(lineno, entp->m_rightp.get(), resultsr, بل_Changer);
        }
    }

عام:
    // CONSTRUCTORS
    VInterval_ونه() = افتراضي;
    ~VInterval_ونه() = افتراضي;
    // METHODS
    فراغ clear() { m_rootp = تش_اشاره; }
    فراغ build(ثابت IgnIndices& points) {
        clear();
        m_rootp = build_ونه(points);
    }
    فراغ پیدا_کړه(صحيح lineno, IgnIndices& resultsr, صحيح& بل_Changer) ثابت {
        resultsr = m_every_کرښې;
        بل_Changer = std::numeric_limits<صحيح>::max();

        إذا (m_rootp) پیدا_کړه_ونه(lineno, m_rootp.get(), resultsr, بل_Changer);
        // Sort indices, so can process in parse order
        std::ترتیبول(resultsr.begin(), resultsr.پای_ټکی());
    }

    ساكن فراغ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices معلومات;
        std::ویکټور<std::pair<صحيح, صحيح>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        لكل (ثابت خپلکاری& it : points) {
            control_Ign_کرښې.emplace_شا(
                V3_Control_Ignores_کرښه{V3_Error_کوډ::I_LINT, it.لومړی, it.دوهم, ریښتیا});
            معلومات.emplace_شا(static_cast<uint32_t>(control_Ign_کرښې.اندازه() - 1));
        }
        VInterval_ونه ونه;
        ونه.build(معلومات);

        IgnIndices results;
        صحيح بل_Change = 0;
        ونه.پیدا_کړه(0, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 0);
        UASSERT_SELFTEST(بل_Change, 10);
        ونه.پیدا_کړه(10, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(بل_Change, 11);
        ونه.پیدا_کړه(11, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(بل_Change, 15);  // Center, or would be 20
        ونه.پیدا_کړه(20, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(بل_Change, 21);
        ونه.پیدا_کړه(21, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(بل_Change, 25);  // Center, or would be 30
        ونه.پیدا_کړه(30, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(بل_Change, 31);
        ونه.پیدا_کړه(40, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(بل_Change, 41);
        ونه.پیدا_کړه(41, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 0);
        UASSERT_SELFTEST(بل_Change, std::numeric_limits<صحيح>::max());
        //
        points = {{0, 0}};
        لكل (ثابت خپلکاری& it : points) {
            control_Ign_کرښې.emplace_شا(
                V3_Control_Ignores_کرښه{V3_Error_کوډ::I_LINT, it.لومړی, it.دوهم, ریښتیا});
            معلومات.emplace_شا(static_cast<uint32_t>(control_Ign_کرښې.اندازه() - 1));
        }
        ونه.build(معلومات);
        //
        ونه.پیدا_کړه(50, results, بل_Change);
        UASSERT_SELFTEST(results.اندازه(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
کارول V3_Control_کرښه_Attribute = std::bitset<VPragmaType::ENUM_اندازه>;

صنف WaiverSetting final {
عام:
    V3_Error_کوډ m_کوډ;  // Error code
    string m_contents;  // --contents regexp
    string m_سمون;  // --match regexp
    WaiverSetting(V3_Error_کوډ کوډ, ثابت string& contents, ثابت string& سمون)
        : m_کوډ{کوډ}
        , m_contents{contents}
        , m_سمون{سمون} {}
    ~WaiverSetting() = افتراضي;
    WaiverSetting& چلوونکی=(ثابت WaiverSetting& rhs) = افتراضي;
};

// File entity
صنف V3ControlFile final {
    کارول کرښه_Attr_Map = std::map<صحيح, V3_Control_کرښه_Attribute>;  // Map line->bitset of attributes
    کارول Waivers = std::ویکټور<WaiverSetting>;  // List of {code,wildcard string}

    کرښه_Attr_Map m_کرښه_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_ونه m_interval_ونه;  // Tree of indices to IgnLines

    بنية {
        صحيح filenameno = -1;  // Last filename
        صحيح lineno = -1;  // Last linenumber
        صحيح بل_Change = -1;  // Line number of next change
    } m_وروستی_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    منطقي کرښه_سمون(صحيح lineno, VPragmaType type) {
        إذا (m_کرښه_Attrs.پیدا_کړه(0) != m_کرښه_Attrs.پای_ټکی() && m_کرښه_Attrs[0][type]) أرجع ریښتیا;
        إذا (m_کرښه_Attrs.پیدا_کړه(lineno) == m_کرښه_Attrs.پای_ټکی()) أرجع خطأ;
        أرجع m_کرښه_Attrs[lineno][type];
    }

عام:
    V3ControlFile() = افتراضي;
    فراغ update(ثابت V3ControlFile& file) {
        // Copy in all attributes and waivers
        لكل (ثابت خپلکاری& itr : file.m_کرښه_Attrs) m_کرښه_Attrs[itr.لومړی] |= itr.دوهم;
        m_waivers.reserve(m_waivers.اندازه() + file.m_waivers.اندازه());
        m_waivers.ورننباسه(m_waivers.پای_ټکی(), file.m_waivers.begin(), file.m_waivers.پای_ټکی());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.اندازه() + file.m_ignIndices.اندازه());
        m_ignIndices.ورننباسه(m_ignIndices.پای_ټکی(), file.m_ignIndices.begin(),
                            file.m_ignIndices.پای_ټکی());
        // updateFinalize() will soon build tree
    }
    فراغ updateFinalize() { m_interval_ونه.build(m_ignIndices); }
    فراغ جمع_کړه_کرښه_Attribute(صحيح lineno, VPragmaType attr) { m_کرښه_Attrs[lineno].سېټ(attr); }
    فراغ جمع_کړه_Ignore(V3_Error_کوډ کوډ, صحيح min, صحيح max, منطقي on) {
        control_Ign_کرښې.emplace_شا(V3_Control_Ignores_کرښه{کوډ, min, max, on});
        m_ignIndices.emplace_شا(static_cast<uint32_t>(control_Ign_کرښې.اندازه() - 1));
        m_وروستی_Ignore.بل_Change = -1;
    }
    فراغ جمع_کړه_Ignore_سمون(V3_Error_کوډ کوډ, ثابت string& contents, ثابت string& سمون) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_سمون = سمون;
        إذا (new_سمون.تشه() || new_سمون.شا() != '*') new_سمون += '*';
        m_waivers.emplace_شا(کوډ, contents, new_سمون);
    }

    فراغ apply_بلاک(Ast_Gen_بلاک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        إذا (کرښه_سمون(nodep->fileline()->lineno(), pragma)) {
            nodep->جمع_کړه_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
        }
    }
    فراغ apply_بلاک(Ast_غوټه_بلاک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        File_کرښه* ثابت flp = nodep->fileline();
        إذا (کرښه_سمون(flp->lineno(), pragma)) {
            nodep->جمع_کړه_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
        }
    }
    فراغ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ثابت صحيح lineno = nodep->fileline()->lineno();
        إذا (کرښه_سمون(lineno, VPragmaType::ډک_CASE)) nodep->ډک_Pragma(ریښتیا);
        إذا (کرښه_سمون(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(ریښتیا);
    }
    فراغ applyIgnores(File_کرښه* filelinep) {
        // HOT routine, called each parsed token line of this filename
        إذا (filelinep->filenameno() == m_وروستی_Ignore.filenameno
            && filelinep->lineno() == m_وروستی_Ignore.lineno)
            أرجع;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        إذا (filelinep->filenameno() == m_وروستی_Ignore.filenameno
            && filelinep->lineno() < m_وروستی_Ignore.بل_Change) {
            m_وروستی_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            أرجع;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_وروستی_Ignore.filenameno = filelinep->filenameno();
        m_وروستی_Ignore.lineno = filelinep->lineno();
        m_وروستی_Ignore.بل_Change = std::numeric_limits<صحيح>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_ونه.پیدا_کړه(filelinep->lineno(), results /*ref*/, m_وروستی_Ignore.بل_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_بټ_سېټ bitset{VError_بټ_سېټ::AllOnes{}};
        لكل (ثابت خپلکاری& it : results) {
            ثابت V3_Control_Ignores_کرښه& cign = control_Ign_کرښې[it];
            UASSERT(cign.دننه_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_کوډ.forDelegateCodes(
                [&](V3_Error_کوډ subcode) { bitset.سېټ(subcode, cign.m_on); });
        }

        filelinep->warn_سېټ_Ctrl_بټ_سېټ(bitset);
        m_وروستی_Ignore.بل_Change = std::max(m_وروستی_Ignore.بل_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_وروستی_Ignore.بل_Change
                                       << " Messages: " << bitset.ascii());
    }
    منطقي waive(V3_Error_کوډ کوډ, ثابت string& پیغام) {
        إذا (کوډ.hardError()) أرجع خطأ;
        لكل (ثابت خپلکاری& itr : m_waivers) {
            إذا ((کوډ.isUnder(itr.m_کوډ) || (itr.m_کوډ == V3_Error_کوډ::I_LINT))
                && VString::wildmatch(پیغام, itr.m_سمون)
                && WildcardContents::resolve(itr.m_contents)) {
                أرجع ریښتیا;
            }
        }
        أرجع خطأ;
    }
};

کارول V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

صنف V3_Control_Scope_Trace_ثبت final {
عام:
    ثابت string m_scope;  // Scope or regexp to match
    ثابت منطقي m_on;  // True to enable message
    صحيح m_پوړونه = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_ثبت(ثابت string& scope, منطقي on, صحيح پوړونه)
        : m_scope{scope}
        , m_on{on}
        , m_پوړونه{پوړونه} {}
    ~V3_Control_Scope_Trace_ثبت() = افتراضي;
    منطقي چلوونکی<(ثابت V3_Control_Scope_Trace_ثبت& other) ثابت {
        إذا (m_on < other.m_on) أرجع ریښتیا;
        إذا (m_on > other.m_on) أرجع خطأ;
        إذا (m_پوړونه < other.m_پوړونه) أرجع ریښتیا;
        إذا (m_پوړونه > other.m_پوړونه) أرجع خطأ;
        أرجع m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
صنف V3_Control_Scope_Trace_ثبت_سمون final {
عام:
    ثابت V3_Control_Scope_Trace_ثبت* m_entryp;  // Entry the match refers to
    ثابت string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_ثبت_سمون(ثابت V3_Control_Scope_Trace_ثبت* entryp, ثابت string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    منطقي چلوونکی<(ثابت V3_Control_Scope_Trace_ثبت_سمون& other) ثابت {
        إذا (m_entryp < other.m_entryp) أرجع ریښتیا;
        إذا (m_entryp > other.m_entryp) أرجع خطأ;
        أرجع m_scopepart < other.m_scopepart;
    }
};

صنف V3ControlScopeTraceResolver final {
    std::ویکټور<V3_Control_Scope_Trace_ثبت> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_ثبت_سمون, منطقي> m_سمون_Cache;  // Matching entries for speed

عام:
    فراغ جمع_کړه_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح پوړونه) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << پوړونه);
        m_entries.emplace_شا(V3_Control_Scope_Trace_ثبت{scope, on, پوړونه});
        m_سمون_Cache.clear();
    }

    منطقي get_ثبت_سمون(ثابت V3_Control_Scope_Trace_ثبت* entp, ثابت string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ثابت V3_Control_Scope_Trace_ثبت_سمون کلی{entp, scopepart};
        ثابت خپلکاری pair = m_سمون_Cache.emplace(کلی, خطأ);
        إذا (pair.دوهم) pair.لومړی->دوهم = VString::wildmatch(scopepart, entp->m_scope);
        أرجع pair.لومړی->دوهم;
    }

    منطقي getScopeTraceOn(ثابت string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        صحيح maxLevel = 1;
        لكل (ثابت خپلکاری& ch : scope) {
            إذا (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        منطقي enabled = ریښتیا;
        لكل (ثابت خپلکاری& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            صحيح partLevel = 1;
            لكل (string::اندازه_type part_پای_ټکی = 0; ریښتیا;) {
                part_پای_ټکی = scope.پیدا_کړه('.', part_پای_ټکی + 1);
                إذا (part_پای_ټکی == string::npos) part_پای_ټکی = scope.اوږدتیا();
                ثابت std::string scopepart = scope.substr(0, part_پای_ټکی);
                إذا (get_ثبت_سمون(&ent, scopepart)) {
                    ثابت منطقي level_سمون
                        = !ent.m_پوړونه || (ent.m_پوړونه >= maxLevel - partLevel);
                    إذا (level_سمون) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_سمون ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    اكسر;
                }
                إذا (part_پای_ټکی == scope.اوږدتیا()) اكسر;
                ++partLevel;
            }
        }
        أرجع enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

صنف V3_Control_Resolver_Hier_کارګر_ثبت final {
    ثابت صحيح m_workers;  // Number of workers requested for the hierarchical block
    File_کرښه* ثابت m_flp;  // Location the worker count was specified at

عام:
    څرګند V3_Control_Resolver_Hier_کارګر_ثبت(صحيح workers, File_کرښه* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    صحيح workers() ثابت { أرجع m_workers; }
    File_کرښه* flp() ثابت { أرجع m_flp; }
};

صنف V3ControlResolver final {
    تعداد Profile_معلومات_ډېرپېښ : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_معلومات;  // Access to profile_data records
    uint8_t m_ډېرپېښ = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_کارګر_ثبت> m_hierWorkers;
    File_کرښه* m_profile_File_کرښه = تش_اشاره;  // Location profile_data was read from

    V3ControlResolver() = افتراضي;
    ~V3ControlResolver() = افتراضي;

عام:
    ساكن V3ControlResolver& s() {
        ساكن V3ControlResolver s_singleton;
        أرجع s_singleton;
    }
    V3ControlModuleResolver& modules() { أرجع m_modules; }
    V3ControlFileResolver& files() { أرجع m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { أرجع m_scopeTraces; }

    فراغ جمع_کړه_Profile_معلومات(File_کرښه* fl, ثابت string& hierDpi, uint64_t لګښت) {
        // Empty key for hierarchical DPI wrapper costs.
        جمع_کړه_Profile_معلومات(fl, hierDpi, "", لګښت, HIER_DPI);
    }
    فراغ جمع_کړه_Profile_معلومات(File_کرښه* fl, ثابت string& model, ثابت string& کلی, uint64_t لګښت,
                        Profile_معلومات_ډېرپېښ ډېرپېښ = MTASK) {
        إذا (!m_profile_File_کرښه) m_profile_File_کرښه = fl;
        إذا (لګښت == 0) لګښت = 1;  // Cost 0 means delete (or no data)
        إذا (ډېرپېښ == MTASK) {
            m_profile_معلومات[model][کلی] += لګښت;
        } وإلا إذا (ډېرپېښ == HIER_DPI) {
            m_profile_معلومات[model][کلی] = std::max(m_profile_معلومات[model][کلی], لګښت);
        }
        m_ډېرپېښ |= ډېرپېښ;
    }
    منطقي لري_MTask_Profile_معلومات() ثابت { أرجع m_ډېرپېښ & MTASK; }
    uint64_t get_Profile_معلومات(ثابت string& hierDpi) ثابت {
        // Empty key for hierarchical DPI wrapper costs.
        أرجع get_Profile_معلومات(hierDpi, "");
    }
    فراغ جمع_کړه_Hier_Workers(File_کرښه* flp, ثابت string& model, صحيح workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    صحيح getHierWorkers(ثابت string& model) ثابت {
        ثابت خپلکاری mit = m_hierWorkers.پیدا_کړه(model);
        // Assign a single worker if no specified.
        أرجع mit != m_hierWorkers.cend() ? mit->دوهم.workers() : 0;
    }
    File_کرښه* get_Hier_Workers_File_کرښه(ثابت string& model) ثابت {
        ثابت خپلکاری mit = m_hierWorkers.پیدا_کړه(model);
        أرجع mit != m_hierWorkers.cend() ? mit->دوهم.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_معلومات(ثابت string& model, ثابت string& کلی) ثابت {
        ثابت خپلکاری mit = m_profile_معلومات.پیدا_کړه(model);
        إذا (mit == m_profile_معلومات.cend()) أرجع 0;
        ثابت خپلکاری it = mit->دوهم.پیدا_کړه(کلی);
        إذا (it == mit->دوهم.cend()) أرجع 0;
        أرجع it->دوهم;
    }
    File_کرښه* get_Profile_معلومات_File_کرښه() ثابت { أرجع m_profile_File_کرښه; }  // Maybe null
    ساكن uint64_t get_Current_Hier_بلاک_لګښت() {
        إذا (uint64_t لګښت = V3Control::get_Profile_معلومات(v3Global.opt.مختاړی())) {
            UINFO(9, "Fetching cost from profile info: " << لګښت);
            أرجع لګښت;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ثابت netlistp = v3Global.rootp();
        uint64_t لګښت = 0;
        لكل (صحيح i = 0; i < VEval::ENUM_پای_ټکی; ++i) {
            ثابت VEval eval{i};
            إذا (eval.slow()) استمر;
            لګښت += V3_Instr_شمېر::شمېر(netlistp->evalFuncp(eval), خطأ);
        }
        UINFO(9, "Evaluating cost: " << لګښت);
        أرجع لګښت;
    }
};

//######################################################################
// V3Control

فراغ V3Control::جمع_کړه_Case_ډک(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جمع_کړه_کرښه_Attribute(lineno, VPragmaType::ډک_CASE);
}
فراغ V3Control::جمع_کړه_Case_Parallel(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جمع_کړه_کرښه_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
فراغ V3Control::جمع_کړه_Coverage_بلاک_Off(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جمع_کړه_کرښه_Attribute(lineno, VPragmaType::COVERAGE_بلاک_OFF);
}
فراغ V3Control::جمع_کړه_Coverage_بلاک_Off(ثابت string& module, ثابت string& blockname) {
    V3ControlResolver::s().modules().at(module).جمع_کړه_Coverage_بلاک_Off(blockname);
}

فراغ V3Control::جمع_کړه_Hier_Workers(File_کرښه* fl, ثابت string& model, صحيح workers) {
    V3ControlResolver::s().جمع_کړه_Hier_Workers(fl, model, workers);
}

فراغ V3Control::جمع_کړه_Fsm_ثبتوونکی_Wrapper(File_کرښه* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_ارزښت) {
    string missing;
    إذا (module.تشه()) missing += "-module";
    إذا (d.تشه()) missing = VString::dot(missing, ", ", "-d");
    إذا (q.تشه()) missing = VString::dot(missing, ", ", "-q");
    إذا (clock.تشه()) missing = VString::dot(missing, ", ", "-clock");
    إذا (!missing.تشه()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        أرجع;
    }

    ثابت Fsm_ثبتوونکی_Wrapper desc{module, d, q, clock, reset, reset_ارزښت};
    V3ControlResolver::s().modules().at(module).سېټ_Fsm_ثبتوونکی_Wrapper(fl, desc);
}

فراغ V3Control::جمع_کړه_Ignore(V3_Error_کوډ کوډ, منطقي on, ثابت string& filename, صحيح min, صحيح max) {
    UINFO(9, "addIgnore " << کوډ << " " << min << "-" << max << " fn=" << filename);
    إذا (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_کرښه::globalWarnOff(کوډ, !on);
    } وإلا {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).جمع_کړه_Ignore(کوډ, min, max, on);
    }
}
فراغ V3Control::جمع_کړه_Ignore_سمون(V3_Error_کوډ کوډ, ثابت string& filename, ثابت string& contents,
                               ثابت string& سمون) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).جمع_کړه_Ignore_سمون(کوډ, contents, سمون);
}

فراغ V3Control::جمع_کړه_Inline(File_کرښه* fl, ثابت string& module, ثابت string& ftask, منطقي on) {
    إذا (ftask.تشه()) {
        V3ControlResolver::s().modules().at(module).سېټ_Inline(on);
    } وإلا {
        إذا (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } وإلا {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سېټ_No_Inline(on);
        }
    }
}

فراغ V3Control::جمع_کړه_Module_Pragma(ثابت string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).جمع_کړه_Module_Pragma(pragma);
}

فراغ V3Control::جمع_کړه_Profile_معلومات(File_کرښه* fl, ثابت string& hierDpi, uint64_t لګښت) {
    V3ControlResolver::s().جمع_کړه_Profile_معلومات(fl, hierDpi, لګښت);
}
فراغ V3Control::جمع_کړه_Profile_معلومات(File_کرښه* fl, ثابت string& model, ثابت string& کلی,
                               uint64_t لګښت) {
    V3ControlResolver::s().جمع_کړه_Profile_معلومات(fl, model, کلی, لګښت);
}

فراغ V3Control::جمع_کړه_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح پوړونه) {
    V3ControlResolver::s().scopeTraces().جمع_کړه_Scope_Trace_On(on, scope, پوړونه);
}

فراغ V3Control::جمع_کړه_Var_Attr(File_کرښه* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_ډول ډول, ثابت string& نمونه_2, VAttrType attr,
                           Ast_Sen_ونه* sensep) {
    إذا (sensep) {
        File_کرښه* ثابت flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_ونه(), sensep);
        // Used to be only accepted on public_flat_rw
        إذا (attr != VAttrType::VAR_عامه_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            أرجع;
        }
    }

    إذا (ډول != Var_Spec_ډول::VAR) {
        بدل (attr) {
        حالة VAttrType::VAR_عامه_FLAT:
        حالة VAttrType::VAR_عامه_FLAT_RD:
        حالة VAttrType::VAR_عامه_FLAT_RW: اكسر;
        افتراضي:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            أرجع;
        }
    }

    // Semantics: Most of the attributes operate on signals
    إذا (نمونه_2.تشه()) {
        إذا (attr == VAttrType::VAR_عامه) {
            إذا (ftask.تشه()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).جمع_کړه_Module_Pragma(
                    VPragmaType::عامه_MODULE);
            } وإلا {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سېټ_عامه(ریښتیا);
            }
        } وإلا {
            fl->v3error("missing -var");
        }
    } وإلا {
        إذا (attr == VAttrType::VAR_FORCEABLE) {
            إذا (module.تشه()) {
                fl->v3error("forceable missing -module");
            } وإلا إذا (!ftask.تشه()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } وإلا {
                V3ControlResolver::s().modules().at(module).vars().at(نمونه_2).جمع_کړه(attr);
            }
        } وإلا {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                إذا (ftask.تشه()) {
                    إذا (ډول == Var_Spec_ډول::PARAM) أرجع mod.params().at(نمونه_2);
                    إذا (ډول == Var_Spec_ډول::پورټ) أرجع mod.ports().at(نمونه_2);
                    UASSERT_OBJ(ډول == Var_Spec_ډول::VAR, fl, "Unexpected VarSpecKind");
                    أرجع mod.vars().at(نمونه_2);
                }
                إذا (ډول == Var_Spec_ډول::PARAM) أرجع mod.ftasks().at(ftask).params().at(نمونه_2);
                إذا (ډول == Var_Spec_ډول::پورټ) أرجع mod.ftasks().at(ftask).ports().at(نمونه_2);
                UASSERT_OBJ(ډول == Var_Spec_ډول::VAR, fl, "Unexpected VarSpecKind");
                أرجع mod.ftasks().at(ftask).vars().at(نمونه_2);
            }();
            controlVar.جمع_کړه(attr);
        }
    }
}

فراغ V3Control::applyCase(AstCase* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyCase(nodep);
}

فراغ V3Control::apply_Coverage_بلاک(Ast_غوټه_Module* modulep, AstBegin* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلاک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلاک(nodep);
}
فراغ V3Control::apply_Coverage_بلاک(Ast_غوټه_Module* modulep, Ast_Gen_بلاک* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلاک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلاک(nodep);
}

فراغ V3Control::applyIgnores(File_کرښه* filelinep) {
    ثابت string& filename = filelinep->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyIgnores(filelinep);
}

فراغ V3Control::applyModule(Ast_غوټه_Module* modulep) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply(modulep);
}

فراغ V3Control::applyFTask(Ast_غوټه_Module* modulep, Ast_غوټه_FTask* ftaskp) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (!modp) أرجع;
    ثابت V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    إذا (ftp) ftp->apply(ftaskp);
}

کينډۍ <ډول_نوم T_Resolver>
ساكن فراغ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ثابت std::string name = varp->prettyDehashOrigOrName();
    إذا (ثابت V3ControlVar* ثابت vp = resolver.vars().resolve(name)) vp->apply(varp);
    إذا (varp->isParam()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    إذا (varp->isIO()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

فراغ V3Control::applyVarAttr(ثابت Ast_غوټه_Module* modulep, ثابت Ast_غوټه_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ثابت modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    إذا (!modp) أرجع;
    إذا (ftaskp) {
        V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        إذا (!ftp) أرجع;
        resolveThenApply(*ftp, varp);
    } وإلا {
        resolveThenApply(*modp, varp);
    }
}

صحيح V3Control::getHierWorkers(ثابت string& model) {
    أرجع V3ControlResolver::s().getHierWorkers(model);
}
File_کرښه* V3Control::get_Hier_Workers_File_کرښه(ثابت string& model) {
    أرجع V3ControlResolver::s().get_Hier_Workers_File_کرښه(model);
}
ثابت V3Control::Fsm_ثبتوونکی_Wrapper* V3Control::get_Fsm_ثبتوونکی_Wrapper(ثابت string& module) {
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(module);
    أرجع modp ? modp->fsm_ثبتوونکی_Wrapperp() : تش_اشاره;
}
uint64_t V3Control::get_Profile_معلومات(ثابت string& hierDpi) {
    أرجع V3ControlResolver::s().get_Profile_معلومات(hierDpi);
}
uint64_t V3Control::get_Profile_معلومات(ثابت string& model, ثابت string& کلی) {
    أرجع V3ControlResolver::s().get_Profile_معلومات(model, کلی);
}
File_کرښه* V3Control::get_Profile_معلومات_File_کرښه() {
    أرجع V3ControlResolver::s().get_Profile_معلومات_File_کرښه();
}
منطقي V3Control::getScopeTraceOn(ثابت string& scope) {
    أرجع V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

فراغ V3Control::contents_ټېل_متن(ثابت string& متن) { WildcardContents::ټېل_متن(متن); }

منطقي V3Control::لري_MTask_Profile_معلومات() {
    أرجع V3ControlResolver::s().لري_MTask_Profile_معلومات();
}
uint64_t V3Control::get_Current_Hier_بلاک_لګښت() {
    أرجع V3ControlResolver::s().get_Current_Hier_بلاک_لګښت();
}

منطقي V3Control::waive(ثابت File_کرښه* filelinep, V3_Error_کوډ کوډ, ثابت string& پیغام) {
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    إذا (!filep) أرجع خطأ;
    أرجع filep->waive(کوډ, پیغام);
}

فراغ V3Control::selfTest() { VInterval_ونه::selfTest(); }
