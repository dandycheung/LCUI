add_rules("mode.debug", "mode.release")
add_includedirs("include", "../../../include")

target("test")
    set_kind("static")
    add_files("src/*.c")
