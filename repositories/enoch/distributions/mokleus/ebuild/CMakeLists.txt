paludis_add_test(conditional_functions BASH EBUILD_MODULE_SUFFIXES exheres-0)

install(
	PROGRAMS
		"${CMAKE_CURRENT_SOURCE_DIR}/build_functions.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_infovars.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_init.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_initrm.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_initmisc.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_loadenv.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_metadata.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_killold.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_killoldrm.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_saveenv.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_tidyup.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_tidyuprm.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/builtin_variable.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/conditional_functions.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/ever_functions.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/exlib_functions.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/list_functions.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/output_functions.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_bad_options.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_config.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_info.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_nofetch.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_postinst.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_postrm.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_preinst.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_prerm.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_pretend.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/pkg_setup.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/portage_stubs.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_compile.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_configure.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_fetch_extra.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_install.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_test.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_test_expensive.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_prepare.bash"
		"${CMAKE_CURRENT_SOURCE_DIR}/src_unpack.bash"
	DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/paludis/exheres-0")