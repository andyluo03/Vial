cc_binary (
    name="many_tasks",
    srcs=["many_tasks.cc"],
    deps=[
        "//core:core",
        "@google_benchmark//:benchmark_main"
    ],
    linkopts = ["-lpthread", "-O3"],
)