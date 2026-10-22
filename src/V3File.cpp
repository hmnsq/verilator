// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#ضمن "V3File.h"

#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <cerrno>
#ضمن <cstdarg>
#ضمن <fcntl.h>
#ضمن <iomanip>
#ضمن <map>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

// clang-format off
#إذا defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#د_که_پای

#که_تعریف_شوی HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#وإلا
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#د_که_پای

#که_تعریف_شوی INFILTER_PIPE
# include <sys/wait.h>
#د_که_پای

#إذا defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#د_که_پای
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr صحيح INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr صحيح INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_اندازه_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

صنف V3FileDependImp final {
    // TYPES
    صنف DependFile final {
        // A single file
        ثابت منطقي m_موخه;  // True if write, else read
        منطقي m_exists = ریښتیا;  // File exists
        ثابت string m_filename;  // Filename
        بنية stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    عام:
        DependFile(ثابت string& filename, منطقي موخه)
            : m_موخه{موخه}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = افتراضي;
        ثابت string& filename() ثابت { أرجع m_filename; }
        منطقي موخه() ثابت { أرجع m_موخه; }
        منطقي exists() ثابت { أرجع m_exists; }
        off_t اندازه() ثابت { أرجع m_stat.st_اندازه; }
        ino_t ino() ثابت { أرجع m_stat.st_ino; }
        وخت_t cstime() ثابت { أرجع m_stat.st_ctime; }  // Seconds
        وخت_t cnstime() ثابت { أرجع VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        وخت_t mstime() ثابت { أرجع m_stat.st_mtime; }  // Seconds
        وخت_t mnstime() ثابت { أرجع VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_نښه() {
            ساكن VHashSha512 s_تشه_Hash;
            أرجع m_hash.digestSymbol24() != s_تشه_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        فراغ بارول_Stats() {
            إذا (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحيح err = stat(fn.c_str(), &m_stat);
                إذا (err != 0) {
                    std::memset(&m_stat, 0, حجم(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = خطأ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } وإلا {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    إذا (!موخه() && اندازه() <= FILE_HASH_اندازه_MAX) m_hash.ورننباسه_File(filename());
                }
            }
        }
        منطقي چلوونکی<(ثابت DependFile& rhs) ثابت { أرجع filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::سېټ<string> m_filename_سېټ VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::سېټ<DependFile> m_filenameList;  // Files sourced/generated

    ساكن string stripQuotes(ثابت string& دننه) {
        string pretty = دننه;
        string::اندازه_type pos;
        طالما ((pos = pretty.پیدا_کړه('\"')) != string::npos) pretty.replace(pos, 1, "_");
        طالما ((pos = pretty.پیدا_کړه('\n')) != string::npos) pretty.replace(pos, 1, "_");
        أرجع pretty;
    }

عام:
    // ACCESSOR METHODS
    فراغ جمع_کړه_Src_Depend(ثابت string& filename) VL_MT_خوندي_EXCLUDES(m_mutex) {
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        ثابت خپلکاری it_وموندل_Pair = m_filename_سېټ.ورننباسه(filename);
        إذا (it_وموندل_Pair.دوهم) {
            DependFile df{filename, خطأ};
            df.بارول_Stats();  // Get size now, in case changes during the run
            m_filenameList.ورننباسه(df);
        }
    }
    فراغ جمع_کړه_Tgt_Depend(ثابت string& filename) VL_MT_خوندي_EXCLUDES(m_mutex) {
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        ثابت خپلکاری it_وموندل_Pair = m_filename_سېټ.ورننباسه(filename);
        إذا (it_وموندل_Pair.دوهم) m_filenameList.ورننباسه(DependFile{filename, ریښتیا});
    }
    فراغ writeDepend(ثابت string& filename);
    std::ویکټور<string> getAllDeps() ثابت;
    فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_دننه);
    منطقي checkTimes(ثابت string& filename, ثابت string& cmdline_دننه);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

فراغ V3FileDependImp::writeDepend(ثابت string& filename) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (i.موخه()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (!i.موخه()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    إذا (v3Global.opt.makePhony()) {
        *ofp << '\n';
        لكل (ثابت DependFile& i : m_filenameList) {
            إذا (!i.موخه()) *ofp << i.filename() << ":\n";
        }
    }
}

std::ویکټور<string> V3FileDependImp::getAllDeps() ثابت {
    std::ویکټور<string> r;
    لكل (ثابت خپلکاری& itr : m_filenameList) {
        إذا (!itr.موخه() && itr.exists()) r.ټېل_شا(itr.filename());
    }
    أرجع r;
}

فراغ V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdline_دننه) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    ثابت string cmdline = stripQuotes(cmdline_دننه);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    لكل (خپلکاری& it : m_filenameList) {
        DependFile* ثابت depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->بارول_Stats();
        off_t show_اندازه = depfilep->اندازه();
        ino_t showIno = depfilep->ino();
        إذا (depfilep->filename() == filename) {
            show_اندازه = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->موخه() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_اندازه;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_نښه() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

منطقي V3FileDependImp::checkTimes(ثابت string& filename, ثابت string& cmdline_دننه) {
    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    إذا (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        أرجع خطأ;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        إذا (ignore.تشه()) { /*کارول_شوی*/
        }
    }
    {
        حرف chkDir;
        *ifp >> chkDir;
        حرف quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdline_دننه);
        إذا (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            أرجع خطأ;
        }
    }

    ثابت منطقي skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").تشه();

    طالما (!ifp->eof()) {
        حرف chkDir;
        *ifp >> chkDir;
        off_t chk_اندازه;
        *ifp >> chk_اندازه;
        ino_t chkIno;
        *ifp >> chkIno;
        إذا (ifp->eof()) اكسر;  // Needed to read final whitespace before found eof
        وخت_t chkCstime;
        *ifp >> chkCstime;
        وخت_t chkCnstime;
        *ifp >> chkCnstime;
        وخت_t chkMstime;
        *ifp >> chkMstime;
        وخت_t chkMnstime;
        *ifp >> chkMnstime;
        حرف quote;
        *ifp >> quote;
        ثابت string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ثابت string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        بنية stat curStat;
        ثابت صحيح err = stat(chkFilename.c_str(), &curStat);
        إذا (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            أرجع خطأ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        إذا (filename == chkFilename) استمر;

        UINFO(9, " got d=" << chkDir << " s=" << chk_اندازه << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_اندازه << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        إذا (!(curStat.st_اندازه >= chk_اندازه && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_اندازه << "=?" << chk_اندازه << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            إذا (skipHashing) أرجع خطأ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            إذا (chkHash == "unhashed") أرجع خطأ;

            VHashSha512 curHash;
            curHash.ورننباسه_File(chkFilename);
            إذا (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                أرجع خطأ;
            }
        }
    }
    أرجع ریښتیا;
}

//######################################################################
// V3File

فراغ V3File::جمع_کړه_Src_Depend(ثابت string& filename) VL_MT_خوندي { dependImp.جمع_کړه_Src_Depend(filename); }
فراغ V3File::جمع_کړه_Tgt_Depend(ثابت string& filename) VL_MT_خوندي { dependImp.جمع_کړه_Tgt_Depend(filename); }
فراغ V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::ویکټور<string> V3File::getAllDeps() { أرجع dependImp.getAllDeps(); }
فراغ V3File::writeTimes(ثابت string& filename, ثابت string& cmdline_دننه) {
    dependImp.writeTimes(filename, cmdline_دننه);
}
منطقي V3File::checkTimes(ثابت string& filename, ثابت string& cmdline_دننه) {
    أرجع dependImp.checkTimes(filename, cmdline_دننه);
}
فراغ V3File::createMakeDirFor(ثابت string& filename) {
    إذا (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().اوږدتیا() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
فراغ V3File::createMakeDir() {
    ساكن منطقي s_created = خطأ;
    إذا (!s_created) {
        s_created = ریښتیا;
        V3Os::createDir(v3Global.opt.makeDir());
        إذا (v3Global.opt.hier_سر()) V3Os::createDir(v3Global.opt.hier_سر_معلومات_Dir());
    }
}

//######################################################################
// VInFilterImp

صنف VInFilterImp final {
    کارول StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقي m_readEof = خطأ;  // Received EOF on read
#که_تعریف_شوی INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#وإلا
    صحيح m_pid = 0;  // fork() process id - always zero as disabled
#د_که_پای
    منطقي m_pidExited = خطأ;  //  waitpid() reaped the filter process
    صحيح m_pid_حالت = 0;  // Exit status of the filter process
    صحيح m_writeFd = 0;  // File descriptor TO filter
    صحيح m_readFd = 0;  // File descriptor FROM filter

خاص:
    // METHODS

    منطقي readContents(ثابت string& filename, StrList& outl) {
        إذا (m_pid) {
            أرجع readContentsFilter(filename, outl);
        } وإلا {
            أرجع readContentsFile(filename, outl);
        }
    }
    منطقي readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحيح fd = open(filename.c_str(), O_RDONLY);
        إذا (fd < 0) أرجع خطأ;
        m_readEof = خطأ;
        read_بلاکونه(fd, -1, outl);
        close(fd);
        أرجع ریښتیا;
    }
    // cppcheck-suppress constParameterReference
    منطقي readContentsFilter(ثابت string& filename, StrList& outl) {
        (فراغ)filename;  // Prevent unused variable warning
        (فراغ)outl;  // Prevent unused variable warning
#که_تعریف_شوی INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string کرښه = read_Filter_کرښه();
        إذا (کرښه.پیدا_کړه("Content-Length") != string::npos) {
            صحيح len = 0;
            له_مزي_ولوله(کرښه.c_str(), "Content-Length: %d\n", &len);
            read_بلاکونه(m_readFd, len, outl);
            أرجع ریښتیا;
        } وإلا {
            إذا (کرښه != "") v3error("--pipe-filter protocol error, unexpected: " << کرښه);
            أرجع خطأ;
        }
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
        أرجع خطأ;
#د_که_پای
    }

    // cppcheck-suppress functionConst
    فراغ checkFilter(منطقي hang) {
#که_تعریف_شوی INFILTER_PIPE
        إذا (!m_pidExited && waitpid(m_pid, &m_pid_حالت, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_حالت << " exit=" << WEXITSTATUS(m_pid_حالت) << " err"
                         << std::strerror(errno));
            m_readEof = ریښتیا;
            m_pidExited = ریښتیا;
        }
#د_که_پای
    }

    فراغ read_بلاکونه(صحيح fd, صحيح اندازه, StrList& outl) {
        حرف buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        طالما (!m_readEof && (اندازه < 0 || اندازه > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            إذا (اندازه > 0 && اندازه < todo) todo = اندازه;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                outl.emplace_شا(string(buf, got));
                sizegot += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#که_تعریف_شوی EWOULDBLOCK
                       || errno == EWOULDBLOCK
#د_که_پای
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                m_readEof = ریښتیا;
                اكسر;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_کرښه() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string کرښه;
        طالما (!m_readEof) {
            StrList outl;
            read_بلاکونه(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            کرښه += onechar;
            إذا (onechar == "\n") {
                إذا (کرښه == "\n") {
                    کرښه = "";
                    استمر;
                } وإلا {
                    اكسر;
                }
            }
        }
        UINFO(6, "filter-line-in: " << کرښه);
        أرجع کرښه;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    فراغ writeFilter(ثابت string& بهر) {
        إذا (debug() >= 6) {
            UINFO(6, "filter-out: " << بهر);
            إذا (بهر[بهر.اوږدتیا() - 1] != '\n') وتنه << '\n';
        }
        إذا (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = ریښتیا;
            stop();
        }
        بینښه دننه_واټن = 0;
        طالما (!m_readEof && بهر.اوږدتیا() > دننه_واټن) {
            errno = 0;
            ثابت صحيح got = اكتب(m_writeFd, (بهر.c_str()) + دننه_واټن, بهر.اوږدتیا() - دننه_واټن);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                دننه_واټن += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#که_تعریف_شوی EWOULDBLOCK
                       || errno == EWOULDBLOCK
#د_که_پای
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                اكسر;
            }
        }
    }

    // Start the filter
    فراغ پیل(ثابت string& command) {
        إذا (command == "") {
            m_pid = 0;  // Disabled
        } وإلا {
            پیل_Filter(command);
        }
    }
    فراغ پیل_Filter(ثابت string& command) {
        (فراغ)command;  // Prevent Unused variable warning
#که_تعریف_شوی INFILTER_PIPE
        صحيح fd_stdin[2];  // Can't use std::array
        صحيح fd_stdout[2];  // Can't use std::array
        constexpr صحيح P_RD = 0;
        constexpr صحيح P_WR = 1;

        إذا (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        إذا (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (pid == 0) {  //ماشوم
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<حرف*>(تش_اشاره));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } وإلا {  //پلار
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = خطأ;
            m_pid_حالت = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = خطأ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحيح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#د_که_پای
    }

    فراغ stop() {
        إذا (m_pid) stopFilter();
    }
    فراغ stopFilter() {
        UINFO(6, "Stopping filter process");
#که_تعریف_شوی INFILTER_PIPE
        close(m_writeFd);
        checkFilter(ریښتیا);
        إذا (!WIFEXITED(m_pid_حالت) || WEXITSTATUS(m_pid_حالت) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#د_که_پای
    }

محمي:
    ملګری صنف VInFilter;
    // Read file contents and return it
    منطقي readWholefile(ثابت string& filename, StrList& outl) {
        ثابت خپلکاری it = m_contentsMap.پیدا_کړه(filename);
        إذا (it != m_contentsMap.پای_ټکی()) {
            outl.ټېل_شا(it->دوهم);
            أرجع ریښتیا;
        }
        إذا (!readContents(filename, outl)) أرجع خطأ;
        إذا (list_اندازه(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        أرجع ریښتیا;
    }
    ساكن اندازه_t list_اندازه(ثابت StrList& sl) {
        اندازه_t پایله = 0;
        لكل (ثابت string& i : sl) پایله += i.اوږدتیا();
        أرجع پایله;
    }
    ساكن string listString(ثابت StrList& sl) {
        string پایله;
        لكل (ثابت string& i : sl) پایله += i;
        أرجع پایله;
    }
    // CONSTRUCTORS
    څرګند VInFilterImp(ثابت string& command) { پیل(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ثابت string& command) { m_impp = جديد VInFilterImp{command}; }
VInFilter::~VInFilter() {
    إذا (m_impp) VL_DO_CLEAR(ړنګول m_impp, m_impp = تش_اشاره);
}

منطقي VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    أرجع m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_بهر_Formatter::V3_بهر_Formatter(V3_بهر_Formatter::Language lang)
    : m_lang{lang} {
    m_بلاک_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_پلنوالی = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ساكن constexpr صحيح MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ساكن ثابت std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> جدول;
    لكل (صحيح i = 0; i <= MAXSPACE; ++i) جدول[i] = std::string(static_cast<اندازه_t>(i), ' ');
    أرجع جدول;
}();

ثابت std::string& V3_بهر_Formatter::indentSpaces(صحيح عدد) {
    // Indent the specified number of spaces.
    أرجع s_indentSpaces[std::max(0, std::min(عدد, MAXSPACE))];
}

منطقي V3_بهر_Formatter::token_سمون(ثابت حرف* cp, ثابت حرف* cmp) {
    طالما (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    إذا (*cmp) أرجع خطأ;
    إذا (*cp && !std::که_تشه(*cp)) أرجع خطأ;
    أرجع ریښتیا;
}

منطقي V3_بهر_Formatter::token_پیل(ثابت حرف* cp) {
    أرجع (token_سمون(cp, "begin") || token_سمون(cp, "case") || token_سمون(cp, "casex")
            || token_سمون(cp, "casez") || token_سمون(cp, "class") || token_سمون(cp, "function")
            || token_سمون(cp, "interface") || token_سمون(cp, "module") || token_سمون(cp, "package")
            || token_سمون(cp, "task"));
}

منطقي V3_بهر_Formatter::token_پای_ټکی(ثابت حرف* cp) {
    أرجع (token_سمون(cp, "end") || token_سمون(cp, "endcase") || token_سمون(cp, "endclass")
            || token_سمون(cp, "endfunction") || token_سمون(cp, "endinterface")
            || token_سمون(cp, "endmodule") || token_سمون(cp, "endpackage")
            || token_سمون(cp, "endtask"));
}

منطقي V3_بهر_Formatter::token_Not_پیل(ثابت حرف* cp) {
    أرجع (token_سمون(cp, "export") || token_سمون(cp, "import"));
}

صحيح V3_بهر_Formatter::پای_ټکی_پوړونه(ثابت حرف* strg) {
    صحيح پوړونه = m_indentLevel;
    {
        ثابت حرف* cp = strg;
        طالما (std::که_تشه(*cp)) ++cp;
        بدل (*cp) {
        حالة '\n':  // Newlines.. No need for whitespace before it
            أرجع 0;
        حالة '#':  // Preproc directive
            إذا (m_lang == LA_C) أرجع 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت حرف* mp = cp;
            لكل (; std::که_توری_یا_عدد(*mp); ++mp) {}
            إذا (mp[0] == ':' && mp[1] != ':') أرجع (پوړونه - m_بلاک_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        بدل (*cp) {
        حالة '}':
        حالة ')': پوړونه -= m_بلاک_Indent; اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') پوړونه -= m_بلاک_Indent;
            }
            اكسر;
        حالة 'e':
            إذا (m_lang == LA_VERILOG && token_پای_ټکی(cp)) پوړونه -= m_بلاک_Indent;
            اكسر;
        حالة '\t':
        حالة ' ': اكسر;  // Continue
        افتراضي: أرجع پوړونه;  // Letter
        }
    }
    أرجع پوړونه;
}

فراغ V3_بهر_Formatter::putns(ثابت Ast_غوټه* nodep, ثابت حرف* strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg);
        أرجع;
    }

    ثابت منطقي put_غوټه_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_سرچینه_وروستی_Filenameno != nodep->fileline()->filenameno()  //
            || m_سرچینه_وروستی_Lineno != nodep->fileline()->لومړی_Lineno())  //
        && File_کرښه::built_دننه_Filename() != nodep->fileline()->filename();

    إذا (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_غوټه_Decoration)) {
        putsNoTracking(indentSpaces(پای_ټکی_پوړونه(strg)));
        m_prependIndent = خطأ;
    }

    إذا (put_غوټه_Decoration) {
        ثابت File_کرښه* ثابت flp = nodep->fileline();
        m_سرچینه_وروستی_Lineno = flp->لومړی_Lineno();
        m_سرچینه_وروستی_Filenameno = flp->filenameno();
        ثابت std::string lineno = std::منزل_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_منزل_Hex(nodep) + "*/");
    }

    منطقي notstart = خطأ;
    منطقي wordstart = ریښتیا;
    منطقي equalsForBracket = خطأ;  // Looking for "= {"
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        إذا (std::که_توری(*cp)) {
            إذا (wordstart && m_lang == LA_VERILOG && token_Not_پیل(cp)) notstart = ریښتیا;
            إذا (wordstart && m_lang == LA_VERILOG && !notstart && token_پیل(cp)) indentInc();
            إذا (wordstart && m_lang == LA_VERILOG && token_پای_ټکی(cp)) indentDec();
        }
        ثابت منطقي was_دننه_Backslash = m_دننه_Backslash;
        m_دننه_Backslash = خطأ;
        بدل (*cp) {
        حالة '\n':
            ++m_lineno;
            wordstart = ریښتیا;
            إذا (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = ریښتیا;
            } وإلا {
                m_prependIndent = خطأ;
                putsNoTracking(indentSpaces(پای_ټکی_پوړونه(cp + 1)));
            }
            اكسر;
        حالة ' ': wordstart = ریښتیا; اكسر;
        حالة '\t': wordstart = ریښتیا; اكسر;
        حالة '"':
            wordstart = خطأ;
            إذا (!was_دننه_Backslash) m_دننه_String_Literal = !m_دننه_String_Literal;
            اكسر;
        حالة '\\':
            wordstart = خطأ;
            إذا (!was_دننه_Backslash) m_دننه_Backslash = ریښتیا;
            اكسر;
        حالة '/':
            إذا (m_lang == LA_C || m_lang == LA_VERILOG) {
                إذا (cp > strg && cp[-1] == '/' && !m_دننه_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    طالما (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    إذا (*cp) putcNoTracking(*cp);
                }
            }
            اكسر;
        حالة '{':
            إذا (!m_دننه_String_Literal) {
                إذا (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ټېل(m_indentLevel
                                    * m_بلاک_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            اكسر;
        حالة '}':
            إذا (!m_دننه_String_Literal) {
                إذا (m_bracketLevel > 0) {
                    m_parenVec.راوباسه();
                    --m_bracketLevel;
                }
                indentDec();
            }
            اكسر;
        حالة '(':
            إذا (!m_دننه_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ټېل(m_ستنه);
            }
            اكسر;
        حالة ')':
            إذا (!m_دننه_String_Literal) {
                إذا (!m_parenVec.تشه()) m_parenVec.راوباسه();
                indentDec();
            }
            اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } وإلا إذا (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } وإلا {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            اكسر;
        حالة '>':
            إذا (m_lang == LA_XML) {
                indentDec();
                إذا (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            اكسر;
        افتراضي: wordstart = خطأ; اكسر;
        }

        بدل (*cp) {
        حالة '=': equalsForBracket = ریښتیا; اكسر;
        حالة ' ': اكسر;
        افتراضي: equalsForBracket = خطأ; اكسر;
        }
    }
}

فراغ V3_بهر_Formatter::put_Break_بیان() {
    إذا (!m_parenVec.تشه()) putBreak();
}

// Add a line break if too wide
فراغ V3_بهر_Formatter::putBreak() {
    إذا (!v3Global.opt.decoration()) أرجع;
    إذا (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        إذا (exceeded_پلنوالی()) {
            putcNoTracking('\n');
            إذا (!m_parenVec.تشه()) putsNoTracking(indentSpaces(m_parenVec.سر()));
        }
    }
}

فراغ V3_بهر_Formatter::putsQuoted(ثابت string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ثابت string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    لكل (ثابت حرف c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    إذا (strg.پیدا_کړه('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
فراغ V3_بهر_Formatter::putsNoTracking(ثابت string& strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        أرجع;
    }
    // Don't track {}'s, probably because it's a $display format string
    لكل (ثابت حرف c : strg) putcNoTracking(c);
}

فراغ V3_بهر_Formatter::putcNoTracking(حرف chr) {
    إذا (!v3Global.opt.decoration()) {
        putcOutput(chr);
        أرجع;
    }
    بدل (chr) {
    حالة '\n':
        ++m_lineno;
        m_ستنه = 0;
        m_nobreak = ریښتیا;
        اكسر;
    حالة '\t': m_ستنه = ((m_ستنه + 9) / 8) * 8; اكسر;
    حالة ' ':
    حالة '(':
    حالة '|':
    حالة '&': ++m_ستنه; اكسر;
    افتراضي:
        ++m_ستنه;
        m_nobreak = خطأ;
        اكسر;
    }
    putcOutput(chr);
}

string V3_بهر_Formatter::quoteNameControls(ثابت string& namein,
                                         V3_بهر_Formatter::Language lang) VL_خالص {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string بهر;
    إذا (lang == LA_XML) {
        // Encode chars into XML string
        لكل (ثابت حرف c : namein) {
            إذا (c == '"') {
                بهر += "&quot;"s;
            } وإلا إذا (c == '\'') {
                بهر += "&apos;"s;
            } وإلا إذا (c == '<') {
                بهر += "&lt;"s;
            } وإلا إذا (c == '>') {
                بهر += "&gt;"s;
            } وإلا إذا (c == '&') {
                بهر += "&amp;"s;
            } وإلا إذا (std::isprint(c)) {
                بهر += c;
            } وإلا {
                بهر += "&#"s + cvt_منزل_Str(static_cast<بینښه صحيح>(c & 0xff)) + ";";
            }
        }
    } وإلا {
        // Encode control chars into C style escapes
        لكل (ثابت حرف c : namein) {
            إذا (c == '\\' || c == '"') {
                بهر += "\\"s + c;
            } وإلا إذا (c == '\n') {
                بهر += "\\n";
            } وإلا إذا (c == '\r') {
                بهر += "\\r";
            } وإلا إذا (c == '\t') {
                بهر += "\\t";
            } وإلا إذا (std::isprint(c)) {
                بهر += c;
            } وإلا {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvt_منزل_Str((c >> 6) & 3) + cvt_منزل_Str((c >> 3) & 7) + cvt_منزل_Str(c & 7);
                بهر += octal;
            }
        }
    }
    أرجع بهر;
}

//----------------------------------------------------------------------
// Simple wrappers

فراغ V3_بهر_Formatter::چاپ_کړه(ثابت حرف* fmt...) {
    constexpr اندازه_t bufsize = 5000;
    حرف sbuff[bufsize];
    va_list ap;
    va_پیل(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_پای_ټکی(ap);
    هذا->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_بهر_File::V3_بهر_File(ثابت string& filename, V3_بهر_Formatter::Language lang)
    : V3_بهر_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{جديد std::array<حرف, WRITE_بفر_اندازه_بایټونه>{}} {
    إذا ((m_fp = V3File::new_fopen_w(filename)) == تش_اشاره) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_بهر_File::~V3_بهر_File() {
    write_بلاک();

    إذا (m_fp) fclose(m_fp);
    m_fp = تش_اشاره;
}

فراغ V3_بهر_File::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    لكل (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

فراغ V3_بهر_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = ریښتیا;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    لكل (حرف& c : var) {
        إذا (!std::که_توری_یا_عدد(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_بهر_بهیر::V3_بهر_بهیر(std::ostream& ostream, V3_بهر_Formatter::Language lang)
    : V3_بهر_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

صنف VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ثابت std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_سېټ<std::string> m_new_Id_سېټ VL_GUARDED_BY(m_mutex);  // Which new names exist
محمي:
    // CONSTRUCTORS
    ملګری صنف VIdProtect;
    ساكن VIdProtectImp& singleton() VL_MT_خوندي {
        ساكن VIdProtectImp s;
        أرجع s;
    }

عام:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = افتراضي;
    // METHODS
    string passthru(ثابت string& old) VL_MT_خوندي_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds()) أرجع old;
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        ثابت خپلکاری it = m_nameMap.پیدا_کړه(old);
        إذا (it != m_nameMap.پای_ټکی()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->دوهم,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } وإلا {
            m_nameMap.emplace(old, old);
            m_new_Id_سېټ.ورننباسه(old);
        }
        أرجع old;
    }
    string protectIf(ثابت string& old, منطقي doIt) VL_MT_خوندي_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds() || old.تشه() || !doIt) أرجع old;
        ثابت V3_قلف_Guard قلف_2{m_mutex};
        ثابت خپلکاری pair = m_nameMap.emplace(old, "");
        إذا (pair.دوهم) {
            string بهر;
            إذا (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                بهر = "PS" + old;
            } وإلا {
                VHashSha512 digest{v3Global.opt.protect_کلی_Defaulted()};
                digest.ورننباسه(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                بهر = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                لكل (اندازه_t len = 6; len < بهر.اندازه() - 3; len += 3) {
                    ثابت string tryout = بهر.substr(0, len);
                    إذا (m_new_Id_سېټ.ورننباسه(tryout).دوهم) {
                        بهر = tryout;
                        اكسر;
                    }
                }
            }
            pair.لومړی->دوهم = بهر;
        }
        أرجع pair.لومړی->دوهم;
    }
    string protect_ټکي_If(ثابت string& old, منطقي doIt) VL_MT_خوندي {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        إذا (!(doIt && v3Global.opt.protectIds())) أرجع old;
        string بهر;
        string::اندازه_type پیل = 0;
        // space, ., ->
        طالما (ریښتیا) {
            // When C++11, use find_if and lambda
            string::اندازه_type pos = string::npos;
            string separator;
            trySep(old, پیل, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, پیل, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, پیل, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, پیل, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, پیل, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, پیل, ")", pos /*ref*/, separator /*ref*/);
            إذا (pos == string::npos) اكسر;
            بهر += protectIf(old.substr(پیل, pos - پیل), ریښتیا) + separator;
            پیل = pos + separator.اوږدتیا();
        }
        بهر += protectIf(old.substr(پیل), ریښتیا);
        أرجع بهر;
    }
    فراغ writeMapFile(ثابت string& filename) ثابت {
        V3_بهر_Xml_File د{filename};
        د.puts_سرلیک();
        د.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        د.puts("<verilator_id_map>\n");
        {
            لكل (ثابت خپلکاری& itr : m_nameMap) {
                د.puts("<map from=\"" + itr.دوهم + "\" to=\"" + itr.لومړی + "\"/>\n");
            }
        }
        د.puts("</verilator_id_map>\n");
    }

خاص:
    فراغ trySep(ثابت string& old, string::اندازه_type پیل, ثابت string& trySep,
                string::اندازه_type& posr, string& separatorr) VL_خالص {
        ثابت string::اندازه_type trypos = old.پیدا_کړه(trySep, پیل);
        إذا (trypos != string::npos) {
            إذا (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& دننه) VL_MT_خوندي {
    أرجع v3Global.opt.protectIds() ? "" : دننه;
}
string VIdProtect::protectIf(ثابت string& old, منطقي doIt) VL_MT_خوندي {
    أرجع VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_ټکي_If(ثابت string& old, منطقي doIt) VL_MT_خوندي {
    أرجع VIdProtectImp::singleton().protect_ټکي_If(old, doIt);
}
فراغ VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
