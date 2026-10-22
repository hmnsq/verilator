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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3File.h"

#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3String.h"

#అన్తర్భావ <cerrno>
#అన్తర్భావ <cstdarg>
#అన్తర్భావ <fcntl.h>
#అన్తర్భావ <iomanip>
#అన్తర్భావ <map>
#అన్తర్భావ <memory>

#అన్తర్భావ <sys/stat.h>
#అన్తర్భావ <sys/types.h>

// clang-format off
#ఒకవేళ defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#అయితే_ముగింపు

#నిర్వచితమైతే HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#లేకపోతే
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#అయితే_ముగింపు

#నిర్వచితమైతే INFILTER_PIPE
# include <sys/wait.h>
#అయితే_ముగింపు

#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#అయితే_ముగింపు
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr పూర్ణాంకం INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr పూర్ణాంకం INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_కొలత_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

తరగతి V3FileDependImp final {
    // TYPES
    తరగతి DependFile final {
        // A single file
        స్థిరరాశి తార్కిక m_లక్ష్యం;  // True if write, else read
        తార్కిక m_exists = నిజం;  // File exists
        స్థిరరాశి string m_filename;  // Filename
        నిర్మాణం stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    బహిరంగ:
        DependFile(స్థిరరాశి string& filename, తార్కిక లక్ష్యం)
            : m_లక్ష్యం{లక్ష్యం}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = అప్రమేయ;
        స్థిరరాశి string& filename() స్థిరరాశి { తిరిగి m_filename; }
        తార్కిక లక్ష్యం() స్థిరరాశి { తిరిగి m_లక్ష్యం; }
        తార్కిక exists() స్థిరరాశి { తిరిగి m_exists; }
        off_t కొలత() స్థిరరాశి { తిరిగి m_stat.st_కొలత; }
        ino_t ino() స్థిరరాశి { తిరిగి m_stat.st_ino; }
        సమయం_t cstime() స్థిరరాశి { తిరిగి m_stat.st_ctime; }  // Seconds
        సమయం_t cnstime() స్థిరరాశి { తిరిగి VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        సమయం_t mstime() స్థిరరాశి { తిరిగి m_stat.st_mtime; }  // Seconds
        సమయం_t mnstime() స్థిరరాశి { తిరిగి VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_చిహ్నం() {
            స్థిర VHashSha512 s_ఖాళీలు_Hash;
            తిరిగి m_hash.digestSymbol24() != s_ఖాళీలు_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        శూన్య ఎక్కించు_Stats() {
            ఒకవేళ (!m_stat.st_mtime) {
                స్థిరరాశి string fn = filename();
                స్థిరరాశి పూర్ణాంకం err = stat(fn.c_str(), &m_stat);
                ఒకవేళ (err != 0) {
                    std::memset(&m_stat, 0, పరిమాణం(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = అబద్ధం;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } లేకపోతే {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    ఒకవేళ (!లక్ష్యం() && కొలత() <= FILE_HASH_కొలత_MAX) m_hash.చొప్పించు_File(filename());
                }
            }
        }
        తార్కిక క్రియాకారకం<(స్థిరరాశి DependFile& rhs) స్థిరరాశి { తిరిగి filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::సమితి<string> m_filename_సమితి VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::సమితి<DependFile> m_filenameList;  // Files sourced/generated

    స్థిర string stripQuotes(స్థిరరాశి string& లోపల) {
        string pretty = లోపల;
        string::కొలత_type pos;
        అయితే ((pos = pretty.కనుగొను('\"')) != string::npos) pretty.replace(pos, 1, "_");
        అయితే ((pos = pretty.కనుగొను('\n')) != string::npos) pretty.replace(pos, 1, "_");
        తిరిగి pretty;
    }

బహిరంగ:
    // ACCESSOR METHODS
    శూన్య కూడు_Src_Depend(స్థిరరాశి string& filename) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        స్థిరరాశి స్వయంచాలక it_దొరికింది_Pair = m_filename_సమితి.చొప్పించు(filename);
        ఒకవేళ (it_దొరికింది_Pair.రెండవ) {
            DependFile df{filename, అబద్ధం};
            df.ఎక్కించు_Stats();  // Get size now, in case changes during the run
            m_filenameList.చొప్పించు(df);
        }
    }
    శూన్య కూడు_Tgt_Depend(స్థిరరాశి string& filename) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        స్థిరరాశి స్వయంచాలక it_దొరికింది_Pair = m_filename_సమితి.చొప్పించు(filename);
        ఒకవేళ (it_దొరికింది_Pair.రెండవ) m_filenameList.చొప్పించు(DependFile{filename, నిజం});
    }
    శూన్య writeDepend(స్థిరరాశి string& filename);
    std::సదిశ<string> getAllDeps() స్థిరరాశి;
    శూన్య writeTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల);
    తార్కిక checkTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

శూన్య V3FileDependImp::writeDepend(స్థిరరాశి string& filename) {
    స్థిరరాశి std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ఒకవేళ (ofp->fail()) v3fatal("Can't write file: " << filename);

    కొరకు (స్థిరరాశి DependFile& i : m_filenameList) {
        ఒకవేళ (i.లక్ష్యం()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    కొరకు (స్థిరరాశి DependFile& i : m_filenameList) {
        ఒకవేళ (!i.లక్ష్యం()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    ఒకవేళ (v3Global.opt.makePhony()) {
        *ofp << '\n';
        కొరకు (స్థిరరాశి DependFile& i : m_filenameList) {
            ఒకవేళ (!i.లక్ష్యం()) *ofp << i.filename() << ":\n";
        }
    }
}

std::సదిశ<string> V3FileDependImp::getAllDeps() స్థిరరాశి {
    std::సదిశ<string> r;
    కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_filenameList) {
        ఒకవేళ (!itr.లక్ష్యం() && itr.exists()) r.నెట్టు_వెనుక(itr.filename());
    }
    తిరిగి r;
}

శూన్య V3FileDependImp::writeTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల) {
    స్థిరరాశి std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ఒకవేళ (ofp->fail()) v3fatal("Can't write file: " << filename);

    స్థిరరాశి string cmdline = stripQuotes(cmdline_లోపల);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    కొరకు (స్వయంచాలక& it : m_filenameList) {
        DependFile* స్థిరరాశి depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->ఎక్కించు_Stats();
        off_t show_కొలత = depfilep->కొలత();
        ino_t showIno = depfilep->ino();
        ఒకవేళ (depfilep->filename() == filename) {
            show_కొలత = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->లక్ష్యం() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_కొలత;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_చిహ్నం() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

తార్కిక V3FileDependImp::checkTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల) {
    స్థిరరాశి std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    ఒకవేళ (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        తిరిగి అబద్ధం;
    }
    {
        స్థిరరాశి string ignore = V3Os::getline(*ifp);
        ఒకవేళ (ignore.ఖాళీలు()) { /*వాడిన*/
        }
    }
    {
        అక్షరం chkDir;
        *ifp >> chkDir;
        అక్షరం quote;
        *ifp >> quote;
        స్థిరరాశి string chkCmdline = V3Os::getline(*ifp, '"');
        స్థిరరాశి string cmdline = stripQuotes(cmdline_లోపల);
        ఒకవేళ (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            తిరిగి అబద్ధం;
        }
    }

    స్థిరరాశి తార్కిక skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").ఖాళీలు();

    అయితే (!ifp->eof()) {
        అక్షరం chkDir;
        *ifp >> chkDir;
        off_t chk_కొలత;
        *ifp >> chk_కొలత;
        ino_t chkIno;
        *ifp >> chkIno;
        ఒకవేళ (ifp->eof()) ఆపు;  // Needed to read final whitespace before found eof
        సమయం_t chkCstime;
        *ifp >> chkCstime;
        సమయం_t chkCnstime;
        *ifp >> chkCnstime;
        సమయం_t chkMstime;
        *ifp >> chkMstime;
        సమయం_t chkMnstime;
        *ifp >> chkMnstime;
        అక్షరం quote;
        *ifp >> quote;
        స్థిరరాశి string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        స్థిరరాశి string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        నిర్మాణం stat curStat;
        స్థిరరాశి పూర్ణాంకం err = stat(chkFilename.c_str(), &curStat);
        ఒకవేళ (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            తిరిగి అబద్ధం;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        ఒకవేళ (filename == chkFilename) కొనసాగు;

        UINFO(9, " got d=" << chkDir << " s=" << chk_కొలత << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_కొలత << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        ఒకవేళ (!(curStat.st_కొలత >= chk_కొలత && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_కొలత << "=?" << chk_కొలత << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            ఒకవేళ (skipHashing) తిరిగి అబద్ధం;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            ఒకవేళ (chkHash == "unhashed") తిరిగి అబద్ధం;

            VHashSha512 curHash;
            curHash.చొప్పించు_File(chkFilename);
            ఒకవేళ (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                తిరిగి అబద్ధం;
            }
        }
    }
    తిరిగి నిజం;
}

//######################################################################
// V3File

శూన్య V3File::కూడు_Src_Depend(స్థిరరాశి string& filename) VL_MT_సురక్షిత { dependImp.కూడు_Src_Depend(filename); }
శూన్య V3File::కూడు_Tgt_Depend(స్థిరరాశి string& filename) VL_MT_సురక్షిత { dependImp.కూడు_Tgt_Depend(filename); }
శూన్య V3File::writeDepend(స్థిరరాశి string& filename) { dependImp.writeDepend(filename); }
std::సదిశ<string> V3File::getAllDeps() { తిరిగి dependImp.getAllDeps(); }
శూన్య V3File::writeTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల) {
    dependImp.writeTimes(filename, cmdline_లోపల);
}
తార్కిక V3File::checkTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల) {
    తిరిగి dependImp.checkTimes(filename, cmdline_లోపల);
}
శూన్య V3File::createMakeDirFor(స్థిరరాశి string& filename) {
    ఒకవేళ (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().నిడివి() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
శూన్య V3File::createMakeDir() {
    స్థిర తార్కిక s_created = అబద్ధం;
    ఒకవేళ (!s_created) {
        s_created = నిజం;
        V3Os::createDir(v3Global.opt.makeDir());
        ఒకవేళ (v3Global.opt.hier_శిఖరం()) V3Os::createDir(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir());
    }
}

//######################################################################
// VInFilterImp

తరగతి VInFilterImp final {
    ఉపయోగించి StrList = VInFilter::StrList;

    std::map<స్థిరరాశి std::string, std::string> m_contentsMap;  // Cache of file contents
    తార్కిక m_readEof = అబద్ధం;  // Received EOF on read
#నిర్వచితమైతే INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#లేకపోతే
    పూర్ణాంకం m_pid = 0;  // fork() process id - always zero as disabled
#అయితే_ముగింపు
    తార్కిక m_pidExited = అబద్ధం;  //  waitpid() reaped the filter process
    పూర్ణాంకం m_pid_స్థితి = 0;  // Exit status of the filter process
    పూర్ణాంకం m_writeFd = 0;  // File descriptor TO filter
    పూర్ణాంకం m_readFd = 0;  // File descriptor FROM filter

అంతరంగ:
    // METHODS

    తార్కిక readContents(స్థిరరాశి string& filename, StrList& outl) {
        ఒకవేళ (m_pid) {
            తిరిగి readContentsFilter(filename, outl);
        } లేకపోతే {
            తిరిగి readContentsFile(filename, outl);
        }
    }
    తార్కిక readContentsFile(స్థిరరాశి string& filename, StrList& outl) {
        స్థిరరాశి పూర్ణాంకం fd = open(filename.c_str(), O_RDONLY);
        ఒకవేళ (fd < 0) తిరిగి అబద్ధం;
        m_readEof = అబద్ధం;
        read_ఖండాలు(fd, -1, outl);
        close(fd);
        తిరిగి నిజం;
    }
    // cppcheck-suppress constParameterReference
    తార్కిక readContentsFilter(స్థిరరాశి string& filename, StrList& outl) {
        (శూన్య)filename;  // Prevent unused variable warning
        (శూన్య)outl;  // Prevent unused variable warning
#నిర్వచితమైతే INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        స్థిరరాశి string పంక్తి = read_Filter_పంక్తి();
        ఒకవేళ (పంక్తి.కనుగొను("Content-Length") != string::npos) {
            పూర్ణాంకం len = 0;
            అక్షరశ్రేణి_చదువు(పంక్తి.c_str(), "Content-Length: %d\n", &len);
            read_ఖండాలు(m_readFd, len, outl);
            తిరిగి నిజం;
        } లేకపోతే {
            ఒకవేళ (పంక్తి != "") v3error("--pipe-filter protocol error, unexpected: " << పంక్తి);
            తిరిగి అబద్ధం;
        }
#లేకపోతే
        v3fatalSrc("--pipe-filter not implemented on this platform");
        తిరిగి అబద్ధం;
#అయితే_ముగింపు
    }

    // cppcheck-suppress functionConst
    శూన్య checkFilter(తార్కిక hang) {
#నిర్వచితమైతే INFILTER_PIPE
        ఒకవేళ (!m_pidExited && waitpid(m_pid, &m_pid_స్థితి, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_స్థితి << " exit=" << WEXITSTATUS(m_pid_స్థితి) << " err"
                         << std::strerror(errno));
            m_readEof = నిజం;
            m_pidExited = నిజం;
        }
#అయితే_ముగింపు
    }

    శూన్య read_ఖండాలు(పూర్ణాంకం fd, పూర్ణాంకం కొలత, StrList& outl) {
        అక్షరం buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        అయితే (!m_readEof && (కొలత < 0 || కొలత > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            ఒకవేళ (కొలత > 0 && కొలత < todo) todo = కొలత;
            errno = 0;
            స్థిరరాశి ssize_t got = పఠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ఒకవేళ (got > 0) {
                outl.emplace_వెనుక(string(buf, got));
                sizegot += got;
            } లేకపోతే ఒకవేళ (errno == EINTR || errno == EAGAIN
#నిర్వచితమైతే EWOULDBLOCK
                       || errno == EWOULDBLOCK
#అయితే_ముగింపు
            ) {
                checkFilter(అబద్ధం);
                V3Os::u_sleep(1000);
                కొనసాగు;
            } లేకపోతే {
                m_readEof = నిజం;
                ఆపు;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_పంక్తి() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string పంక్తి;
        అయితే (!m_readEof) {
            StrList outl;
            read_ఖండాలు(m_readFd, 1, outl);
            స్థిరరాశి string onechar = listString(outl);
            పంక్తి += onechar;
            ఒకవేళ (onechar == "\n") {
                ఒకవేళ (పంక్తి == "\n") {
                    పంక్తి = "";
                    కొనసాగు;
                } లేకపోతే {
                    ఆపు;
                }
            }
        }
        UINFO(6, "filter-line-in: " << పంక్తి);
        తిరిగి పంక్తి;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    శూన్య writeFilter(స్థిరరాశి string& బహిర్గతం) {
        ఒకవేళ (debug() >= 6) {
            UINFO(6, "filter-out: " << బహిర్గతం);
            ఒకవేళ (బహిర్గతం[బహిర్గతం.నిడివి() - 1] != '\n') బయటకు << '\n';
        }
        ఒకవేళ (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = నిజం;
            stop();
        }
        గుర్తులేని స్థానభ్రంశం = 0;
        అయితే (!m_readEof && బహిర్గతం.నిడివి() > స్థానభ్రంశం) {
            errno = 0;
            స్థిరరాశి పూర్ణాంకం got = లిఖ(m_writeFd, (బహిర్గతం.c_str()) + స్థానభ్రంశం, బహిర్గతం.నిడివి() - స్థానభ్రంశం);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ఒకవేళ (got > 0) {
                స్థానభ్రంశం += got;
            } లేకపోతే ఒకవేళ (errno == EINTR || errno == EAGAIN
#నిర్వచితమైతే EWOULDBLOCK
                       || errno == EWOULDBLOCK
#అయితే_ముగింపు
            ) {
                checkFilter(అబద్ధం);
                V3Os::u_sleep(1000);
                కొనసాగు;
            } లేకపోతే {
                ఆపు;
            }
        }
    }

    // Start the filter
    శూన్య ప్రారంభం(స్థిరరాశి string& command) {
        ఒకవేళ (command == "") {
            m_pid = 0;  // Disabled
        } లేకపోతే {
            ప్రారంభం_Filter(command);
        }
    }
    శూన్య ప్రారంభం_Filter(స్థిరరాశి string& command) {
        (శూన్య)command;  // Prevent Unused variable warning
#నిర్వచితమైతే INFILTER_PIPE
        పూర్ణాంకం fd_stdin[2];  // Can't use std::array
        పూర్ణాంకం fd_stdout[2];  // Can't use std::array
        constexpr పూర్ణాంకం P_RD = 0;
        constexpr పూర్ణాంకం P_WR = 1;

        ఒకవేళ (VL_UNCOVERABLE(ప్రణాల(fd_stdin) != 0 || ప్రణాల(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ఒకవేళ (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        స్థిరరాశి pid_t pid = fork();
        ఒకవేళ (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ఒకవేళ (pid == 0) {  //బిడ్డ
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<అక్షరం*>(శూన్యసూచిక));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } లేకపోతే {  //మాతృ
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = అబద్ధం;
            m_pid_స్థితి = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = అబద్ధం;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            పూర్ణాంకం flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#లేకపోతే
        v3fatalSrc("--pipe-filter not implemented on this platform");
#అయితే_ముగింపు
    }

    శూన్య stop() {
        ఒకవేళ (m_pid) stopFilter();
    }
    శూన్య stopFilter() {
        UINFO(6, "Stopping filter process");
#నిర్వచితమైతే INFILTER_PIPE
        close(m_writeFd);
        checkFilter(నిజం);
        ఒకవేళ (!WIFEXITED(m_pid_స్థితి) || WEXITSTATUS(m_pid_స్థితి) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#లేకపోతే
        v3fatalSrc("--pipe-filter not implemented on this platform");
#అయితే_ముగింపు
    }

రక్షిత:
    మిత్ర తరగతి VInFilter;
    // Read file contents and return it
    తార్కిక readWholefile(స్థిరరాశి string& filename, StrList& outl) {
        స్థిరరాశి స్వయంచాలక it = m_contentsMap.కనుగొను(filename);
        ఒకవేళ (it != m_contentsMap.చివర()) {
            outl.నెట్టు_వెనుక(it->రెండవ);
            తిరిగి నిజం;
        }
        ఒకవేళ (!readContents(filename, outl)) తిరిగి అబద్ధం;
        ఒకవేళ (list_కొలత(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        తిరిగి నిజం;
    }
    స్థిర కొలత_t list_కొలత(స్థిరరాశి StrList& sl) {
        కొలత_t ఫలితం = 0;
        కొరకు (స్థిరరాశి string& i : sl) ఫలితం += i.నిడివి();
        తిరిగి ఫలితం;
    }
    స్థిర string listString(స్థిరరాశి StrList& sl) {
        string ఫలితం;
        కొరకు (స్థిరరాశి string& i : sl) ఫలితం += i;
        తిరిగి ఫలితం;
    }
    // CONSTRUCTORS
    స్పష్టమైన VInFilterImp(స్థిరరాశి string& command) { ప్రారంభం(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(స్థిరరాశి string& command) { m_impp = కొత్త VInFilterImp{command}; }
VInFilter::~VInFilter() {
    ఒకవేళ (m_impp) VL_DO_CLEAR(తొలగించు m_impp, m_impp = శూన్యసూచిక);
}

తార్కిక VInFilter::readWholefile(స్థిరరాశి string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    తిరిగి m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_బహిర్గతం_Formatter::V3_బహిర్గతం_Formatter(V3_బహిర్గతం_Formatter::Language lang)
    : m_lang{lang} {
    m_ఖండం_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_వెడల్పు = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

స్థిర constexpr పూర్ణాంకం MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
స్థిర స్థిరరాశి std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> పట్టిక;
    కొరకు (పూర్ణాంకం i = 0; i <= MAXSPACE; ++i) పట్టిక[i] = std::string(static_cast<కొలత_t>(i), ' ');
    తిరిగి పట్టిక;
}();

స్థిరరాశి std::string& V3_బహిర్గతం_Formatter::indentSpaces(పూర్ణాంకం అంకె) {
    // Indent the specified number of spaces.
    తిరిగి s_indentSpaces[std::max(0, std::min(అంకె, MAXSPACE))];
}

తార్కిక V3_బహిర్గతం_Formatter::token_సరిపోలిక(స్థిరరాశి అక్షరం* cp, స్థిరరాశి అక్షరం* cmp) {
    అయితే (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    ఒకవేళ (*cmp) తిరిగి అబద్ధం;
    ఒకవేళ (*cp && !std::ఖాళీనా(*cp)) తిరిగి అబద్ధం;
    తిరిగి నిజం;
}

తార్కిక V3_బహిర్గతం_Formatter::token_ప్రారంభం(స్థిరరాశి అక్షరం* cp) {
    తిరిగి (token_సరిపోలిక(cp, "begin") || token_సరిపోలిక(cp, "case") || token_సరిపోలిక(cp, "casex")
            || token_సరిపోలిక(cp, "casez") || token_సరిపోలిక(cp, "class") || token_సరిపోలిక(cp, "function")
            || token_సరిపోలిక(cp, "interface") || token_సరిపోలిక(cp, "module") || token_సరిపోలిక(cp, "package")
            || token_సరిపోలిక(cp, "task"));
}

తార్కిక V3_బహిర్గతం_Formatter::token_చివర(స్థిరరాశి అక్షరం* cp) {
    తిరిగి (token_సరిపోలిక(cp, "end") || token_సరిపోలిక(cp, "endcase") || token_సరిపోలిక(cp, "endclass")
            || token_సరిపోలిక(cp, "endfunction") || token_సరిపోలిక(cp, "endinterface")
            || token_సరిపోలిక(cp, "endmodule") || token_సరిపోలిక(cp, "endpackage")
            || token_సరిపోలిక(cp, "endtask"));
}

తార్కిక V3_బహిర్గతం_Formatter::token_Not_ప్రారంభం(స్థిరరాశి అక్షరం* cp) {
    తిరిగి (token_సరిపోలిక(cp, "export") || token_సరిపోలిక(cp, "import"));
}

పూర్ణాంకం V3_బహిర్గతం_Formatter::చివర_స్థాయిలు(స్థిరరాశి అక్షరం* strg) {
    పూర్ణాంకం స్థాయిలు = m_indentLevel;
    {
        స్థిరరాశి అక్షరం* cp = strg;
        అయితే (std::ఖాళీనా(*cp)) ++cp;
        ఎంపిక (*cp) {
        స్థితి '\n':  // Newlines.. No need for whitespace before it
            తిరిగి 0;
        స్థితి '#':  // Preproc directive
            ఒకవేళ (m_lang == LA_C) తిరిగి 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            స్థిరరాశి అక్షరం* mp = cp;
            కొరకు (; std::అక్షరమా_అంకెనా(*mp); ++mp) {}
            ఒకవేళ (mp[0] == ':' && mp[1] != ':') తిరిగి (స్థాయిలు - m_ఖండం_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    కొరకు (స్థిరరాశి అక్షరం* cp = strg; *cp; ++cp) {
        ఎంపిక (*cp) {
        స్థితి '}':
        స్థితి ')': స్థాయిలు -= m_ఖండం_Indent; ఆపు;
        స్థితి '<':
            ఒకవేళ (m_lang == LA_XML) {
                ఒకవేళ (cp[1] == '/') స్థాయిలు -= m_ఖండం_Indent;
            }
            ఆపు;
        స్థితి 'e':
            ఒకవేళ (m_lang == LA_VERILOG && token_చివర(cp)) స్థాయిలు -= m_ఖండం_Indent;
            ఆపు;
        స్థితి '\t':
        స్థితి ' ': ఆపు;  // Continue
        అప్రమేయ: తిరిగి స్థాయిలు;  // Letter
        }
    }
    తిరిగి స్థాయిలు;
}

శూన్య V3_బహిర్గతం_Formatter::putns(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* strg) {
    ఒకవేళ (!v3Global.opt.decoration()) {
        putsOutput(strg);
        తిరిగి;
    }

    స్థిరరాశి తార్కిక put_కణుపు_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_ఆధారం_చివరి_Filenameno != nodep->fileline()->filenameno()  //
            || m_ఆధారం_చివరి_Lineno != nodep->fileline()->మొదటి_Lineno())  //
        && File_పంక్తి::built_లోపల_Filename() != nodep->fileline()->filename();

    ఒకవేళ (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_కణుపు_Decoration)) {
        putsNoTracking(indentSpaces(చివర_స్థాయిలు(strg)));
        m_prependIndent = అబద్ధం;
    }

    ఒకవేళ (put_కణుపు_Decoration) {
        స్థిరరాశి File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        m_ఆధారం_చివరి_Lineno = flp->మొదటి_Lineno();
        m_ఆధారం_చివరి_Filenameno = flp->filenameno();
        స్థిరరాశి std::string lineno = std::గమ్యం_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_గమ్యం_Hex(nodep) + "*/");
    }

    తార్కిక notstart = అబద్ధం;
    తార్కిక wordstart = నిజం;
    తార్కిక equalsForBracket = అబద్ధం;  // Looking for "= {"
    కొరకు (స్థిరరాశి అక్షరం* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        ఒకవేళ (std::అక్షరమా(*cp)) {
            ఒకవేళ (wordstart && m_lang == LA_VERILOG && token_Not_ప్రారంభం(cp)) notstart = నిజం;
            ఒకవేళ (wordstart && m_lang == LA_VERILOG && !notstart && token_ప్రారంభం(cp)) indentInc();
            ఒకవేళ (wordstart && m_lang == LA_VERILOG && token_చివర(cp)) indentDec();
        }
        స్థిరరాశి తార్కిక was_లోపల_Backslash = m_లోపల_Backslash;
        m_లోపల_Backslash = అబద్ధం;
        ఎంపిక (*cp) {
        స్థితి '\n':
            ++m_lineno;
            wordstart = నిజం;
            ఒకవేళ (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = నిజం;
            } లేకపోతే {
                m_prependIndent = అబద్ధం;
                putsNoTracking(indentSpaces(చివర_స్థాయిలు(cp + 1)));
            }
            ఆపు;
        స్థితి ' ': wordstart = నిజం; ఆపు;
        స్థితి '\t': wordstart = నిజం; ఆపు;
        స్థితి '"':
            wordstart = అబద్ధం;
            ఒకవేళ (!was_లోపల_Backslash) m_లోపల_String_Literal = !m_లోపల_String_Literal;
            ఆపు;
        స్థితి '\\':
            wordstart = అబద్ధం;
            ఒకవేళ (!was_లోపల_Backslash) m_లోపల_Backslash = నిజం;
            ఆపు;
        స్థితి '/':
            ఒకవేళ (m_lang == LA_C || m_lang == LA_VERILOG) {
                ఒకవేళ (cp > strg && cp[-1] == '/' && !m_లోపల_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    అయితే (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    ఒకవేళ (*cp) putcNoTracking(*cp);
                }
            }
            ఆపు;
        స్థితి '{':
            ఒకవేళ (!m_లోపల_String_Literal) {
                ఒకవేళ (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.నెట్టు(m_indentLevel
                                    * m_ఖండం_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            ఆపు;
        స్థితి '}':
            ఒకవేళ (!m_లోపల_String_Literal) {
                ఒకవేళ (m_bracketLevel > 0) {
                    m_parenVec.తీయి();
                    --m_bracketLevel;
                }
                indentDec();
            }
            ఆపు;
        స్థితి '(':
            ఒకవేళ (!m_లోపల_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.నెట్టు(m_స్తంభం);
            }
            ఆపు;
        స్థితి ')':
            ఒకవేళ (!m_లోపల_String_Literal) {
                ఒకవేళ (!m_parenVec.ఖాళీలు()) m_parenVec.తీయి();
                indentDec();
            }
            ఆపు;
        స్థితి '<':
            ఒకవేళ (m_lang == LA_XML) {
                ఒకవేళ (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } లేకపోతే ఒకవేళ (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } లేకపోతే {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            ఆపు;
        స్థితి '>':
            ఒకవేళ (m_lang == LA_XML) {
                indentDec();
                ఒకవేళ (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            ఆపు;
        అప్రమేయ: wordstart = అబద్ధం; ఆపు;
        }

        ఎంపిక (*cp) {
        స్థితి '=': equalsForBracket = నిజం; ఆపు;
        స్థితి ' ': ఆపు;
        అప్రమేయ: equalsForBracket = అబద్ధం; ఆపు;
        }
    }
}

శూన్య V3_బహిర్గతం_Formatter::put_Break_సమాసం() {
    ఒకవేళ (!m_parenVec.ఖాళీలు()) putBreak();
}

// Add a line break if too wide
శూన్య V3_బహిర్గతం_Formatter::putBreak() {
    ఒకవేళ (!v3Global.opt.decoration()) తిరిగి;
    ఒకవేళ (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        ఒకవేళ (exceeded_వెడల్పు()) {
            putcNoTracking('\n');
            ఒకవేళ (!m_parenVec.ఖాళీలు()) putsNoTracking(indentSpaces(m_parenVec.శిఖరం()));
        }
    }
}

శూన్య V3_బహిర్గతం_Formatter::putsQuoted(స్థిరరాశి string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    స్థిరరాశి string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    కొరకు (స్థిరరాశి అక్షరం c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    ఒకవేళ (strg.కనుగొను('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
శూన్య V3_బహిర్గతం_Formatter::putsNoTracking(స్థిరరాశి string& strg) {
    ఒకవేళ (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        తిరిగి;
    }
    // Don't track {}'s, probably because it's a $display format string
    కొరకు (స్థిరరాశి అక్షరం c : strg) putcNoTracking(c);
}

శూన్య V3_బహిర్గతం_Formatter::putcNoTracking(అక్షరం chr) {
    ఒకవేళ (!v3Global.opt.decoration()) {
        putcOutput(chr);
        తిరిగి;
    }
    ఎంపిక (chr) {
    స్థితి '\n':
        ++m_lineno;
        m_స్తంభం = 0;
        m_nobreak = నిజం;
        ఆపు;
    స్థితి '\t': m_స్తంభం = ((m_స్తంభం + 9) / 8) * 8; ఆపు;
    స్థితి ' ':
    స్థితి '(':
    స్థితి '|':
    స్థితి '&': ++m_స్తంభం; ఆపు;
    అప్రమేయ:
        ++m_స్తంభం;
        m_nobreak = అబద్ధం;
        ఆపు;
    }
    putcOutput(chr);
}

string V3_బహిర్గతం_Formatter::quoteNameControls(స్థిరరాశి string& namein,
                                         V3_బహిర్గతం_Formatter::Language lang) VL_శుద్ధ {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string బహిర్గతం;
    ఒకవేళ (lang == LA_XML) {
        // Encode chars into XML string
        కొరకు (స్థిరరాశి అక్షరం c : namein) {
            ఒకవేళ (c == '"') {
                బహిర్గతం += "&quot;"s;
            } లేకపోతే ఒకవేళ (c == '\'') {
                బహిర్గతం += "&apos;"s;
            } లేకపోతే ఒకవేళ (c == '<') {
                బహిర్గతం += "&lt;"s;
            } లేకపోతే ఒకవేళ (c == '>') {
                బహిర్గతం += "&gt;"s;
            } లేకపోతే ఒకవేళ (c == '&') {
                బహిర్గతం += "&amp;"s;
            } లేకపోతే ఒకవేళ (std::isprint(c)) {
                బహిర్గతం += c;
            } లేకపోతే {
                బహిర్గతం += "&#"s + cvt_గమ్యం_Str(static_cast<గుర్తులేని పూర్ణాంకం>(c & 0xff)) + ";";
            }
        }
    } లేకపోతే {
        // Encode control chars into C style escapes
        కొరకు (స్థిరరాశి అక్షరం c : namein) {
            ఒకవేళ (c == '\\' || c == '"') {
                బహిర్గతం += "\\"s + c;
            } లేకపోతే ఒకవేళ (c == '\n') {
                బహిర్గతం += "\\n";
            } లేకపోతే ఒకవేళ (c == '\r') {
                బహిర్గతం += "\\r";
            } లేకపోతే ఒకవేళ (c == '\t') {
                బహిర్గతం += "\\t";
            } లేకపోతే ఒకవేళ (std::isprint(c)) {
                బహిర్గతం += c;
            } లేకపోతే {
                // This will also cover \a etc
                స్థిరరాశి string octal
                    = "\\"s + cvt_గమ్యం_Str((c >> 6) & 3) + cvt_గమ్యం_Str((c >> 3) & 7) + cvt_గమ్యం_Str(c & 7);
                బహిర్గతం += octal;
            }
        }
    }
    తిరిగి బహిర్గతం;
}

//----------------------------------------------------------------------
// Simple wrappers

శూన్య V3_బహిర్గతం_Formatter::రాయి(స్థిరరాశి అక్షరం* fmt...) {
    constexpr కొలత_t bufsize = 5000;
    అక్షరం sbuff[bufsize];
    va_list ap;
    va_ప్రారంభం(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_చివర(ap);
    ఇది->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_బహిర్గతం_File::V3_బహిర్గతం_File(స్థిరరాశి string& filename, V3_బహిర్గతం_Formatter::Language lang)
    : V3_బహిర్గతం_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{కొత్త std::array<అక్షరం, WRITE_బఫరు_కొలత_బైట్లు>{}} {
    ఒకవేళ ((m_fp = V3File::new_fopen_w(filename)) == శూన్యసూచిక) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_బహిర్గతం_File::~V3_బహిర్గతం_File() {
    write_ఖండం();

    ఒకవేళ (m_fp) fclose(m_fp);
    m_fp = శూన్యసూచిక;
}

శూన్య V3_బహిర్గతం_File::putsForceIncs() {
    స్థిరరాశి VStringList& forceIncs = v3Global.opt.forceIncs();
    కొరకు (స్థిరరాశి string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

శూన్య V3_బహిర్గతం_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = నిజం;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    కొరకు (అక్షరం& c : var) {
        ఒకవేళ (!std::అక్షరమా_అంకెనా(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_బహిర్గతం_ప్రవాహం::V3_బహిర్గతం_ప్రవాహం(std::ostream& ostream, V3_బహిర్గతం_Formatter::Language lang)
    : V3_బహిర్గతం_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

తరగతి VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<స్థిరరాశి std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_సమితి<std::string> m_new_Id_సమితి VL_GUARDED_BY(m_mutex);  // Which new names exist
రక్షిత:
    // CONSTRUCTORS
    మిత్ర తరగతి VIdProtect;
    స్థిర VIdProtectImp& singleton() VL_MT_సురక్షిత {
        స్థిర VIdProtectImp s;
        తిరిగి s;
    }

బహిరంగ:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = అప్రమేయ;
    // METHODS
    string passthru(స్థిరరాశి string& old) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        ఒకవేళ (!v3Global.opt.protectIds()) తిరిగి old;
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        స్థిరరాశి స్వయంచాలక it = m_nameMap.కనుగొను(old);
        ఒకవేళ (it != m_nameMap.చివర()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->రెండవ,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } లేకపోతే {
            m_nameMap.emplace(old, old);
            m_new_Id_సమితి.చొప్పించు(old);
        }
        తిరిగి old;
    }
    string protectIf(స్థిరరాశి string& old, తార్కిక doIt) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        ఒకవేళ (!v3Global.opt.protectIds() || old.ఖాళీలు() || !doIt) తిరిగి old;
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        స్థిరరాశి స్వయంచాలక pair = m_nameMap.emplace(old, "");
        ఒకవేళ (pair.రెండవ) {
            string బహిర్గతం;
            ఒకవేళ (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                బహిర్గతం = "PS" + old;
            } లేకపోతే {
                VHashSha512 digest{v3Global.opt.protect_కీలకం_Defaulted()};
                digest.చొప్పించు(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                బహిర్గతం = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                కొరకు (కొలత_t len = 6; len < బహిర్గతం.కొలత() - 3; len += 3) {
                    స్థిరరాశి string tryout = బహిర్గతం.substr(0, len);
                    ఒకవేళ (m_new_Id_సమితి.చొప్పించు(tryout).రెండవ) {
                        బహిర్గతం = tryout;
                        ఆపు;
                    }
                }
            }
            pair.మొదటి->రెండవ = బహిర్గతం;
        }
        తిరిగి pair.మొదటి->రెండవ;
    }
    string protect_పదాలు_If(స్థిరరాశి string& old, తార్కిక doIt) VL_MT_సురక్షిత {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        ఒకవేళ (!(doIt && v3Global.opt.protectIds())) తిరిగి old;
        string బహిర్గతం;
        string::కొలత_type ప్రారంభం = 0;
        // space, ., ->
        అయితే (నిజం) {
            // When C++11, use find_if and lambda
            string::కొలత_type pos = string::npos;
            string separator;
            trySep(old, ప్రారంభం, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, ప్రారంభం, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, ప్రారంభం, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, ప్రారంభం, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, ప్రారంభం, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, ప్రారంభం, ")", pos /*ref*/, separator /*ref*/);
            ఒకవేళ (pos == string::npos) ఆపు;
            బహిర్గతం += protectIf(old.substr(ప్రారంభం, pos - ప్రారంభం), నిజం) + separator;
            ప్రారంభం = pos + separator.నిడివి();
        }
        బహిర్గతం += protectIf(old.substr(ప్రారంభం), నిజం);
        తిరిగి బహిర్గతం;
    }
    శూన్య writeMapFile(స్థిరరాశి string& filename) స్థిరరాశి {
        V3_బహిర్గతం_Xml_File యొక్క{filename};
        యొక్క.puts_శీర్షిక();
        యొక్క.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        యొక్క.puts("<verilator_id_map>\n");
        {
            కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_nameMap) {
                యొక్క.puts("<map from=\"" + itr.రెండవ + "\" to=\"" + itr.మొదటి + "\"/>\n");
            }
        }
        యొక్క.puts("</verilator_id_map>\n");
    }

అంతరంగ:
    శూన్య trySep(స్థిరరాశి string& old, string::కొలత_type ప్రారంభం, స్థిరరాశి string& trySep,
                string::కొలత_type& posr, string& separatorr) VL_శుద్ధ {
        స్థిరరాశి string::కొలత_type trypos = old.కనుగొను(trySep, ప్రారంభం);
        ఒకవేళ (trypos != string::npos) {
            ఒకవేళ (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(స్థిరరాశి string& లోపల) VL_MT_సురక్షిత {
    తిరిగి v3Global.opt.protectIds() ? "" : లోపల;
}
string VIdProtect::protectIf(స్థిరరాశి string& old, తార్కిక doIt) VL_MT_సురక్షిత {
    తిరిగి VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_పదాలు_If(స్థిరరాశి string& old, తార్కిక doIt) VL_MT_సురక్షిత {
    తిరిగి VIdProtectImp::singleton().protect_పదాలు_If(old, doIt);
}
శూన్య VIdProtect::writeMapFile(స్థిరరాశి string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
