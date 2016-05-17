

find_package( Boost REQUIRED
	COMPONENTS
		system
)

find_package( PythonLibs REQUIRED )

add_library( testme SHARED
	main.cpp
)

target_include_directories( testme
	PUBLIC
		${Boost_INCLUDE_DIRS}
		${PYTHON_INCLUDE_DIRS}
		pybind11/include
)

target_link_libraries( testme
	PUBLIC
		${Boost_LIBRARIES}
)

target_compile_features( testme
	PUBLIC
		cxx_constexpr
)
