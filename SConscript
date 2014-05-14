from os.path import join
Import('env')


def find_sources(root, depth=4):
    sources = []
    wildcards = root

    for i in range(depth):
        wildcards = join(wildcards, "*")
        sources += env.Glob(wildcards + "cpp")
    return sources


sources = find_sources("app") + find_sources("common")
print [x.abspath for x in sources]
env.Append(CPPPATH = "/common",
           LINKFLAGS = " -lboost_system -lboost_thread")
obj = env.Program(target = "diag", source = sources)
