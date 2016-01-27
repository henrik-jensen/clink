#include "pch.h"
#ifdef CLINK_EMBED_LUA_SCRIPTS
const char* dll_dir_lua_script = 
"local function dir_match_generator(word)\n"
"    local matches = {}\n"
"    local root = path.getdirectory(word) or \"\"\n"
"    for _, dir in ipairs(os.globdirs(path.join(root, \"*\"))) do\n"
"        table.insert(matches, dir)\n"
"    end\n"
"    return matches\n"
"end\n"
"clink.arg.register_parser(\"cd\", dir_match_generator)\n"
"clink.arg.register_parser(\"chdir\", dir_match_generator)\n"
"clink.arg.register_parser(\"pushd\", dir_match_generator)\n"
"clink.arg.register_parser(\"rd\", dir_match_generator)\n"
"clink.arg.register_parser(\"rmdir\", dir_match_generator)\n"
"clink.arg.register_parser(\"md\", dir_match_generator)\n"
"clink.arg.register_parser(\"mkdir\", dir_match_generator)\n"
"";const char* dll_env_lua_script = 
"local special_env_vars = {\n"
"    \"cd\", \"date\", \"time\", \"random\", \"errorlevel\",\n"
"    \"cmdextversion\", \"cmdcmdline\", \"highestnumanodenumber\"\n"
"}\n"
"local function env_vars_display_filter(matches)\n"
"    local to_display = {}\n"
"    for _, m in ipairs(matches) do\n"
"        local _, _, out = m:find(\"(%%[^%%]+%%)$\")\n"
"        table.insert(to_display, out)\n"
"    end\n"
"    return to_display\n"
"end\n"
"local function env_vars_find_matches(candidates, prefix, part, result)\n"
"    for _, name in ipairs(candidates) do\n"
"        result:addmatch(prefix..'%'..name:lower()..'%')\n"
"    end\n"
"end\n"
"local function env_vars_match_generator(text, first, last, result)\n"
"    local all = line_state.line:sub(1, last)\n"
"    -- Skip pairs of %s\n"
"    local i = 1\n"
"    for _, r in function () return all:find(\"%b%%\", i) end do\n"
"        i = r + 2\n"
"    end\n"
"    -- Find a solitary %\n"
"    local i = all:find(\"%%\", i)\n"
"    if not i then\n"
"        return false\n"
"    end\n"
"    if i < first then\n"
"        return false\n"
"    end\n"
"    local part = clink.lower(all:sub(i + 1))\n"
"    i = i - first\n"
"    local prefix = text:sub(1, i)\n"
"    env_vars_find_matches(os.getenvnames(), prefix, part, result)\n"
"    env_vars_find_matches(special_env_vars, prefix, part, result)\n"
"    if result:getmatchcount() >= 1 then\n"
"        clink.match_display_filter = env_vars_display_filter\n"
"        return true\n"
"    end\n"
"    return false\n"
"end\n"
"if clink.get_host_process() == \"cmd\" then\n"
"    clink.register_match_generator(env_vars_match_generator, 10)\n"
"end\n"
"";const char* dll_exec_lua_script = 
"local dos_commands = {\n"
"    \"assoc\", \"break\", \"call\", \"cd\", \"chcp\", \"chdir\", \"cls\", \"color\", \"copy\",\n"
"    \"date\", \"del\", \"dir\", \"diskcomp\", \"diskcopy\", \"echo\", \"endlocal\", \"erase\",\n"
"    \"exit\", \"for\", \"format\", \"ftype\", \"goto\", \"graftabl\", \"if\", \"md\", \"mkdir\",\n"
"    \"mklink\", \"more\", \"move\", \"path\", \"pause\", \"popd\", \"prompt\", \"pushd\", \"rd\",\n"
"    \"rem\", \"ren\", \"rename\", \"rmdir\", \"set\", \"setlocal\", \"shift\", \"start\",\n"
"    \"time\", \"title\", \"tree\", \"type\", \"ver\", \"verify\", \"vol\"\n"
"}\n"
"local function get_environment_paths()\n"
"    local paths = clink.split(os.getenv(\"path\"), \";\")\n"
"    -- We're expecting absolute paths and as ';' is a valid path character\n"
"    -- there maybe unneccessary splits. Here we resolve them.\n"
"    local paths_merged = { paths[1] }\n"
"    for i = 2, #paths, 1 do\n"
"        if not paths[i]:find(\"^[a-zA-Z]:\") then\n"
"            local t = paths_merged[#paths_merged];\n"
"            paths_merged[#paths_merged] = t..paths[i]\n"
"        else\n"
"            table.insert(paths_merged, paths[i])\n"
"        end\n"
"    end\n"
"    -- Append slashes.\n"
"    for i = 1, #paths_merged, 1 do\n"
"        paths_merged[i] = paths_merged[i]..\"/\"\n"
"    end\n"
"    return paths_merged\n"
"end\n"
"local function exec_find_dirs(pattern, case_map)\n"
"    local ret = {}\n"
"    for _, dir in ipairs(clink.find_dirs(pattern, case_map)) do\n"
"        if dir ~= \".\" and dir ~= \"..\" then\n"
"            table.insert(ret, dir)\n"
"        end\n"
"    end\n"
"    return ret\n"
"end\n"
"local function exec_match_generator(text, first, last, result)\n"
"    -- If match style setting is < 0 then consider executable matching disabled.\n"
"    local match_style = clink.get_setting_int(\"exec_match_style\")\n"
"    if match_style < 0 then\n"
"        return false\n"
"    end\n"
"    -- We're only interested in exec completion if this is the first word of the\n"
"    -- line, or the first word after a command separator.\n"
"    if clink.get_setting_int(\"space_prefix_match_files\") > 0 then\n"
"        if first > 1 then\n"
"            return false\n"
"        end\n"
"    else\n"
"        local leading = line_state.line:sub(1, first - 1)\n"
"        local is_first = leading:find(\"^%s*\\\"*$\")\n"
"        if not is_first then\n"
"            return false\n"
"        end\n"
"    end\n"
"    -- Split text into directory and name\n"
"    local text_dir = path.getdirectory(text) or \"\"\n"
"    local text_name = path.getname(text)\n"
"    local paths\n"
"    if #text_dir == 0 then\n"
"        -- If the terminal is cmd.exe check it's commands for matches.\n"
"        if clink.get_host_process() == \"cmd.exe\" then\n"
"            result:addmatches(dos_commands)\n"
"        end\n"
"        -- Add console aliases as matches.\n"
"        local aliases = clink.get_console_aliases()\n"
"        result:addmatches(aliases)\n"
"        paths = get_environment_paths();\n"
"    else\n"
"        paths = {}\n"
"        -- 'text' is an absolute or relative path. If we're doing Bash-style\n"
"        -- matching should now consider directories.\n"
"        if match_style < 1 then\n"
"            match_style = 2\n"
"        else\n"
"            match_style = 1\n"
"        end\n"
"    end\n"
"    -- Should we also consider the path referenced by 'text'?\n"
"    if match_style >= 1 then\n"
"        table.insert(paths, text_dir)\n"
"    end\n"
"    -- Search 'paths' for files ending in 'suffices' and look for matches\n"
"    local suffices = clink.split(os.getenv(\"pathext\"), \";\")\n"
"    for _, suffix in ipairs(suffices) do\n"
"        for _, dir in ipairs(paths) do\n"
"            for _, file in ipairs(os.globfiles(dir..\"*\"..suffix)) do\n"
"                file = path.getname(file)\n"
"                result:addmatch(text_dir..file)\n"
"            end\n"
"        end\n"
"    end\n"
"    -- Lastly we may wish to consider directories too.\n"
"    if result:getmatchcount() == 0 or match_style >= 2 then\n"
"        result:addmatches(os.globdirs(text..\"*\"))\n"
"    end\n"
"    clink.matches_are_files()\n"
"    return true\n"
"end\n"
"clink.register_match_generator(exec_match_generator, 50)\n"
"";const char* dll_git_lua_script = 
"local git_argument_tree = {\n"
"    -- Porcelain and ancillary commands from git's man page.\n"
"    \"add\", \"am\", \"archive\", \"bisect\", \"branch\", \"bundle\", \"checkout\",\n"
"    \"cherry-pick\", \"citool\", \"clean\", \"clone\", \"commit\", \"describe\", \"diff\",\n"
"    \"fetch\", \"format-patch\", \"gc\", \"grep\", \"gui\", \"init\", \"log\", \"merge\", \"mv\",\n"
"    \"notes\", \"pull\", \"push\", \"rebase\", \"reset\", \"revert\", \"rm\", \"shortlog\",\n"
"    \"show\", \"stash\", \"status\", \"submodule\", \"tag\", \"config\", \"fast-export\",\n"
"    \"fast-import\", \"filter-branch\", \"lost-found\", \"mergetool\", \"pack-refs\",\n"
"    \"prune\", \"reflog\", \"relink\", \"remote\", \"repack\", \"replace\", \"repo-config\",\n"
"    \"annotate\", \"blame\", \"cherry\", \"count-objects\", \"difftool\", \"fsck\",\n"
"    \"get-tar-commit-id\", \"help\", \"instaweb\", \"merge-tree\", \"rerere\",\n"
"    \"rev-parse\", \"show-branch\", \"verify-tag\", \"whatchanged\"\n"
"}\n"
"clink.arg.register_parser(\"git\", git_argument_tree)\n"
"";const char* dll_go_lua_script = 
"local function flags(...)\n"
"    local p = clink.arg.new_parser()\n"
"    p:set_flags(...)\n"
"    return p\n"
"end\n"
"local go_tool_parser = clink.arg.new_parser()\n"
"go_tool_parser:set_flags(\"-n\")\n"
"go_tool_parser:set_arguments({\n"
"    \"8a\", \"8c\", \"8g\", \"8l\", \"addr2line\", \"cgo\", \"dist\", \"nm\", \"objdump\",\n"
"    \"pack\",\n"
"    \"cover\" .. flags(\"-func\", \"-html\", \"-mode\", \"-o\", \"-var\"),\n"
"    \"fix\"   .. flags(\"-diff\", \"-force\", \"-r\"),\n"
"    \"prof\"  .. flags(\"-p\", \"-t\", \"-d\", \"-P\", \"-h\", \"-f\", \"-l\", \"-r\", \"-s\",\n"
"                     \"-hs\"),\n"
"    \"pprof\" .. flags(-- Options:\n"
"                     \"--cum\", \"--base\", \"--interactive\", \"--seconds\",\n"
"                     \"--add_lib\", \"--lib_prefix\",\n"
"                     -- Reporting Granularity:\n"
"                     \"--addresses\", \"--lines\", \"--functions\", \"--files\",\n"
"                     -- Output type:\n"
"                     \"--text\", \"--callgrind\", \"--gv\", \"--web\", \"--list\",\n"
"                     \"--disasm\", \"--symbols\", \"--dot\", \"--ps\", \"--pdf\",\n"
"                     \"--svg\", \"--gif\", \"--raw\",\n"
"                     -- Heap-Profile Options:\n"
"                     \"--inuse_space\", \"--inuse_objects\", \"--alloc_space\",\n"
"                     \"--alloc_objects\", \"--show_bytes\", \"--drop_negative\",\n"
"                     -- Contention-profile options:\n"
"                     \"--total_delay\", \"--contentions\", \"--mean_delay\",\n"
"                     -- Call-graph Options:\n"
"                     \"--nodecount\", \"--nodefraction\", \"--edgefraction\",\n"
"                     \"--focus\", \"--ignore\", \"--scale\", \"--heapcheck\",\n"
"                     -- Miscellaneous:\n"
"                     \"--tools\", \"--test\", \"--help\", \"--version\"),\n"
"    \"vet\"   .. flags(\"-all\", \"-asmdecl\", \"-assign\", \"-atomic\", \"-buildtags\",\n"
"                     \"-composites\", \"-compositewhitelist\", \"-copylocks\",\n"
"                     \"-methods\", \"-nilfunc\", \"-printf\", \"-printfuncs\",\n"
"                     \"-rangeloops\", \"-shadow\", \"-shadowstrict\", \"-structtags\",\n"
"                     \"-test\", \"-unreachable\", \"-v\"),\n"
"    \"yacc\"  .. flags(\"-l\", \"-o\", \"-p\", \"-v\"),\n"
"})\n"
"local go_parser = clink.arg.new_parser()\n"
"go_parser:set_arguments({\n"
"    \"env\",\n"
"    \"fix\",\n"
"    \"version\",\n"
"    \"build\"    .. flags(\"-o\", \"-a\", \"-n\", \"-p\", \"-installsuffix\", \"-v\", \"-x\",\n"
"                        \"-work\", \"-gcflags\", \"-ccflags\", \"-ldflags\",\n"
"                        \"-gccgoflags\", \"-tags\", \"-compiler\", \"-race\"),\n"
"    \"clean\"    .. flags(\"-i\", \"-n\", \"-r\", \"-x\"),\n"
"    \"fmt\"      .. flags(\"-n\", \"-x\"),\n"
"    \"get\"      .. flags(\"-d\", \"-fix\", \"-t\", \"-u\",\n"
"                        -- Build flags\n"
"                        \"-a\", \"-n\", \"-p\", \"-installsuffix\", \"-v\", \"-x\",\n"
"                        \"-work\", \"-gcflags\", \"-ccflags\", \"-ldflags\",\n"
"                        \"-gccgoflags\", \"-tags\", \"-compiler\", \"-race\"),\n"
"    \"install\"  .. flags(-- All `go build` flags\n"
"                        \"-o\", \"-a\", \"-n\", \"-p\", \"-installsuffix\", \"-v\", \"-x\",\n"
"                        \"-work\", \"-gcflags\", \"-ccflags\", \"-ldflags\",\n"
"                        \"-gccgoflags\", \"-tags\", \"-compiler\", \"-race\"),\n"
"    \"list\"     .. flags(\"-e\", \"-race\", \"-f\", \"-json\", \"-tags\"),\n"
"    \"run\"      .. flags(\"-exec\",\n"
"                        -- Build flags\n"
"                        \"-a\", \"-n\", \"-p\", \"-installsuffix\", \"-v\", \"-x\",\n"
"                        \"-work\", \"-gcflags\", \"-ccflags\", \"-ldflags\",\n"
"                        \"-gccgoflags\", \"-tags\", \"-compiler\", \"-race\"),\n"
"    \"test\"     .. flags(-- Local.\n"
"                        \"-c\", \"-file\", \"-i\", \"-cover\", \"-coverpkg\",\n"
"                        -- Build flags\n"
"                        \"-a\", \"-n\", \"-p\", \"-x\", \"-work\", \"-ccflags\",\n"
"                        \"-gcflags\", \"-exec\", \"-ldflags\", \"-gccgoflags\",\n"
"                        \"-tags\", \"-compiler\", \"-race\", \"-installsuffix\",\n"
"                        -- Passed to 6.out\n"
"                        \"-bench\", \"-benchmem\", \"-benchtime\", \"-covermode\",\n"
"                        \"-coverprofile\", \"-cpu\", \"-cpuprofile\", \"-memprofile\",\n"
"                        \"-memprofilerate\", \"-blockprofile\",\n"
"                        \"-blockprofilerate\", \"-outputdir\", \"-parallel\", \"-run\",\n"
"                        \"-short\", \"-timeout\", \"-v\"),\n"
"    \"tool\"     .. go_tool_parser,\n"
"    \"vet\"      .. flags(\"-n\", \"-x\"),\n"
"})\n"
"local go_help_parser = clink.arg.new_parser()\n"
"go_help_parser:set_arguments({\n"
"    \"help\" .. clink.arg.new_parser():set_arguments({\n"
"        go_parser:flatten_argument(1)\n"
"    })\n"
"})\n"
"local godoc_parser = clink.arg.new_parser()\n"
"godoc_parser:set_flags(\n"
"    \"-zip\", \"-write_index\", \"-analysis\", \"-http\", \"-server\", \"-html\",\"-src\",\n"
"    \"-url\", \"-q\", \"-v\", \"-goroot\", \"-tabwidth\", \"-timestamps\", \"-templates\",\n"
"    \"-play\", \"-ex\", \"-links\", \"-index\", \"-index_files\", \"-maxresults\",\n"
"    \"-index_throttle\", \"-notes\", \"-httptest.serve\"\n"
")\n"
"local gofmt_parser = clink.arg.new_parser()\n"
"gofmt_parser:set_flags(\n"
"    \"-cpuprofile\", \"-d\", \"-e\", \"-l\", \"-r\", \"-s\", \"-w\"\n"
")\n"
"clink.arg.register_parser(\"go\", go_parser)\n"
"clink.arg.register_parser(\"go\", go_help_parser)\n"
"clink.arg.register_parser(\"godoc\", godoc_parser)\n"
"clink.arg.register_parser(\"gofmt\", gofmt_parser)\n"
"";const char* dll_hg_lua_script = 
"local hg_tree = {\n"
"    \"add\", \"addremove\", \"annotate\", \"archive\", \"backout\", \"bisect\", \"bookmarks\",\n"
"    \"branch\", \"branches\", \"bundle\", \"cat\", \"clone\", \"commit\", \"copy\", \"diff\",\n"
"    \"export\", \"forget\", \"grep\", \"heads\", \"help\", \"identify\", \"import\",\n"
"    \"incoming\", \"init\", \"locate\", \"log\", \"manifest\", \"merge\", \"outgoing\",\n"
"    \"parents\", \"paths\", \"pull\", \"push\", \"recover\", \"remove\", \"rename\", \"resolve\",\n"
"    \"revert\", \"rollback\", \"root\", \"serve\", \"showconfig\", \"status\", \"summary\",\n"
"    \"tag\", \"tags\", \"tip\", \"unbundle\", \"update\", \"verify\", \"version\", \"graft\",\n"
"    \"phases\"\n"
"}\n"
"clink.arg.register_parser(\"hg\", hg_tree)\n"
"";const char* dll_p4_lua_script = 
"local p4_tree = {\n"
"    \"add\", \"annotate\", \"attribute\", \"branch\", \"branches\", \"browse\", \"change\",\n"
"    \"changes\", \"changelist\", \"changelists\", \"client\", \"clients\", \"copy\",\n"
"    \"counter\", \"counters\", \"cstat\", \"delete\", \"depot\", \"depots\", \"describe\",\n"
"    \"diff\", \"diff2\", \"dirs\", \"edit\", \"filelog\", \"files\", \"fix\", \"fixes\",\n"
"    \"flush\", \"fstat\", \"grep\", \"group\", \"groups\", \"have\", \"help\", \"info\",\n"
"    \"integrate\", \"integrated\", \"interchanges\", \"istat\", \"job\", \"jobs\", \"label\",\n"
"    \"labels\", \"labelsync\", \"legal\", \"list\", \"lock\", \"logger\", \"login\",\n"
"    \"logout\", \"merge\", \"move\", \"opened\", \"passwd\", \"populate\", \"print\",\n"
"    \"protect\", \"protects\", \"reconcile\", \"rename\", \"reopen\", \"resolve\",\n"
"    \"resolved\", \"revert\", \"review\", \"reviews\", \"set\", \"shelve\", \"status\",\n"
"    \"sizes\", \"stream\", \"streams\", \"submit\", \"sync\", \"tag\", \"tickets\", \"unlock\",\n"
"    \"unshelve\", \"update\", \"user\", \"users\", \"where\", \"workspace\", \"workspaces\"\n"
"}\n"
"clink.arg.register_parser(\"p4\", p4_tree)\n"
"local p4vc_tree = {\n"
"    \"help\", \"branchmappings\", \"branches\", \"diff\", \"groups\", \"branch\", \"change\",\n"
"    \"client\", \"workspace\", \"depot\", \"group\", \"job\", \"label\", \"user\", \"jobs\",\n"
"    \"labels\", \"pendingchanges\", \"resolve\", \"revisiongraph\", \"revgraph\",\n"
"    \"streamgraph\", \"streams\", \"submit\", \"submittedchanges\", \"timelapse\",\n"
"    \"timelapseview\", \"tlv\", \"users\", \"workspaces\", \"clients\", \"shutdown\"\n"
"}\n"
"clink.arg.register_parser(\"p4vc\", p4vc_tree)\n"
"";const char* dll_powershell_lua_script = 
"local function powershell_prompt_filter()\n"
"    local l, r, path = clink.prompt.value:find(\"([a-zA-Z]:\\\\.*)> $\")\n"
"    if path ~= nil then\n"
"        os.chdir(path)\n"
"    end\n"
"end\n"
"if clink.get_host_process() == \"powershell.exe\" then\n"
"    clink.prompt.register_filter(powershell_prompt_filter, -493)\n"
"end\n"
"";const char* dll_prompt_lua_script = 
"clink.prompt = {}\n"
"clink.prompt.filters = {}\n"
"function clink.prompt.register_filter(filter, priority)\n"
"    if priority == nil then\n"
"        priority = 999\n"
"    end\n"
"    table.insert(clink.prompt.filters, {f=filter, p=priority})\n"
"    table.sort(clink.prompt.filters, function(a, b) return a[\"p\"] < b[\"p\"] end)\n"
"end\n"
"function clink.filter_prompt(prompt)\n"
"    local function add_ansi_codes(p)\n"
"        local c = tonumber(clink.get_setting_int(\"prompt_colour\"))\n"
"        if c < 0 then\n"
"            return p\n"
"        end\n"
"        c = c % 16\n"
"       \n"
"        -- Convert from cmd.exe colour indices to ANSI ones.\n"
"        local colour_id = c % 8\n"
"        if (colour_id % 2) == 1 then\n"
"            if colour_id < 4 then\n"
"                c = c + 3\n"
"            end\n"
"        elseif colour_id >= 4 then\n"
"            c = c - 3\n"
"        end\n"
"        -- Clamp\n"
"        if c > 15 then\n"
"            c = 15\n"
"        end\n"
"        -- Build ANSI code\n"
"        local code = \"\\x1b[0;\"\n"
"        if c > 7 then\n"
"            c = c - 8\n"
"            code = code..\"1;\"\n"
"        end\n"
"        code = code..(c + 30)..\"m\"\n"
"        return code..p..\"\\x1b[0m\"\n"
"    end\n"
"    clink.prompt.value = prompt\n"
"    for _, filter in ipairs(clink.prompt.filters) do\n"
"        if filter.f() == true then\n"
"            return add_ansi_codes(clink.prompt.value)\n"
"        end\n"
"    end\n"
"    return add_ansi_codes(clink.prompt.value)\n"
"end\n"
"";const char* dll_self_lua_script = 
"local null_parser = clink.arg.new_parser()\n"
"null_parser:disable_file_matching()\n"
"local inject_parser = clink.arg.new_parser()\n"
"inject_parser:set_flags(\n"
"    \"--help\",\n"
"    \"--pid\",\n"
"    \"--profile\",\n"
"    \"--quiet\",\n"
"    \"--scripts\"\n"
")\n"
"local autorun_dashdash_parser = clink.arg.new_parser()\n"
"autorun_dashdash_parser:set_arguments({ \"--\" .. inject_parser })\n"
"local autorun_parser = clink.arg.new_parser()\n"
"autorun_parser:set_flags(\"--allusers\", \"--help\")\n"
"autorun_parser:set_arguments(\n"
"    {\n"
"        \"install\"   .. autorun_dashdash_parser,\n"
"        \"uninstall\" .. null_parser,\n"
"        \"show\"      .. null_parser,\n"
"        \"set\"\n"
"    }\n"
")\n"
"local set_parser = clink.arg.new_parser()\n"
"set_parser:disable_file_matching()\n"
"set_parser:set_flags(\"--help\")\n"
"set_parser:set_arguments(\n"
"    {\n"
"        \"ansi_code_support\",\n"
"        \"ctrld_exits\",\n"
"        \"exec_match_style\",\n"
"        \"history_dupe_mode\",\n"
"        \"history_expand_mode\",\n"
"        \"history_file_lines\",\n"
"        \"history_ignore_space\",\n"
"        \"history_io\",\n"
"        \"match_colour\",\n"
"        \"prompt_colour\",\n"
"        \"space_prefix_match_files\",\n"
"        \"strip_crlf_on_paste\",\n"
"        \"terminate_autoanswer\",\n"
"        \"use_altgr_substitute\",\n"
"    }\n"
")\n"
"local self_parser = clink.arg.new_parser()\n"
"self_parser:set_arguments(\n"
"    {\n"
"        \"inject\" .. inject_parser,\n"
"        \"autorun\" .. autorun_parser,\n"
"        \"set\" .. set_parser,\n"
"    }\n"
")\n"
"clink.arg.register_parser(\"clink\", self_parser)\n"
"clink.arg.register_parser(\"clink_x86\", self_parser)\n"
"clink.arg.register_parser(\"clink_x64\", self_parser)\n"
"";const char* dll_set_lua_script = 
"local function set_match_generator(word)\n"
"    -- Skip this generator if first is in the rvalue.\n"
"    local leading = line_state.line:sub(1, line_state.first - 1)\n"
"    if leading:find(\"=\") then\n"
"        return false\n"
"    end\n"
"    -- Enumerate environment variables and check for potential matches.\n"
"    local matches = {}\n"
"    for _, name in ipairs(os.getenvnames()) do\n"
"        table.insert(matches, name:lower())\n"
"    end\n"
"    return matches\n"
"end\n"
"clink.arg.register_parser(\"set\", set_match_generator)\n"
"";const char* dll_svn_lua_script = 
"local svn_tree = {\n"
"    \"add\", \"blame\", \"praise\", \"annotate\", \"ann\", \"cat\", \"changelist\", \"cl\",\n"
"    \"checkout\", \"co\", \"cleanup\", \"commit\", \"ci\", \"copy\", \"cp\", \"delete\", \"del\",\n"
"    \"remove\", \"rm\", \"diff\", \"di\", \"export\", \"help\", \"h\", \"import\", \"info\",\n"
"    \"list\", \"ls\", \"lock\", \"log\", \"merge\", \"mergeinfo\", \"mkdir\", \"move\", \"mv\",\n"
"    \"rename\", \"ren\", \"propdel\", \"pdel\", \"pd\", \"propedit\", \"pedit\", \"pe\",\n"
"    \"propget\", \"pget\", \"pg\", \"proplist\", \"plist\", \"pl\", \"propset\", \"pset\", \"ps\",\n"
"    \"resolve\", \"resolved\", \"revert\", \"status\", \"stat\", \"st\", \"switch\", \"sw\",\n"
"    \"unlock\", \"update\", \"up\"\n"
"}\n"
"clink.arg.register_parser(\"svn\", svn_tree)\n"
"";const char* dll_lua_scripts[] = {dll_dir_lua_script,dll_env_lua_script,dll_exec_lua_script,dll_git_lua_script,dll_go_lua_script,dll_hg_lua_script,dll_p4_lua_script,dll_powershell_lua_script,dll_prompt_lua_script,dll_self_lua_script,dll_set_lua_script,dll_svn_lua_script,nullptr,};
#else
const char* dll_embed_path = __FILE__;
const char* dll_dir_lua_file = "dir.lua";
const char* dll_env_lua_file = "env.lua";
const char* dll_exec_lua_file = "exec.lua";
const char* dll_git_lua_file = "git.lua";
const char* dll_go_lua_file = "go.lua";
const char* dll_hg_lua_file = "hg.lua";
const char* dll_p4_lua_file = "p4.lua";
const char* dll_powershell_lua_file = "powershell.lua";
const char* dll_prompt_lua_file = "prompt.lua";
const char* dll_self_lua_file = "self.lua";
const char* dll_set_lua_file = "set.lua";
const char* dll_svn_lua_file = "svn.lua";
const char* dll_lua_files[] = {dll_dir_lua_file,dll_env_lua_file,dll_exec_lua_file,dll_git_lua_file,dll_go_lua_file,dll_hg_lua_file,dll_p4_lua_file,dll_powershell_lua_file,dll_prompt_lua_file,dll_self_lua_file,dll_set_lua_file,dll_svn_lua_file,nullptr,};
#endif