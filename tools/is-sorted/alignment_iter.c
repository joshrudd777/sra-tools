/* ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnologmsgy Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 */
#include "alignment_iter.h"
#include "common.h"

#include <klib/text.h>
#include <klib/log.h>
#include <klib/printf.h>
#include <klib/out.h>

#include <kfs/directory.h>
#include <kfs/file.h>
 
#include <vdb/manager.h>
#include <vdb/database.h>
#include <vdb/table.h>
#include <vdb/cursor.h>

/* ----------------------------------------------------------------------------------------------- */

#define ALIG_ITER_N_COLS 2

typedef struct alig_iter
{
    /* common for both modes: the name of the source, and the slice */
    const char * source;

    /* for the CSRA-MODE */
    const VCursor *curs;
    uint32_t idx[ ALIG_ITER_N_COLS ];
    row_range to_process;
    uint64_t rows_processed;
    int64_t current_row;
} alig_iter;


/* releae an alignment-iterator */
rc_t alig_iter_release( struct alig_iter * self )
{
    rc_t rc = 0;
    if ( self == NULL )
        rc = RC( rcApp, rcNoTarg, rcReleasing, rcParam, rcNull );
    else
    {
        if ( self->curs != NULL )
        {
            rc = VCursorRelease( self->curs );
            if ( rc != 0 )
                log_err( "error (%R) releasing VCursor for %s", rc, self->source );
        }
        free( ( void * ) self );
    }
    return rc;
}


static rc_t alig_iter_csra_initialize( struct alig_iter * self, size_t cache_capacity )
{
    const VDBManager * mgr;
    rc_t rc = VDBManagerMakeRead( &mgr, NULL );
    if ( rc != 0 )
        log_err( "aligmnet_iter.alig_iter_initialize.VDBManagerMakeRead() %R", rc );
    else
    {
        const VDatabase * db;
        rc = VDBManagerOpenDBRead( mgr, &db, NULL, "%s", self->source );
        if ( rc != 0 )
            log_err( "aligmnet_iter.alig_iter_initialize.VDBManagerOpenDBRead( '%s' ) %R", self->source, rc );
        else
        {
            const VTable * tbl;
            rc = VDatabaseOpenTableRead( db, &tbl, "%s", "PRIMARY_ALIGNMENT" );
            if ( rc != 0 )
                log_err( "aligmnet_iter.alig_iter_initialize.VDatabaseOpenTableRead( '%s'.PRIMARY_ALIGNMENT ) %R", self->source, rc );
            else
            {
                if ( cache_capacity > 0 )
                    rc = VTableCreateCachedCursorRead( tbl, &self->curs, cache_capacity );
                else
                    rc = VTableCreateCursorRead( tbl, &self->curs );
                if ( rc != 0 )
                    log_err( "aligmnet_iter.alig_iter_initialize.TableCreateCursorRead( '%s'.PRIMARY_ALIGNMENT ) %R", self->source, rc );
                else
                {
                    row_range id_range;
                    rc = add_cols_to_cursor( self->curs, self->idx, "PRIMARY_ALIGNMENT", self->source, ALIG_ITER_N_COLS,
                                             "REF_SEQ_ID", "REF_POS" );
                    if ( rc == 0 )
                    {
                        rc = VCursorIdRange( self->curs, self->idx[ 3 ], &id_range.first_row, &id_range.row_count );
                        if ( rc != 0 )
                            log_err( "aligmnet_iter.alig_iter_initialize.VCursorIdRange( '%s'.PRIMARY_ALIGNMENT ) %R", self->source, rc );
                    }
                    if ( rc == 0 )
                    {
                        self->to_process.first_row = id_range.first_row;
                        self->to_process.row_count = id_range.row_count;
                        self->current_row = id_range.first_row;
                    }
                }
                VTableRelease( tbl );
            }
            VDatabaseRelease( db );
        }
        VDBManagerRelease( mgr );
    }
    return rc;
}

static rc_t alig_iter_common_make( struct alig_iter ** self, const char * source )
{
    rc_t rc = 0;
    if ( self == NULL || source == NULL )
    {
        rc = RC( rcApp, rcNoTarg, rcAllocating, rcParam, rcNull );
        log_err( "aligmnet_iter.alig_iter_make() given a NULL-ptr" );
    }
    else
    {
        alig_iter * o = calloc( 1, sizeof *o );
        *self = NULL;
        if ( o == NULL )
        {
            rc = RC( rcApp, rcNoTarg, rcAllocating, rcMemory, rcExhausted );
            log_err( "aligmnet_iter.alig_iter_make() memory exhausted" );
        }
        else
        {
            o->source = source;
        }
        
        if ( rc == 0 )
            *self = o;
        else
            alig_iter_release( o );
    }
    return rc;
}

/* construct an alignmet-iterator from an accession */
rc_t alig_iter_make( struct alig_iter ** self, const char * acc, size_t cache_capacity )
{
    rc_t rc = alig_iter_common_make( self, acc );
    if ( rc == 0 )
    {
        rc = alig_iter_csra_initialize( *self, cache_capacity );
        if ( rc != 0 )
        {
            alig_iter_release( *self );
            *self = NULL;
        }
    }
    return rc;
}


static rc_t fill_alignment( struct alig_iter * self, int64_t row_id, AlignmentT * al )
{
    uint32_t elem_bits, boff, row_len;
    const VCursor * curs = self->curs;
    uint32_t * idx = self->idx;
    
    /* get the REFERENCE-NAME */
    rc_t rc = VCursorCellDataDirect( curs, row_id, idx[ 0 ], &elem_bits, ( const void ** )&al->rname.addr, &boff, &row_len );
    if ( rc != 0 )
        log_err( "cannot read '%s'.PRIMARY_ALIGNMENT.REF_SEQ_ID[ %ld ] %R", self->source, row_id, rc );
    else
        al->rname.len = al->rname.size = row_len;

    al->row_id = row_id;
    if ( rc == 0 )
    {
        /* get the REFERENCE-POSITION */
        uint32_t * pp;
        rc = VCursorCellDataDirect( curs, row_id, idx[ 1 ], &elem_bits, ( const void ** )&pp, &boff, &row_len );
        if ( rc != 0 )
            log_err( "cannot read '%s'.PRIMARY_ALIGNMENT.REF_POS[ %ld ] %R", self->source, row_id, rc );
        else
            al->pos = pp[ 0 ] + 1; /* to produce the same as the SAM-spec, a 1-based postion! */
    }
    return rc;
}


/* get the next alignemnt from the iter */
bool alig_iter_get( struct alig_iter * self, AlignmentT * alignment )
{
    bool res = false;
    rc_t rc;
    if ( self != NULL && alignment != NULL )
    {
        res = ( self->rows_processed < self->to_process.row_count );
        if ( res )
        {
            rc = fill_alignment( self, self->current_row, alignment );
            if ( rc == 0 )
            {
                self->current_row++;
                self->rows_processed++;
            }
            else
                res = false;
        }
    }
    return res;
}
