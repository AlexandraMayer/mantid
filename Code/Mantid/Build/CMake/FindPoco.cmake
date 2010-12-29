# - try to find Poco libraries and include files
# POCO_INCLUDE_DIR where to find Standard.hxx, etc.
# POCO_LIBRARIES libraries to link against
# POCO_FOUND If false, do not try to use POCO

find_path ( POCO_INCLUDE_DIR Poco/Poco.h )

find_library ( POCO_LIB_FOUNDATION NAMES PocoFoundation )
find_library ( POCO_LIB_UTIL NAMES PocoUtil )
find_library ( POCO_LIB_XML NAMES PocoXML )
find_library ( POCO_LIB_NET NAMES PocoNet )

find_library ( POCO_LIB_FOUNDATION_DEBUG NAMES PocoFoundationd )
find_library ( POCO_LIB_UTIL_DEBUG NAMES PocoUtild )
find_library ( POCO_LIB_XML_DEBUG NAMES PocoXMLd )
find_library ( POCO_LIB_NET_DEBUG NAMES PocoNetd )

set ( POCO_LIBRARIES optimized ${POCO_LIB_FOUNDATION}
                     optimized ${POCO_LIB_UTIL}
                     optimized ${POCO_LIB_XML}
                     optimized ${POCO_LIB_NET}
		             debug ${POCO_LIB_FOUNDATION_DEBUG}
                     debug ${POCO_LIB_UTIL_DEBUG}
                     debug ${POCO_LIB_XML_DEBUG}
                     debug ${POCO_LIB_NET_DEBUG}
)

# handle the QUIETLY and REQUIRED arguments and set POCO_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Poco DEFAULT_MSG POCO_LIBRARIES POCO_INCLUDE_DIR )
