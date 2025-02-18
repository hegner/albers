# pylint: disable=invalid-name, redefined-builtin, missing-module-docstring

# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import subprocess
import shutil
from pathlib import Path


sys.path.insert(0, os.path.abspath("../python"))

doc_dir = Path(__file__).parent

# -- Project information -----------------------------------------------------

project = "PODIO"
copyright = "2023, Key4hep authors"
author = "Key4hep authors"


# The full version, including alpha/beta/rc tags
# version = '@podio_VERSION@'
# release = version


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    # auto generation of python bindings documentation from docstrings
    "sphinx.ext.napoleon",
    "myst_parser",
    "breathe",
    "sphinx_copybutton",
]

source_suffix = {".rst": "restructuredtext", ".md": "markdown"}

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "cpp_api.rst", "py_api.rst"]

# cpp as main language
primary_domain = "cpp"
highlight_language = "cpp"

# -- Options for MyST parser -------------------------------------------------
myst_enable_extensions = ["colon_fence", "html_image"]
myst_heading_anchor = 3

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

# # Add any paths that contain custom static files (such as style sheets) here,
# # relative to this directory. They are copied after the builtin static files,
# # so a file named "default.css" will overwrite the builtin "default.css".
# html_static_path tml_static_path = ["_static"]

# Do not copy the source markdown / rst files to the generated documentation
html_copy_source = False
html_show_sourcelink = False

html_theme_options = {
    "collapse_navigation": False,
    "navigation_depth": 4,
    "prev_next_buttons_location": None,  # no navigation buttons
}

# -- Doxygen integration with Breathe -----------------------------------------

breathe_projects = {"PODIO": "_build/cpp/doxygen-xml"}
breathe_default_project = "PODIO"
breathe_domain_by_extension = {"h": "cpp", "cc": "cpp", "ipp": "cpp"}
breathe_default_members = ("members", "undoc-members")

# -- Automatic API documentation (c++) ----------------------------------------

print(f"Executing doxygen in {doc_dir}")
doxygen_version = subprocess.check_output(["doxygen", "--version"], encoding="utf-8")
print(f"Doxygen version: {doxygen_version}")

env = os.environ.copy()
env["DOXYGEN_WARN_AS_ERROR"] = "NO"

os.makedirs("_build/cpp", exist_ok=True)

doxygen_output = subprocess.check_output(
    ["doxygen", "Doxyfile"], cwd=doc_dir, env=env, encoding="utf-8"
)
print(doxygen_output)

cpp_api_index_target = doc_dir / "cpp_api/api.rst"

print(f"Executing breathe apidoc in {doc_dir}")
subprocess.check_call(
    [sys.executable, "-m", "breathe.apidoc", "_build/cpp/doxygen-xml", "-o", "cpp_api"],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    cwd=doc_dir,
    env=env,
)

if not cpp_api_index_target.exists():
    shutil.copyfile(doc_dir / "cpp_api.rst", cpp_api_index_target)

print("Done with c++ API doc generation")

# -- Automatic API documentation (python) --------------------------------------

# Make sure that the __init__ docstrings appear as part of the class
# documentation
autoclass_content = "both"

print("Executing sphinx-apidoc for python API")
print(f"Executing sphinx-apidoc in {doc_dir}")
subprocess.check_call(
    [
        sys.executable,
        "-m",
        "sphinx.ext.apidoc",
        "--force",
        "-o",
        "py_api",
        "../python",
        "../*/*test_*.py",  # exclude tests
        "../python/podio_version.py",  # exclude convenience module
    ],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    env=env,
    cwd=doc_dir,
)

print("Done with python API doc generation")
