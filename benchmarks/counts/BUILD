cc_binary (
    name="counts",
    srcs=["counts.cc"],
    deps=[
        "//core:core",
        "@google_benchmark//:benchmark_main"
    ],
    linkopts = ["-lpthread", "-O3"],
)