# FIXME(Krey): Define this based on used PALUDIS_DISTRO
foreach(environment ${PALUDIS_ALL_ENVIRONMENTS})
  string(TOUPPER ${environment} uc_environment)
  if(ENABLE_${uc_environment}_ENVIRONMENT)
    add_subdirectory(${environment})
  endif()
endforeach()

