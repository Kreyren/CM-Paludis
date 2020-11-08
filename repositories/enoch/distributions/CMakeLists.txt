# FIXME-PEER(Krey)
foreach(distro ${PALUDIS_DISTRIBUTION})
	if(mokleus IN_LIST PALUDIS_DISTRIBUTION)
		add_subdirectory(mokleus)
	elseif(gentoo IN_LIST PALUDIS_DISTRIBUTION)
		add_subdirectory(gentoo)
	elseif(paludis IN_LIST PALUDIS_DISTRIBUTION)
		add_subdirectory(paludis)
	elseif(exherbo IN_LIST PALUDIS_DISTRIBUTION)
		add_subdirectory(exherbo)
	else
		(message FATAL_ERROR "Distribution '${distro}' is not implemented")
	endif
endforeach()