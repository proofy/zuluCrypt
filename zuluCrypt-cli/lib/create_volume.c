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

#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

/*
 * Some mkfs.xxx tools like mkfs.reiserfs requires and option to be passed to run them in non-interactive mode.
 * Some mkfs.xxx tools like mkfs.ext4 do not. There does not seem to be a standard way to know which tool 
 * require an option and which doesnt and i cant know all of them. The ones i know are specified in the code below in the child process.
 * 
 * I am not aware of any mkfs.xxx tool that will take 20 seconds to complete its task.Hence each mkfs.xxx is given 20 seconds to complete.
 * A tool that continue to run after 20 seconds is assumed to be stuck at an interactive prompt and will be killed since a tool
 * stuck in interactive mode waiting for user input will have the library and whoever is using it. * 
 */


/*
 *check if an mkfs.xxx is still running
 * after 20 seconds and kills it if it is.
 */
void * kill_hanged_mkfs( void * p )
{
	pid_t pid_1 ;
	pid_t pid_2 ;
	pid_t pid_3 ;

	sleep( 20 ) ;
	/*
	 * The probability of the process to exit and the OS giving the same PID to another process
	 * is not very hight but it doesnt hurt making sure the process with the PID we are about 
	 * to kill belong to us. We know this by checking group PID, processes from the same parent
	 * have the same group ID.	 
	 */
	pid_1 = getpgid( getpid() ) ; 
	pid_2 = ( pid_t ) p ;
	pid_3 = getpgid( pid_2 ) ;
	
	if( pid_1 == pid_3 ){	
		/*
		 *  mkfs.xxx is still running, kill the process
		 */
		kill( pid_2,SIGKILL ) ;		
	}
	return ( void * ) 0 ; 
}

int create_volume( const char * dev,const char * fs,const char * type,const char * pass,size_t pass_size,const char * rng )
{
	int status ;
	pid_t pid ;
	pthread_t thread ;
	
	if ( is_path_valid( dev ) == 1 )
		return 1 ;
		
	if( strcmp( type,"luks" ) == 0 )
		if(  strcmp( rng,"/dev/random" ) != 0 )
			if( strcmp( rng,"/dev/urandom" ) != 0 )
				return 2 ;
			
	if( is_path_valid( "/dev/mapper/zuluCrypt-create-new" ) == 0 )
		close_mapper( "/dev/mapper/zuluCrypt-create-new" );	
	
	if( strcmp( type,"luks" )  == 0 ){
		status = create_luks( dev,pass,pass_size,rng ) ;	
		if( status != 0 )
			return 3 ;
		status = open_luks( dev,"zuluCrypt-create-new","rw",pass,pass_size ) ;
		if( status != 0 )
			return 3 ;
	}else if( strcmp( type,"plain") == 0 ){
		status = open_plain( dev,"zuluCrypt-create-new","rw",pass,pass_size,"cbc-essiv:sha256" ) ;
		if( status != 0 )
			return 3 ;		
	}else{
		return 2 ;
	}		
	
	pid = fork() ;
	if( pid == -1 )
		return 3 ;	
	if( pid == 0 ){
		close( 1 ); 
		close( 2 );
		if( strcmp( fs,"ext2" ) == 0 || strcmp( fs,"ext3" ) == 0 || strcmp( fs,"ext4" ) == 0 )
			execl( ZULUCRYPTmkfs,"mkfs","-t",fs,"-m","1","/dev/mapper/zuluCrypt-create-new",( char * ) 0 ) ;
		else if( strcmp( fs,"reiserfs" ) == 0 )
			execl( ZULUCRYPTmkfs,"mkfs","-t","reiserfs","-f","-f","-q","/dev/mapper/zuluCrypt-create-new",( char * ) 0 ) ;	
		else if( strcmp( fs,"jfs" ) == 0 )
			execl( ZULUCRYPTmkfs,"mkfs","-t","jfs","-q","/dev/mapper/zuluCrypt-create-new",( char * ) 0 ) ;
		else
			execl( ZULUCRYPTmkfs,"mkfs","-t",fs,"/dev/mapper/zuluCrypt-create-new",( char * ) 0 ) ;
	}
	
	pthread_create( &thread,NULL,kill_hanged_mkfs,( void * ) pid );
	
	waitpid( pid,&status,0 ) ;	
	
	pthread_cancel( thread );
	
	close_mapper( "/dev/mapper/zuluCrypt-create-new" );	
	
	if( status == 0 )
		return 0 ;
	else
		return 3 ;	
}
