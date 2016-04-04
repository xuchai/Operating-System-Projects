/* p4.c         */

# include <sys/types.h>
# include <sys/socket.h>
# include <sys/stat.h>

# include <netinet/in.h>
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <arpa/inet.h>
# include <dirent.h>
# include <math.h>

# define BUFFER_SIZE 1024
# define listener_port 8765
# define n_blocks 128
# define blocksize 4096

/* struct for disk memory  */
struct Disk {
    char** disk_files;
    char disk_mem[4][32];
    int num_empty;
    int curr_index;
};

/* function that print current disk memory  */
void print_disk(struct Disk *myDisk) {
    int i = 0, j = 0;

    while ( j < 32 ) {
        printf( "=" );
        ++j;
    }
    printf( "\n" );
    j = 0;

    while (i < 4) {
        while ( j < 32 ) {
            printf( "%c", myDisk->disk_mem[i][j] );
            ++j;
        }
        printf( "\n" );
        ++i;
        j = 0;
    }

    while ( j < 32 ) {
        printf( "=" );
        ++j;
    }
    printf( "\n" );
}

int main() {
    /* Initialize myDisk   */
    struct Disk *myDisk = malloc( sizeof(struct Disk) );
    myDisk->disk_files = malloc( 26 * sizeof(char*) );
    myDisk->num_empty = n_blocks;
    myDisk->curr_index = 0;
    int i = 0, j = 0;
    while ( i < 4 ) {
        while ( j < 32 ) {
            myDisk->disk_mem[i][j] = '.';
            ++j;
        }
        ++i;
        j = 0;
    }

    /* Create hidden directory on server    */
    struct stat st = {0};
    
    if ( stat( "./.storage", &st ) == -1 ) {
        mkdir( "./.storage", 0700 );
    }
    /* Remove all files if dir already exists   */
    else {
        system( "exec rm -r ./.storage/*" );
    }

    /* Create the listener socket as TCP socket */
    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    
    if ( sock < 0 ) {
        perror( "socket() failed" );
        exit( EXIT_FAILURE );
    }

    /* socket structures    */
    struct sockaddr_in server;
    
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    /* listen to port number 8765  */
    unsigned short port = listener_port;

    /* host-to-network-shot */
    server.sin_port = htons( port );
    int len = sizeof( server );
    
    /* bind()   */
    if ( bind( sock, (struct sockaddr *)&server, len ) < 0 ) {
        perror( "bind() failed" );
        exit( EXIT_FAILURE );
    }

    listen( sock, 5);   /* max number of waiting clients */
    printf( "Block size is %d\n", blocksize );
    printf( "Number of blocks is %d\n", n_blocks );
    printf( "Listening on port %d\n", listener_port );


    struct sockaddr_in client;
    int fromlen = sizeof( client );
    
    int pid;
    char buffer[ BUFFER_SIZE ];

    while (1) {
        int newsock = accept (sock, (struct sockaddr *)&client, 
                                (socklen_t*)&fromlen );
        printf( "Received incoming connection from %s\n",
                inet_ntoa( (struct in_addr)client.sin_addr ) );
            
        /* handle socket in a child process */
        pid = fork();

        if ( pid < 0 ) {
            perror( "fork() failed" );
            exit( EXIT_FAILURE );
        }
        else if ( pid == 0 ) {
            int n;
            /* blocked on recv() */
            do {
                n = recv( newsock, buffer, BUFFER_SIZE, 0 );

                if ( n < 0 ) {
                    perror( "recv() failed" );
                }
                else if ( n == 0) {
                    /* only sucess if n > 0 */
                }
                else {
                    buffer[n] = '\0';
                    char *token;
                    token = strtok(buffer, "\r\n");
                    printf( "[CHILD %d] Rcvd: %s\n", getpid(), token );

                    /* analyzing buffer... */
                    char cmdStr[20];
                    char filename[50];
                    char *pch;
                    char **cmdArray;
                    int index = 0;
                    char* msg;
                    int file_exists = 1;
                    int read_success = 0;
                    FILE *file;
                    cmdArray = malloc( 4*sizeof(char*) );
                    pch = strtok( buffer, " " );
                    while ( pch != NULL ) {
                        cmdArray[index] = malloc( (strlen(pch)+1) * sizeof(char) );
                        strcpy( cmdArray[index], pch );
                        ++index;
                        pch = strtok( NULL, " ");
                    }
                    
                    /* get cmdStr and filename  */
                    strcpy( cmdStr, cmdArray[0] );

                    /* if DIR command */
                    if ( strcmp( cmdStr, "DIR" ) == 0 ) {
                        DIR *d;
                        struct dirent *dir;
                        d = opendir("./.storage/");
                        char filenames[200];
                        int filecount = 0;
                        if ( d != NULL ) {
                            /* print all files and dir within d */
                            while ( (dir = readdir(d)) != NULL ) {
                                if ( dir->d_name[0] != '.' ) {
                                    ++filecount;
                                    strcat( filenames, dir->d_name );
                                    strcat( filenames, "\n" );
                                }
                            }
                            closedir( d );
                            msg = malloc( 200 * sizeof(char) );
                            sprintf( msg, "%d", filecount );
                            strcat( msg, "\n" );
                            strcat( msg, filenames );
                            if ( filecount == 0 ) strcpy( msg, "0\n");
                            printf("%s with length %lu\n", msg, strlen(msg));
                        }
                        else {
                            msg = malloc( 20 * sizeof(char) );
                            strcpy( msg, "ERROR: readdir() error\n" );
                        }

                    }

                    /* check if file exists */
                    else if ( strcmp( cmdStr, "STORE" ) == 0
                            || strcmp( cmdStr, "READ" )
                            || strcmp( cmdStr, "DELETE" ) ) {
                        strcpy( filename, ".storage/" );
                        strcat( filename, cmdArray[1] );

                        /* check if file exists */
                        file = fopen( filename, "rb" );
                        if (file == NULL) {
                            file_exists = 0;
                        }
                    }
                   
                    /* if STORE command     */
                    if ( strcmp( cmdStr, "STORE" ) == 0 ) {
                        int filebytes = atoi( cmdArray[2] );
                        
                        /*  if file exists    */                         
                        if ( file_exists == 1 ) {
                            msg = malloc( 20 * sizeof(char) );
                            strcpy( msg, "ERROR: FILE EXISTS\n" );
                        }
                        /*  else, write file to server */
                        else {                         
                            char filebuffer[filebytes];
                            /* recv file from client    */
                            int ret = recv( newsock, filebuffer, filebytes+2, 0 );
                            if ( ret > 0 ) {
                                filebuffer[filebytes] = '\0';
                                
                                /* store file on disk memory    */
                                char file_char = myDisk->curr_index + 65;
                                int file_blocks = filebytes / blocksize;
                                int file_clusters = 1;
                                if ( filebytes % blocksize != 0 ) ++file_blocks;
                                myDisk->num_empty -= file_blocks;

                                /* check if enough memory   */
                                if ( myDisk->num_empty < 0 ) {
                                    /* if not enough memory, sent error back    */
                                    msg = malloc( 40 * sizeof(char) );
                                    strcpy( msg, "ERROR: INSUFFICIENT DISK SPACE\n" );
                                    myDisk->num_empty += file_blocks;
                                }
                                else {
                                    /* else, write file and sent ACK back  */
                                    file = fopen( filename, "wb" );
                                    //fwrite( filebuffer, sizeof(file), filebytes, file );
                                    fwrite( filebuffer, sizeof(filebuffer), 1, file );
                                    fclose( file );
                                    msg = malloc( 4 * sizeof(char) );
                                    strcpy( msg, "ACK\n" );

                                    /* store file name on disk_files    */
                                    myDisk->disk_files[myDisk->curr_index] = malloc( (strlen(cmdArray[1]) + 1) * sizeof(char) );
                                    strcpy(myDisk->disk_files[myDisk->curr_index], cmdArray[1] );
                                    ++myDisk->curr_index;

                                    /* check clusters, store file_mem */
                                    i = 0; j = 0;
                                    int blocks_left = file_blocks;
                                    while ( i < 4 ) {
                                        while ( j < 32 ) {
                                            if ( myDisk->disk_mem[i][j] == '.' ) {
                                                myDisk->disk_mem[i][j] = file_char;
                                                --blocks_left;
                                                if ( blocks_left == 0 ) break;
                                                if ( j+1 < 32 && myDisk->disk_mem[i][j+1] != '.' ) ++file_clusters;
                                                else if ( i+1 < 4 && myDisk->disk_mem[i+1][0] != '.' ) ++file_clusters;
                                            }
                                            ++j;
                                        }
                                        if ( blocks_left == 0 ) break;
                                        ++i;
                                        j = 0;
                                    }

                                    char tmp_str[20];
                                    if ( file_clusters == 1 ) strcpy( tmp_str, "cluster" );
                                    else strcpy( tmp_str, "clusters" );

                                    /* print store disk memory results    */
                                    printf( "[CHILD %d] Stored file '%c' (%d bytes; %d blocks; %d %s)\n"
                                            , getpid(), file_char, filebytes, file_blocks, file_clusters, tmp_str );
                                    printf( "[CHILD %d] Simulated Clustered Disk Space Allocation:\n", getpid() );
                                    print_disk( myDisk );
                                }                             
                            }
                            /* return error if unsuccessful  */
                            else {
                                msg = malloc( 20 * sizeof(char) );
                                strcpy( msg, "ERROR: rcvd() failed\n" );
                            }                
                        }
                    }
                    /* if READ command  */
                    else if ( strcmp( cmdStr, "READ" ) == 0 ) {
                        int byte_offset = atoi( cmdArray[2] );
                        int read_length = atoi( cmdArray[3] );

                        /* if file not exists */
                        if ( file_exists == 0 ) {
                            msg = malloc( 20 * sizeof(char) );
                            strcpy( msg, "ERROR: NO SUCH FILE\n" );
                        }
                        else {
                            /* check if byte range valid    */
                            fseek( file, 0, SEEK_END );
                            long filesize = ftell( file );
                            rewind ( file ); 
                            
                            if ( (byte_offset + read_length) > filesize ) {
                                msg = malloc( 40 * sizeof(char) );
                                strcpy( msg, "ERROR: INVALID BYTE RANGE\n" );
                            }
                            /* else read file   */
                            else {
                                fseek( file, byte_offset, SEEK_SET );
                                char read_msg[read_length];
                                
                                fread(read_msg, sizeof(read_msg), 1, file );
                                read_msg[read_length] = '\0';

                                /* add read data to return msg */
                                msg = malloc( (read_length+20) * sizeof(char) );
                                strcpy( msg, "ACK " );
                                strcat( msg, cmdArray[3] );
                                strcat( msg, "\n" );
                                strcat( msg, read_msg);
                                strcat( msg, "\n" );
                                read_success = 1;
                            }
                        }
                    }
                    /* if DELETE command */
                    else if ( strcmp( cmdStr, "DELETE" ) == 0 ) {
                        /* if file not exists */
                        if ( file_exists  == 0) {
                            msg = malloc( 20 * sizeof(char) );
                            strcpy( msg, "ERROR: NO SUCH FILE\n" );
                        }
                        /* else delete file */
                        else {
                            int ret;
                            ret = remove( filename );
                            if ( ret == 0 ) {
                                msg = malloc( 4 * sizeof(char) );
                                strcpy( msg, "ACK\n" );

                                /* delete file from disk memory    */
                                int num_deleted = 0;
                                char delete_char;
                                i = 0; j = 0;
                                /* get delete char from disk files  */
                                while ( i < myDisk->curr_index ) {
                                    if ( strcmp( myDisk->disk_files[i], cmdArray[1] ) == 0 ) {
                                        delete_char = i + 65;
                                        i = 0;
                                        break;
                                    }
                                    ++i;
                                }
                                /* delete from disk memory  */
                                while ( i < 4 ) {
                                    while ( j < 32 ) {
                                        if ( myDisk->disk_mem[i][j] == delete_char ) {
                                            myDisk->disk_mem[i][j] = '.';
                                            ++num_deleted;
                                        }
                                        ++j;
                                    }
                                    ++i;
                                    j = 0;
                                }
                                /* print delete disk memory results    */
                                printf( "[CHILD %d] Deleted %s file '%c' (deallocated %d blocks)\n"
                                            , getpid(), cmdArray[1], delete_char, num_deleted );
                                printf( "[CHILD %d] Simulated Clustered Disk Space Allocation:\n", getpid() );
                                print_disk( myDisk );
                            }
                            else {
                                msg = malloc( 20 * sizeof(char) );
                                strcpy( msg, "ERROR: delete() error\n" );
                            }
                        }
                    }
                    /* else, invalid command */             
                    else if ( strcmp( cmdStr, "DIR" ) != 0 ){
                        msg = malloc( 20 * sizeof(char) );
                        strcpy( msg, "ERROR: INVALID COMMAND\n" );          
                    }

                    /* send msg back to client  */
                    n = send( newsock, msg, strlen(msg), 0 );
                    fflush( NULL );
                    if ( n != strlen(msg) ) {
                        perror( "ERROR: send() failed" );
                    }

                    /* printf additional output if read success  */
                    if (read_success != 1 ) printf( "[CHILD %d] Sent: %s", getpid(), msg );               
                    else {
                        char read_char = '\0';
                        i = 0;
                        while ( i < myDisk->curr_index ) {
                            if ( strcmp( myDisk->disk_files[i], cmdArray[1] ) == 0 ) {
                                read_char = i + 65;
                                i = 0;
                                break;
                            }
                            ++i;
                        }
                        /* print read disk memory results    */
                        printf( "[CHILD %d] Sent: ACK %d\n", getpid(), atoi(cmdArray[3]) );
                        printf( "[CHILD %d] Sent %d bytes (from '%c' blocks) from offset %d\n"
                                , getpid(), atoi(cmdArray[3]), read_char, atoi(cmdArray[2]) );
                    }
                    
                    /* free dynamically allocated memory    */
                    i = 0;
                    while ( i < index ) {
                        free( cmdArray[i] );
                        ++i;
                    }
                    free( cmdArray );
                    free( msg );
                }
            }
            /* do...while exits when recv() call returns 0 */
            while ( n > 0 );
            
            printf( "[CHILD %d] Client closed its socket....terminating\n",
                     getpid() );
            close( newsock );     
            exit( EXIT_SUCCESS );   /* child terminates */

            /* handle zombies here...   */

        }
        /* pid > 0 PARENT   */
        else {
            close( newsock );
        }
    }

    /* free dynamically allocated memory    */
    i = 0;
    while ( i < 26 ) {
        if ( myDisk->disk_files[i] != NULL ) {
            free( myDisk->disk_files[i] );
            ++i;
        }
    }
    free( myDisk->disk_files );


    close( sock);
    
    return EXIT_SUCCESS;
}
