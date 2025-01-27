# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================

add_compile_definitions( __mod__="tools/ccextract" )

# External
if( LibXml2_FOUND )
	add_executable( ccextract ccextract )
	if( SINGLE_CONFIG )
		target_link_libraries( ccextract
		        ${COMMON_LINK_LIBRARIES}
		        ${NCBI_VDB_LIBDIR}/libkxfs.${STLX}
		        ${NCBI_VDB_LIBDIR}/libkxml.${STLX}
		        ${COMMON_LIBS_READ}
		        ${LIBXML2_LIBRARIES}
		    )
	else()
		target_link_libraries( ccextract
		        ${COMMON_LINK_LIBRARIES}
		        kxfs
		        kxml
		        ${COMMON_LIBS_READ}
		        xml2
		    )
	endif()

        if ( "mac" STREQUAL ${OS} )
            target_link_libraries( ccextract iconv )
        endif()
	MakeLinksExe( ccextract false )
endif()
