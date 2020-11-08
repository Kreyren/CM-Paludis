###! ${PROJECT_NAME}_DISTRIBUTION
###!  - Variable used to define distribution-specific configuration expects string or comma-separated list
###!  - Can be overwritten by '${PROJECT_NAME}_EAPI' and '${PROJECT_NAME}_ENABLE_PBINS'
###!  - Note: Gentoo's EAPI is set as 'gentoo-[num]'
if(${PROJECT_NAME}_DISTRIBUTION STREQUAL "")
	message(STATUS "No distribution was selected, skipping..")
elseif(NOT ${PROJECT_NAME}_DISTRIBUTION STREQUAL "")
	foreach(distro ${${PROJECT_NAME}_DISTRIBUTION})
		if(${distro} STREQUAL mokleus)
			# Add EAPI
			list(APPEND ${PROJECT_NAME}_EAPI mokleus-0)
			# Add relevant PBIN
			if(${PROJECT_NAME}_ENABLE_PBINS)
				list(APPEND ${PROJECT_NAME}_EAPI pbin-mokleus-0)
			endif()
		elseif(${distro} STREQUAL exherbo)
			# Add EAPI
			list(APPEND ${PROJECT_NAME}_EAPI exheres-0)
			# Add relevant PBIN
			if(${PROJECT_NAME}_ENABLE_PBINS)
				list(APPEND ${PROJECT_NAME}_EAPI pbin-exheres-0)
			endif()
		elseif(${distro} STREQUAL gentoo)
			# Add EAPI
			list(APPEND ${PROJECT_NAME}_EAPI
				gentoo-0
				gentoo-1
				gentoo-2
				gentoo-3
				gentoo-4
				gentoo-5
				gentoo-6
				gentoo-7)
			# Add relevant PBIN
			if(${PROJECT_NAME}_ENABLE_PBINS)
				list(APPEND PALUDIS_EAPI
					pbin-gentoo-0
					pbin-gentoo-1
					pbin-gentoo-2
					pbin-gentoo-3
					pbin-gentoo-4
					pbin-gentoo-5
					pbin-gentoo-6
					pbin-gentoo-7)
				# Add support for XML required to process metadata.xml
				set(${PROJECT_NAME}_COMPONENT_XML ON)
			endif()
		elseif(${distro} STREQUAL paludis)
			list(APPEND ${PROJECT_NAME}_EAPI
				paludis-1)
		else()
			message(FATAL_ERROR "UNEXPECTED")
		endif()
	endforeach()
else()
	message(FATAL_ERROR "UNEXPECTED")
endif()

option(${PROJECT_NAME}_EAPI "FIXME" "paludis-1")