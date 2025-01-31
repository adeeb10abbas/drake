load("//tools/workspace:pkg_config.bzl", "pkg_config_repository")

def glew_repository(
        name,
        licenses = ["notice"],  # BSD-3-Clause AND MIT
        modname = "glew",
        pkg_config_paths = [],
        homebrew_subdir = "opt/glew/lib/pkgconfig",
        **kwargs):
    pkg_config_repository(
        name = name,
        licenses = licenses,
        modname = modname,
        pkg_config_paths = pkg_config_paths,
        extra_deprecation = "The @glew external is deprecated in Drake's WORKSPACE and will be removed on or after 2024-01-01.",  # noqa
        **kwargs
    )
