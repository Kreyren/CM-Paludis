# FIXME-Copyright(Krey)

# FIXME-PEERREV(Krey)
foreach(distro PALUDIS_DISTRIBUTION)
	# FIXME-QA(Krey): Add check in case ${distro} is not defined to exit with a fatal err
	add_subdirectory(${distro})
	install(
		FILE "${CMAKE_CURRENT_SOURCE_DIR}/${distro}.conf"
		DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/paludis/distributions")
endforeach()