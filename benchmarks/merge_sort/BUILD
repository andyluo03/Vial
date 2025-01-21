cc_test (
    name="merge_sort",
    srcs=["merge_bench.cc"],
    deps=[
        "//core:core",
        "@google_benchmark//:benchmark_main"
    ],
    linkopts = ["-lpthread", "-O3"],
)