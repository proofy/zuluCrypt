/*
 * 
 *  Copyright (c) 2011
 *  name : mhogo mchungu 
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"

#include <stdio.h>
#include <limits.h>

static int status_msg( int st )
{
	switch ( st ){
		case 0 : printf( "SUCCESS: Volume opened successfully\n" ) ;								break ;
		case 1 : printf( "ERROR: failed to mount ntfs file system using ntfs-3g,is ntfs-3g package installed?\n" ) ;		break ;					
		case 2 : printf( "ERROR: there seem to be an open volume accociated with given address\n" );				break ;				
		case 3 : printf( "ERROR: no file or device exist on given path\n" ) ; 							break ;		
		case 4 : printf( "ERROR: wrong passphrase\n" );										break ;			
		case 5 : printf( "ERROR: could not create mount point, invalid path or path already taken\n" ) ;			break ;		
		case 6 : printf( "ERROR: passphrase file does not exist\n" );								break ;	
		case 8 : printf( "ERROR: failed to open volume\n" );									break ;	
		case 9 : printf( "ERROR: mount point path is already taken\n" );							break ;					 
		case 10: printf( "ERROR: \",\" ( comma ) is not a valid mount point\n" );						break ;
		case 11: printf( "ERROR: one or more required argument(s) for this operation is missing\n" );				break ;				
		case 12: printf( "ERROR: could not get a lock on /etc/mtab~\n" );							break ;
		case 13: printf( "ERROR: wrong argument for mode, valid options are \"rw\" or \"ro\"\n" );				break ;
		case 14: printf( "ERROR: could not get enought memory to hold the key file\n" );					break ;
		case 15: printf( "ERROR: failed to open volume and failed to close the mapper, advice to do it manunally\n" );		break ;
		case 16: printf( "ERROR: could not resolve full path of mount point\n" );						break ;
		case 17: printf( "ERROR: could not resolve full path of device address\n" );						break ;
		case 18: printf( "ERROR: -O and -m options can not be used together\n" );						break ;
		default: printf( "ERROR: unrecognized error with status number %d encountered\n",st );
	}
	return st ;
}
int open_volumes( const struct_opts * opts,const char * mapping_name,uid_t uid )
{
	int nmp                  = opts->open_no_mount ;
	int i                    = opts->interactive_passphrase ;
	const char * dev         = opts->device ;
	const char * mount_point = opts->mount_point ;
	const char * mode        = opts->mode ;
	const char * source      = opts->key_source ;
	const char * pass        = opts->key ;
	
	string_t passphrase  ;	
	string_t m_name  ;	
	string_t data ;
	
	const char * cpass ;	
	const char * cname ;
	
	char * device ;
	char * cpoint = NULL ;
	
	size_t len ;
	int st = 0 ;
	
	if( mode == NULL ) 
		return status_msg( 11 ) ;
	
	if( strncmp( mode,"ro",2 ) != 0 ){
		if ( strncmp( mode,"rw",2 ) != 0 )
			return status_msg( 13 ) ;
	}
	
	if( nmp == 1 && mount_point != NULL )
		return status_msg( 18 ) ;
	
	if( nmp == -1 ){
		if( mount_point == NULL )
			return status_msg( 11 ) ;
		
		if( strlen( mount_point ) == 1 )
			if ( strcmp( mount_point,"," ) == 0 )
				return status_msg( 10 ) ;
		
		if( is_path_valid( mount_point ) == 0 )
			return status_msg( 9 ) ;
		
		if( mkdir( mount_point,S_IRWXU ) != 0 )
			return status_msg( 5 ) ;
		
		cpoint = realpath( mount_point,NULL ) ;
		if( cpoint == NULL )
			return status_msg( 16 ) ;
	}			
	
	device = realpath( dev,NULL ) ;
	if( device == NULL ){
		if( nmp == -1 )
			free( cpoint ) ;
		
		return status_msg( 17 ) ;	
	}
	
	m_name = String( mapping_name ) ;	
	replace_bash_special_chars( m_name ) ;	
	StringPrepend( m_name,"zuluCrypt-" ) ;
	
	cname = StringContent( m_name ) ;
	
	if ( i == 1 ){
		printf( "Enter passphrase: " ) ;		
		passphrase = get_passphrase();	
		printf( "\n" ) ;
		cpass = StringContent( passphrase ) ;
		len = StringLength( passphrase ) ;
		st = open_volume( device,cname,cpoint,uid,mode,cpass,len ) ;
		StringDelete( &passphrase ) ;
	}else{
		if( source == NULL || pass == NULL )
			return status_msg( 11 ) ;
		
		if( strcmp( source,"-p" ) == 0 ){
			cpass = pass ;
			len = strlen(pass) ;
			st = open_volume( device,cname,cpoint,uid,mode,cpass,len ) ;		
		}else if( strcmp( source,"-f" ) == 0 ){			
			switch( StringGetFromFile_1( &data,pass ) ){
				case 1 : return status_msg( 16 ) ; 
				case 3 : return status_msg( 14 ) ;
			}
			cpass = StringContent( data ) ;
			len = StringLength( data ) ;
			st = open_volume( device,cname,cpoint,uid,mode,cpass,len ) ;
			StringDelete( &data ) ;
		}
	}
	
	if( st != 0 )
		rmdir( cpoint ) ;
	
	StringDelete( &m_name ) ;
	free( cpoint ) ;
	free( device ) ;
	
	return status_msg( st ) ;
}
