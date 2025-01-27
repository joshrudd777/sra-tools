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

add_compile_definitions( __mod__="tools/fuse" )

set ( UNIX_SRC unix/remote-fuser-sys.c unix/sra-fuser-sys.c )
set ( LINUX_SRC ${UNIX_SRC} )
set ( MAC_SRC ${UNIX_SRC} )
set ( WIN_SRC )

if ( "windows" STREQUAL ${OS} )
    set ( OS_SPECIFIC_SRC ${WIN_SRC} )
    include_directories( win )
elseif( "linux" STREQUAL ${OS} )
    set ( OS_SPECIFIC_SRC ${LINUX_SRC} )
    include_directories( unix )
elseif( "mac" STREQUAL ${OS} )
    set ( OS_SPECIFIC_SRC ${MAC_SRC} )
    include_directories( unix )
endif()

# External
if( LibXml2_FOUND  )
	add_executable( sra-fuser
		zlib-simple
		log
		node
		tar-list
		file
		tar-file
		tar-node
		directory
		formats
		accessor
		kfile-accessor
		xml
		text-file
		sra-list
		sra-directory
		sra-node
		sra-fastq
		sra-sff
		sra-fuser
		${OS_SPECIFIC_SRC}
	)
	target_link_libraries( sra-fuser ${COMMON_LINK_LIBRARIES} ${COMMON_LIBS_READ} xml2 )
	MakeLinksExe( sra-fuser false )

	add_executable( remote-fuser
		log
		node
		accessor
		kfile-accessor
		remote-xml
		remote-cache
		remote-link
		remote-file
		remote-directory
		remote-fuser
		${OS_SPECIFIC_SRC}
	)
	target_link_libraries( remote-fuser ${COMMON_LINK_LIBRARIES} ${COMMON_LIBS_WRITE} srareader xml2 )
	MakeLinksExe( remote-fuser false )
endif()