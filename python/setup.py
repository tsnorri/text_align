from setuptools import setup
from setuptools.extension import Extension
from Cython.Build import cythonize
import os


def remove_empty(string_list):
	return list(filter(lambda x: 0 < len(x), string_list))
	
def from_environment(var_name):
	return remove_empty(os.environ[var_name].split(" "))


include_dirs = ["."]
include_dirs.extend(os.environ["INCLUDE_DIRS"].split(" "))
include_dirs = remove_empty(include_dirs)

extensions = [
	Extension("*", ["text_align/*.pyx"],
		language			= 'c++',
		extra_compile_args	= from_environment("EXTRA_CXXFLAGS"),
		extra_link_args		= from_environment("EXTRA_LDFLAGS"),
		include_dirs		= include_dirs,
		library_dirs		= from_environment("LIB_DIRS"),
		libraries			= from_environment("LIBRARIES")
	)
]

setup(
	name = "text_align",
	packages = ["text_align"],
	ext_modules = cythonize(extensions)
)
