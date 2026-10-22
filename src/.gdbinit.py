# pylint: disable=line-too-long,invalid-name,multiple-statements,missing-function-docstring,missing-class-docstring,missing-module-docstring,no-else-return,too-few-public-methods,unused-argument
# DESCRIPTION: Verilator: GDB startup file with useful defines
# SPDX-FileCopyrightText: 2023 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
######################################################################

درآمد tempfile
درآمد gdb  # pylint: disable=import-error


تعریف _vltgdb_get_dump(گرہ):
    gdb.execute(f'set $_gdb_dump_json_str = AstNode::dumpTreeJsonGdb({node})')
    dump = gdb.execute('printf "%s", $_gdb_dump_json_str', مقصد_string=سچ)
    gdb.execute('call (void)free($_gdb_dump_json_str)')
    واپس dump


تعریف _vltgdb_tmpfile():
    واپس tempfile.NamedTemporaryFile(منوال="wt")  # write, text mode


تعریف _vltgdb_fwrite(file, s):
    """Write to file and flush buffer before passing the file to astsee"""
    file.اكتب(s)
    file.flush()


جماعت AstseeCmd(gdb.Command):
    """Verilator: Pretty print or diff nodes using `astsee`. A node can be:
    * an pointer identifier,
    * an address literal,
    * a GDB value (like `$1`) that stores a dump previously done by the `jstash` command.
    Apart from not taking input from a file, it works exactly like `verilator_astsee`:
    * passing one node gives you a pretty print,
    * passing two nodes gives you a diff,
    * for more options see `astsee` readme/help.
    For examples see internals.rst
    """

    تعریف __init__(self):
        بالا().__init__("jtree", gdb.COMMAND_USER, gdb.COMPLETE_عبارت)

    تعریف _null_check(self, old, نیا):
        err = ""
        اگر old == "<nullptr>\n":
            err += "old == <nullptr>\n"
        اگر نیا == "<nullptr>\n":
            err += "new == <nullptr>"
        اگر err:
            اٹھاؤ gdb.GdbError(err.strip("\n"))

    تعریف invoke(self, arg_str, from_tty):
        سے astsee درآمد verilator_cli بطور astsee  # pylint: disable=import-error,import-outside-toplevel
        # we import it here so "no module X" error would occur on typing `jtree` rather than on every gdb start

        # We hack `astsee_verilator`'s arg parser to find arguments with nodes
        # After finding them, we replace them with proper files
        astsee_دلائل = astsee.parser.parse_دلائل(gdb.string_مقصد_argv(arg_str))
        ساتھ _vltgdb_tmpfile() بطور oldfile, _vltgdb_tmpfile() بطور newfile, _vltgdb_tmpfile(
        ) بطور metafile:
            اگر astsee_دلائل.file:
                _vltgdb_fwrite(oldfile, _vltgdb_get_dump(astsee_دلائل.file))
                astsee_دلائل.file = oldfile.name
            اگر astsee_دلائل.newfile:
                _vltgdb_fwrite(newfile, _vltgdb_get_dump(astsee_دلائل.newfile))
                astsee_دلائل.newfile = newfile.name
            اگر astsee_دلائل.meta ہے کچھ_نہیں:
                # pass
                gdb.execute(f'call AstNode::dumpJsonMetaFileGdb("{metafile.name}")')
                astsee_دلائل.meta = metafile.name
            کوشش:
                astsee.مین(astsee_دلائل)
            استثنا SystemExit بطور e:  # astsee prints nice errmsgs on exit(), so rethrow it as GdbError to suppress cryptic python trace
                اٹھاؤ gdb.GdbError("astsee exited with non-zero code") سے e


AstseeCmd()
