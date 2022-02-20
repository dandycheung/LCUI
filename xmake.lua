set_project("lcui")
set_version("3.0.0-a")
set_warnings("all")
add_rules("mode.debug", "mode.release", "mode.coverage")
add_requires("libomp", "libxml2", "libpng", "libjpeg", "libx11", "fontconfig", {optional = true})
add_requires("freetype", {optional = true, configs = {shared = false}})
add_rpathdirs("@loader_path/lib", "@loader_path")
add_defines("LCUI_EXPORTS", "YUTIL_EXPORTS", "UNICODE", "_CRT_SECURE_NO_WARNINGS")
add_includedirs("include", "tests/lib/ctest/include/", "lib/yutil/include/")
includes("tests/lib/ctest/xmake.lua")
includes("lib/*/xmake.lua")
includes("examples/*/xmake.lua")
includes("tests/xmake.lua")

option("ci-env", {showmenu = true, default = false})

option("enable-openmp", {showmenu = true, default = true})

option("enable-touch")
    set_showmenu(true)
    set_default(true)
    set_configvar("ENABLE_TOUCH", 1)

if has_config("ci-env") then
    add_defines("CI_ENV")
end

if is_plat("windows") then
    add_defines("_CRT_SECURE_NO_WARNINGS")
else
    add_cxflags("-fPIC")
    if is_mode("coverage") then
        add_cflags("-ftest-coverage", "-fprofile-arcs", {force = true})
        add_syslinks("gcov")
    end
end

target("lcui_tests")
    set_kind("binary")
    set_rundir("tests")
    add_files("tests/run_tests.c", "tests/cases/*.c")
    add_includedirs("tests/lib/ctest/include/")
    add_deps("ctest", "lcui")
    on_run(function (target)
        import("core.base.option")
        local argv = {}
        local options = {{nil, "memcheck",  "k",  nil, "enable memory check."}}
        local args = option.raw_parse(option.get("arguments") or {}, options)
        os.cd("$(projectdir)/tests")
        if args.memcheck then
            if is_plat("windows") then
                table.insert(argv, target:targetfile())
                os.execv("drmemory", argv)
            else
                table.insert(argv, "valgrind")
                table.insert(argv, "--leak-check=full")
                table.insert(argv, "--error-exitcode=42")
                table.insert(argv, target:targetfile())
                os.execv("sudo", argv)
            end
        else
            os.execv(target:targetfile())
        end
    end)

target("lcui")
    set_kind("$(kind)")
    add_files("src/*.c", "lib/*/src/**.c")
    add_includedirs("lib/yutil/include")
    add_configfiles("include/LCUI/config.h.in")
    set_configdir("include/LCUI")
    add_headerfiles("include/LCUI.h")
    add_headerfiles("include/(LCUI/**.h)")
    add_packages("libomp", "libxml2", "libx11", "libpng", "libjpeg", "freetype", "fontconfig")
    add_options("enable-openmp")

    if has_package("libomp") and has_config("enable-openmp") then
        set_configvar("ENABLE_OPENMP", 1)
    end

    if is_plat("windows") then
        add_options("enable-touch")
        add_files("lib/platform/src/windows/*.c")
        add_links("Shell32")
    else
        add_files("lib/platform/src/linux/*.c")
        add_packages("libx11")
        if has_package("libx11") then
            set_configvar("HAVE_LIBX11", 1)
        end
        add_syslinks("pthread", "dl")
    end

    if has_package("fontconfig") then
        set_configvar("HAVE_FONTCONFIG", 1)
    end
    if has_package("freetype") then
        set_configvar("HAVE_FREETYPE", 1)
    end

    if has_package("libjpeg") then
        set_configvar("HAVE_LIBJPEG", 1)
    end
    if has_package("libpng") then
        set_configvar("HAVE_LIBPNG", 1)
    end
    if has_package("libxml2") then
        set_configvar("WITH_LIBXML2", 1)
    end

target("headers")
    set_kind("phony")
    set_default(false)
    before_build(function (target)
        -- Copy the header file of the internal library to the LCUI header file directory
        os.cp("$(projectdir)/lib/util/include/*.h", "$(projectdir)/include/LCUI/util/")
        os.cp("$(projectdir)/lib/yutil/include/yutil/*.h", "$(projectdir)/include/LCUI/util/")

        os.cp("$(projectdir)/lib/thread/include/*.h", "$(projectdir)/include/LCUI/")

        os.cp("$(projectdir)/lib/css/include/*.h", "$(projectdir)/include/LCUI/")
        os.cp("$(projectdir)/lib/css/include/css/*.h", "$(projectdir)/include/LCUI/css/")

        os.cp("$(projectdir)/lib/font/include/*.h", "$(projectdir)/include/LCUI/")
        os.cp("$(projectdir)/lib/font/include/font/*.h", "$(projectdir)/include/LCUI/font/")

        os.cp("$(projectdir)/lib/pandagl/include/*.h", "$(projectdir)/include/LCUI/")
        os.cp("$(projectdir)/lib/pandagl/include/pandagl/*.h", "$(projectdir)/include/LCUI/pandagl/")

        os.cp("$(projectdir)/lib/image/include/*.h", "$(projectdir)/include/LCUI/image/")
        os.cp("$(projectdir)/lib/ui/include/*.h", "$(projectdir)/include/LCUI/")
        os.cp("$(projectdir)/lib/ui-widgets/include/*.h", "$(projectdir)/include/LCUI/ui/widgets/")
        os.cp("$(projectdir)/lib/ui-cursor/include/*.h", "$(projectdir)/include/LCUI/ui/")
        os.cp("$(projectdir)/lib/ui-builder/include/*.h", "$(projectdir)/include/LCUI/ui/")
        os.cp("$(projectdir)/lib/ui-server/include/*.h", "$(projectdir)/include/LCUI/ui/")
        os.cp("$(projectdir)/lib/platform/include/*.h", "$(projectdir)/include/LCUI/")
        os.cp("$(projectdir)/lib/text/include/*.h", "$(projectdir)/include/LCUI/")
        os.cp("$(projectdir)/lib/timer/include/*.h", "$(projectdir)/include/LCUI/")
        os.cp("$(projectdir)/lib/worker/include/*.h", "$(projectdir)/include/LCUI/")
    end)
