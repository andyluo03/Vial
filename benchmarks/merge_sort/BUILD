cc_binary (
    name="merge_sort",
    srcs=["merge_sort.cc"],
    deps=[
        "//core:core",
        "@google_benchmark//:benchmark_main"
    ],
    linkopts = ["-lpthread", "-O3"],
)