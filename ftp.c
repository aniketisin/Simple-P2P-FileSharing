#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <time.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <regex.h>

extern int errno;

#define IndexGet 1
#define FileHash 2
#define FileDownload 3
#define FileUpload 4
#define Quit 5
#define History 6
#define packetSize 1024

int client = 0; //0 for udp, 1 for tcp
char history[1000][1024];
int counter = 0; //no of commands given
int regexMatch = 0, index1 = 0;
int err_flag = -1;

typedef struct Hashing {
	char filename[150];		//name of the file
	time_t stamp;			//time stamp
	unsigned char hash[MD5_DIGEST_LENGTH]; //hashed using MD5
} Hashing;



typedef struct File_Format {
	char filename[150];		//name of the file
	char type;				//type of file
	time_t stamp;			//last modified time stamp
	int size;				//size of the file
} File_Format;

File_Format print[packetSize];
Hashing hashed_data[packetSize];
unsigned char tmpHash[MD5_DIGEST_LENGTH];

int str_to_int(char *s, int val ) {
	val = 0;
	int i;
	for( i=strlen(s)-1; i>=0; --i ) {
		val*=10;
		val+=s[i]-'0';
	}
	return val;
}

int handleList(int type, time_t begin, time_t end) {
	DIR *dir;
	index1 = 0; 
	struct dirent *entries;
	dir = opendir ("./");
	struct stat fs;

	if (dir != NULL) {
		while (entries = readdir(dir)) {
			if(stat(entries->d_name, &fs) < 0)
				return 1;
			else if( type == 0 ) {  //Long List
				strcpy(print[index1].filename, entries->d_name);
				//printf("name = %s\n", entries->d_name);
				print[index1].size = fs.st_size;
				print[index1].stamp = fs.st_mtime;
				print[index1].type = (S_ISDIR(fs.st_mode)) ? 'd' : '-';
				index1++;
			}
			else if( type == 1 && difftime(fs.st_mtime, begin) > 0 && difftime(end, fs.st_mtime) > 0 ) {  //Short List
				strcpy(print[index1].filename, entries->d_name);
				//printf("name = %s\n", entries->d_name);
				print[index1].size = fs.st_size;
				print[index1].stamp = fs.st_mtime;
				print[index1].type = (S_ISDIR(fs.st_mode)) ? 'd' : '-';
				index1++;
			}
		}
		//puts("dfdsfsd");
		closedir(dir);
	}
	else
		printf("Couldn't open the directory");
	return 0;
}

void md5Hash(FILE *fp) {
	FILE *fileCopy = fp;
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];

	if (fileCopy == NULL) {
		printf ("can't be opened.\n");
		return;
	}

	MD5_Init (&mdContext);
	while ((bytes = fread (data, 1, 1024, fileCopy)) != 0)
		MD5_Update (&mdContext, data, bytes);
	MD5_Final (tmpHash,&mdContext);
	fclose (fileCopy);
}

int regexCheck(char *expr) {
	index1 = 0;
	regex_t regex;
	DIR *dir;
	struct dirent *entries;
	dir = opendir ("./");
	struct stat fs;
	int reti = regcomp(&regex, expr, 0);
	if( reti ) {
		return 1;
	}
	if (dir != NULL) {
		while (entries = readdir(dir)) {
			if(stat(entries->d_name, &fs) < 0)
				return 1;
			if(!regexec(&regex, entries->d_name, 0, NULL, 0) ) {
				strcpy(print[index1].filename, entries->d_name);
				//printf("name = %s\n", entries->d_name);
				print[index1].size = fs.st_size;
				print[index1].stamp = fs.st_mtime;
				print[index1].type = (S_ISDIR(fs.st_mode)) ? 'd' : '-';
				index1++;
			}
		}
		//puts("dfdsfsd");
		closedir(dir);
	}
	else
		printf("Couldn't open the directory");
	return 0;
}

int getRequest(char *string) {
	char clone[100];
	strcpy(clone, string);
	char d[] = {' ', '\n'};
	char *s = NULL;
	s = strtok(clone, d);
	if( !(strcmp(s, "IndexGet")) ) 
		return IndexGet;
	if( !(strcmp(s, "FileHash")) ) 
		return FileHash;
	if( !(strcmp(s, "FileDownload")) ) 
		return FileDownload;
	if( !(strcmp(s, "FileUpload")) ) 
		return FileUpload;
	if( !(strcmp(s, "Quit")) ) 
		return Quit;
	if( !(strcmp(s, "History")) ) 
		return History;
	return -1;
}

char *indexGet(char *str) {
	index1 = 0;
	char *temp = (char *)malloc(1024 * sizeof(char));
	temp[0] = '\0';
	char delim[] = {' ', '\n'};
	int iter = 1;
	char *regex;

	char *arg = NULL;
	arg = strtok(str, delim);
	arg = strtok(NULL, delim);

	struct tm timer;
	time_t start, finish;

	if(arg == NULL) {
		err_flag = 1;
		sprintf(temp, "Error: Invalid Format.\nRefer to readme\n");
		return temp;
	}
	else {
		if(strcmp(arg, "--regex") == 0) {
			arg = strtok(NULL, delim);
			if(arg == NULL) {
				printf("Error: The correct format is:\nIndexGet --regex <regex>\n");
				_exit(1);
			}
			regex = arg;
			arg = strtok(NULL, delim);
			if(arg != NULL) {
				printf("Error: The correct format is:\nIndexGet RegEx <regex>\n");
				_exit(1);
			}
			regexCheck(regex);
			return temp;
		}
		else if(!strcmp(arg, "--longlist")) {
			arg = strtok(NULL, delim);

			if(arg != NULL) {
				err_flag = 1;
				sprintf(temp, "Error: Correct format:\nIndexGet --longlist\n");
				return temp;
			}
			else {
				//printf("lister\n");
				handleList(0, start, finish);				//0 handles long listing
				return temp;
			}
		}
		else if( !strcmp(arg, "--shortlist") ) {
			arg = strtok(NULL, delim);
			if(arg == NULL) {
				err_flag = 1;
				sprintf(temp,"Error: Wrong Format. The correct format is:\nIndexGet --shortlist <startTime> <endTime>\n");
				return temp;
			}
			while(arg != NULL) {
				if (strptime(arg, "%d-%b-%Y-%H:%M:%S", &timer) == NULL) {
					err_flag = 1;
					sprintf(temp,"Error: The correct format is:\nDate-Month-Year-hrs:min:sec\n");
					return temp;
				}
				if(iter >= 3) {
					err_flag = 1;
					sprintf(temp,"Error: The correct format is:\nIndexGet ShortList <timestamp1> <timestamp2>\n");
					return temp;
				}
				if(iter == 1)
					start = mktime(&timer);
				else
					finish = mktime(&timer);
				arg = strtok(NULL, delim);
				iter++;
			}
			handleList(1, start, finish);		//1 for short list
			return temp;
		}
		else {
			err_flag = 1;
			sprintf(temp, "Error: Wrong Format.\n");
			return temp;
		}
	}
}

char *fileDownloader(char *req) {
	char *temp = (char *)malloc(1024 * sizeof(char));
	temp[0] = '\0';
	char d[] = {' ','\n'};
	
	char *data = NULL;
	data = strtok(req,d);
	data = strtok(NULL,d);
	err_flag = -1;
	
	if(data == NULL) {
		err_flag = 1;
		sprintf(temp, "Error: The correct format is:\nFileDownload <file>\n");
		return temp;
	}
	
	strcpy(temp,data);
	data = strtok(NULL,d);
	
	if(data != NULL) {
		err_flag = 1;
		sprintf(temp, "Error: The correct format is:\nFileDownload <file>\n");
		return temp;
	}
	return temp;
}	

char *checkFile(char *file) {
	
	char *temp = (char *)malloc(1024*sizeof(char));
	temp[0] = '\0';
	unsigned char check[MD5_DIGEST_LENGTH];
	DIR *dp;
	struct dirent *entries;
	dp = opendir ("./");
	struct stat fs;

	index1 = 0;
	int i;
	
	if (dp != NULL) {
		while (entries = readdir (dp)) {
			if(stat(entries->d_name, &fs) < 0)
				return temp;
			else if( strcmp(file, entries->d_name ) == 0) {
				MD5_CTX mdContext;
				int bytes;
				unsigned char data[1024];
				char *filename = entries->d_name;
				strcpy(hashed_data[i].filename, entries->d_name);
				hashed_data[i].stamp = fs.st_mtime;
				FILE *infp = fopen (filename, "r");
				
				if (infp == NULL) {
					sprintf(temp,"%s can't be opened.\n", filename);
					return temp;
				}

				MD5_Init (&mdContext);
				while ((bytes = fread (data, 1, 1024, infp)) != 0)
					MD5_Update (&mdContext, data, bytes);
				MD5_Final (check, &mdContext);
				for(i = 0; i < MD5_DIGEST_LENGTH; i++)
					hashed_data[index1].hash[i] = check[i];
				fclose (infp);
				index1++;
			}
			else
				continue;
		}
	}
	else {
		err_flag= 1;
		sprintf(temp,"Couldn't open the directory");
	}
	return temp;
}

char *handleCheckAll()
{
	DIR *dp;
	index1 = 0;
	int i;
	struct dirent *entries;
	dp = opendir ("./");
	struct stat fs;
	char *temp = (char *)malloc(1024 * sizeof(temp));
	temp[0] = '\0';
	
	unsigned char check[MD5_DIGEST_LENGTH];
	
	if (dp != NULL) {
		while (entries = readdir(dp)) {
			if( stat(entries->d_name, &fs ) < 0)
				return temp;
			else {
				char *filename = entries->d_name;
				strcpy( hashed_data[index1].filename, entries->d_name );
				//printf("file = %s\n", hashed_data[index1].filename);
				hashed_data[index1].stamp = fs.st_mtime;
				FILE *inFile = fopen (filename, "r");
				MD5_CTX mdContext;
				int bytes;
				unsigned char data[1024];

				if (inFile == NULL) {
					err_flag = 1;
					sprintf (temp,"%s can't be opened.\n", filename);
					return temp;
				}

				MD5_Init (&mdContext);
				while ((bytes = fread (data, 1, 1024, inFile)) != 0)
					MD5_Update (&mdContext, data, bytes);
				MD5_Final (check,&mdContext);
				for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
					hashed_data[index1].hash[i] = check[i];
				}
				//printf("hash = %02x\n", hashed_data[index1].hash);
				fclose (inFile);
				index1++;
			}
		}
	}
	else {
		err_flag = 1;
		sprintf(temp,"Couldn't open the directory");
	}
	return temp;
}


char *FileHash_handler(char *req) {
	char *req_data = NULL;
	char delim[] = " \n";
	req_data = strtok(req,delim);
	req_data = strtok(NULL,delim);
	char *temp = (char *)malloc(1024 * sizeof(char));
	temp[0] = '\0';
	if(req_data == NULL) {
		err_flag = 1;
		sprintf(temp,"ERROR: Wrong Format\n");
	}
	while(req_data != NULL) {
		if(strcmp(req_data,"--verify") == 0) {
			req_data = strtok(NULL,delim);
			if(req_data == NULL) {
				err_flag = 1;
				sprintf(temp,"ERROR: Wrong Format. The correct format is:\nFileHash Verify <filename>\n");
				return temp;
			}
			char *filename = req_data;
			req_data = strtok(NULL,delim);
			if(req_data != NULL) {
				err_flag = 1;
				sprintf(temp,"ERROR: Wrong Format. The correct format is:\nFileHash Verify <filename>\n");
				return temp;
			}
			else
				strcat( temp, checkFile(filename));
		}
		else if(strcmp(req_data,"--checkall") == 0) {
			req_data = strtok(NULL,delim);

			if(req_data != NULL) {
				err_flag = 1;
				sprintf(temp,"ERROR: Wrong Format. The correct format is:\nFileHash CheckAll\n");
				return;
			}
			else
				strcat( temp, handleCheckAll() );
		}
	}
	return temp;
}


int tcpServer(int port) {	
	printf("\nTCP server Start\n");
	struct sockaddr_in addr;
	char *temp1 = (char *)malloc(1024 * sizeof(char));
	temp1[0] = '\0';
	int i, j, k, buff, listenfd=0, connectfd=0;

	/*creating array of 1024 initialized to 0*/
	char input[1024], output[1024];
	memset(input, 0, sizeof(input));
	memset(output, 0, sizeof(output));
	//clearing addr
	memset(&addr, 0, sizeof(addr));

	listenfd = socket(AF_INET, SOCK_STREAM, 0);	//create socket
	if( listenfd == -1 ) {
		printf("Error: Socket Creation unsuccessful\n");
		exit(0);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port); 
	
	int a;
	a = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
	if( a == -1 ) {
		printf("%s\n", strerror(errno));
		printf("\nError: Binding Unseccessful\n");
		exit(0);
	}
	
	listen(listenfd, 10);
	connectfd = accept(listenfd, (struct sockaddr*)NULL, NULL);


	buff = read(connectfd, input, sizeof(input));	
	//puts("poop");

	while( buff > 0 ) {
		printf("\nEnter Command Here: ");
		int type = 0;
		char *request = (char *)malloc(strlen(input)+1);
		strcpy(request, input);
		strcpy(history[counter++], input);
		type = getRequest(request);
		
		/************* This is a test **************/
		//printf("command = %s, type = %d\n", request, type);
		//printf("command = %s, type = %d\n", request, type);
		output[0] = '\0';
		if( type < 0 ) {		//We have an Error
			write(connectfd , "\nError: Invalid command\n" , 23);
			strcat(output,"~@~");
			memset(output, 0, sizeof(output));
			memset(input, 0, sizeof(input)); 
			while( (buff = read(connectfd, input, sizeof(input))) <= 0 )
				;
			continue;
		}
		else if( type == FileDownload ) {
			//Download File Code
			temp1 = fileDownloader(request);
			puts(temp1);
			int i;
			char temp[packetSize];
			if(err_flag != -1)
			{
				printf("%d\n",err_flag);
				strcat( output, temp1 );
				err_flag = -1;
			}
			else
			{
				FILE* fp;
				fp = fopen(temp1,"rb");
				md5Hash(fp);
				fp = fopen(temp1,"rb");
				size_t bytes_read;
				memcpy(output,tmpHash,MD5_DIGEST_LENGTH);
				write(connectfd , output , MD5_DIGEST_LENGTH);
				memset(output, 0, sizeof(output));
				while(!feof(fp))
				{
					bytes_read = fread(temp, 1, 1023, fp);
					memcpy(output,temp,bytes_read);
					write(connectfd , output , bytes_read);
					memset(output, 0, sizeof(output));
					memset(temp, 0, sizeof(temp));
				}
				memcpy(output,"~@~",3); 
				write(connectfd , output , 3);
				memset(output, 0, sizeof(output));
				fclose(fp);
			}
		}
		else if( type == FileHash ) {
			strcat( output, FileHash_handler(request) );
			int i;
			char temp[packetSize];
			for (i = 0 ; i < index1 ; i++)
			{
				sprintf(temp, "%-25s ", hashed_data[i].filename);
				strcat(output, temp);
				sprintf(temp, "%02x", hashed_data[i].hash);
				strcat(output, temp);
				sprintf(temp, "\t %s",ctime(&hashed_data[i].stamp));
				strcat(output, temp);
			}
			strcat(output,"~@~");
			write(connectfd, output , strlen(output));
			output[0] = '\0';
		
		}
		else if( type == IndexGet ) {
			strcat( output, indexGet(request) );
			//puts("popo");
			int i;
			char temp[packetSize];
			for(i = 0 ; i < index1 ; i++)
			{
			//	puts("sdfdsf");
				sprintf(temp, "%-25s  %10d  %-3c  %s" , print[i].filename , print[i].size , print[i].type , ctime(&print[i].stamp));
				strcpy(output, temp);
				output[strlen(output)] = '\0';
				if(i!=index1-1)
				{
					write(connectfd , output , strlen(output));
					memset(output,0,sizeof(output));
				}
				//printf("out = %s\n", print[i].type);
			}
			strcat(output, "~@~");
			write(connectfd, output, strlen(output));
			output[0] = '\0';
		}
		else if(type == FileUpload)
		{
			FILE *fp1;
			fp1 = fopen("permission","r");
			char perm[100];
			fscanf(fp1, "%s", perm);
			if(strcmp(perm,"allow")!=0) {
				printf("\nFileUpload Rejected\n");
				memcpy(output,"FileUpload Deny\n",16);
				write(connectfd , output , 16);
			}
			else {
				printf("\nFileUpload Accepted\n");
				memcpy(output,"FileUpload Accept\n",18);
				write(connectfd , output , 18);
				memset(output, 0,18);
				char clone[1024];
				memset(clone, 0,1024);
				memcpy(clone,request,1024);
				char *size = strtok(clone,"\n");
				size = strtok(NULL,"\n");
				long fsize = str_to_int(size, fsize);
				char *request_data = NULL;
				const char delim[] = " \n";
				request_data = strtok(request,delim);
				request_data = strtok(NULL,delim);
				int file = open(request_data, O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600);
				if (file == -1) {
					//perror("Error opening file for writing:");
					return 1;
				}
				int result = lseek(file,fsize-1, SEEK_SET);
				result = write(file, "", 1);
				if (result < 0) {
					close(file);
					//perror("Error opening file for writing:");
					return 1;
				}
				close(file);
				FILE *fp;
				fp = fopen(request_data,"wb");
				buff = read(connectfd, input, sizeof(input));
				puts(input);
				memset(input,0,buff);
				buff = read(connectfd, input, sizeof(input)-1);	
				while(1)
				{
					input[buff] = 0;
					if(input[buff-1] == '~' && input[buff-3] == '~' && input[buff-2] == '@')
					{
						input[buff-3] = 0;
						fwrite(input,1,buff-3,fp);
						fclose(fp);
						memset(input, 0,buff-3);
						break;
					}
					else
					{
						fwrite(input,1,buff,fp);
						memset(input, 0, buff);
					}
					buff = read(connectfd, input, sizeof(input)-1);
					if(buff < 0)
						break;
				}
			}
		}
		memset(output, 0,1024);
		memset(input, 0 , sizeof(input));
		while((buff = read(connectfd, input,sizeof(input)))<=0)
			;
	}
	return 0;
}

int tcpClient(char *ip, int port) {
	int sock = 0;
	struct sockaddr_in addr;
	char input[1024], output[1024];
	int i, j, k, buff, listenfd, connectfd;
	char downloadName[1024], uploadName[1024];
	/*creating array of 1024 initialized to 0*/
	memset(input, 0, sizeof(input));
	memset(output, 0, sizeof(output));
	
	//clearing addr
	memset(&addr, 0, sizeof(addr));

	sock = socket(AF_INET, SOCK_STREAM, 0);	//create socket

	if( sock < 0 ) {
		printf("\nError: Socket Creation unsuccessful\n");
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if(inet_pton(AF_INET, ip, &addr.sin_addr)<=0)
	{
		puts("inet_pton error");
		return 1;
	}
	while(1) {
		if( connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			continue;
		else {
			if(client)
				printf("\nClient Connection Success\n");
			break;
		}
	}
	
	int down_flag = 0, up_flag = 0, n = 0;
	
	while(1)
	{
		printf("\nEnter Command: ");
		down_flag = 0;
		up_flag = 0;
		FILE *fp = NULL;
		int i;
		char *response = (char *)malloc(packetSize);
		unsigned char md5Check[MD5_DIGEST_LENGTH];
		memset(output,0,sizeof(output));
		fgets(output, sizeof(output), stdin);
		char *filename = (char *)malloc(1024);
		char clone[1024];
mygoto:
		strcpy(clone, output);

		filename = strtok(clone, " \n");
		if(strcmp(filename, "quit") == 0)
			_exit(1);
		if(strcmp(filename, "FileDownload") == 0)
		{
			down_flag = 1;
			filename = strtok(NULL," \n");
			strcpy(downloadName, filename);
			fp = fopen(downloadName,"wb");
		}
		int fail = 0;
		if(strcmp(filename,"FileUpload") == 0)
		{
			up_flag = 1;
			filename = strtok(NULL," \n");
			if( filename == NULL ) {
				strcat(output, "Error: No File given to Upload");
				fail = 1;
			}
			else {
				strcpy(uploadName, filename);
				FILE *file = fopen(uploadName, "r");
				fseek(file, 0, SEEK_END);
				unsigned long length = (unsigned long)ftell(file);
				char len[1024];
				memset(len, 0, sizeof(len));
				sprintf(len, "%ld\n", length);
				strcat(output, len);
				fclose(file);
			}
		}
	//	puts("arrey");
		write(sock, output , strlen(output));
	//	puts(output);
 		//if(fail)
 		//	continue;
		n = read(sock, input, sizeof(input) - 1);
		size_t readBytes;
		//int check_md5_flag = 0;
		if(strcmp(input, "FileUpload Accept\n") == 0)
		{
			int j, k;
			printf("\nUpload Accepted\n");
			
			strcat( output, checkFile(uploadName) );
			for (j = 0 ; j < 1 ; j++)
			{
				sprintf(response, "%s, ",hashed_data[j].filename);
				strcat(output, response);
				for (k = 0; k < MD5_DIGEST_LENGTH; k++) {
					sprintf(response, "%02x", hashed_data[j].hash[k]);
					strcat(output,response);
				}
				sprintf(response, ", %s",ctime(&hashed_data[j].stamp));
				strcat(output,response);
			}
			write(sock , output , sizeof(output));
			//printf("%s\n",output);
			memset(output, 0, sizeof(output));
			fp = fopen(uploadName, "rb");
	
			while( !feof(fp) )
			{
				readBytes = fread(response, 1, 1024, fp);
				response[1024] = 0;
				memcpy(output,response,readBytes);
				write(sock , output , readBytes);
				memset(output, 0, sizeof(output));
				memset(response, 0, sizeof(response));
			}
			memcpy(output,"~@~",3);
			write(sock , output , 3);
			memset(output, 0, sizeof(output));
			memset(input, 0, strlen(input));
			fclose(fp);
		}
		else if(strcmp(input, "FileUpload Failed\n") == 0)
		{
			puts("File Upload Failed, Try again.");
			memset(input, 0,sizeof(input));
			continue;			
		}
		else if(strcmp(input, "FileUpload Deny\n") == 0)
		{
			printf("\nUpload Denied\n");
			memset(input, 0,sizeof(input));
			continue;
		}
		else
		{
			if(down_flag == 1)
			{
				memcpy(md5Check,input,MD5_DIGEST_LENGTH);
				n = read(sock, input, MD5_DIGEST_LENGTH);
			}
			while(1)
			{
				input[n] = 0;
				if(input[n-1] == '~' && input[n-3] == '~' && input[n-2] == '@')
				{
					input[n-3] = 0;
					if(down_flag == 1)
					{
						fwrite(input,1,n-3,fp);
						fclose(fp);
						fp = fopen(downloadName,"rb");
						md5Hash(fp);
						int j;
						for(j=0;j<MD5_DIGEST_LENGTH;j++)
						{
							if(md5Check[j] != tmpHash[j])
							{
								puts("md5 did not match, trying again......");
								goto mygoto;
							}
						}
					}
					else
						strcat(response,input);
					memset(input, 0,strlen(input));
					break;
				}
				else
				{
					if(down_flag == 1)
						fwrite(input,1,n,fp);
					else
						strcat(response,input);
					memset(input, 0,strlen(input));
				}
				n = read(sock, input, sizeof(input)-1);
				if(n < 0)
					break;
			}
		}

		if(down_flag == 0)
			printf("%s\n",response);
		else 
			printf("\nFile Downloaded\n");

		if(n < 0)
			printf("\n Read error \n");
		memset(input, 0,sizeof(input));
		memset(output, 0,sizeof(output));
	}
	return 0;
}

int udpServer(int port) {	
	printf("\nUDP server start\n");
	struct sockaddr_in addr, addr1;
	char input[1024], output[1024];
	char *temp1 = (char *)malloc(1024 * sizeof(char));
	temp1[0] = '\0';
	int i, j, k, buff, listenfd=0;

	listenfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
	/*creating array of 1024 initialized to 0*/
	memset(input, 0, sizeof(input));
	memset(output, 0, sizeof(output));
	//clearing addr
	memset(&addr, 0, sizeof(addr));

	if( listenfd == -1 ) {
		printf("\nError: Socket Creation unsuccessful\n");
		exit(0);

	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port); 
	
	//int a;
	bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
	/*if( a == -1 ) {
		printf("%s\n", strerror(errno));
		printf("Error: Binding Unseccessful\n");
		exit(0);
	}*/

	//buff = read(listenfd, input, sizeof(input));	
	int addr1_len = sizeof(struct sockaddr);
	buff = recvfrom(listenfd,input,1024,0,(struct sockaddr *)&addr1, &addr1_len);
	//sendto(listenfd,output,strlen(output),0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
	while( buff > 0 ) {
		printf("\nEnter Command Here: ");
		int type = 0;
		char *request = (char *)malloc(strlen(input)+1);
		strcpy(request, input);
		strcpy(history[counter++], input);
		type = getRequest(request);
		
		/************* This is a test **************/
		//printf("command = %s, type = %d\n", request, type);
		//printf("command = %s, type = %d\n", request, type);
		output[0] = '\0';
		if( type < 0 ) {		//We have an Error
			//write(listenfd , "Error: Invalid command\n" , 23);
			sendto(listenfd,"Error: Invalid command\n",23,0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
			strcat(output,"~@~");
			memset(output, 0, sizeof(output));
			memset(input, 0, sizeof(input)); 
			while( (buff = recvfrom(listenfd,input,1024,0,(struct sockaddr *)&addr1, &addr1_len)) <= 0 )
				;
			continue;
		}
		else if( type == FileDownload ) {
			//Download File Code
			temp1 = fileDownloader(request);
			//puts(temp1);
		}
		else if( type == FileHash ) {
			strcat( output, FileHash_handler(request) );
		}
		else if( type == IndexGet ) {
			//puts("popo");

			strcat( output, indexGet(request) );
		}
		if(type == IndexGet )
		{
			//puts("popo");
			int i;
			char temp[packetSize];
			for(i = 0 ; i < index1 ; i++)
			{
				//puts("sdfdsf");
				sprintf(temp, "%-25s  %10d  %-3c  %s" , print[i].filename , print[i].size , print[i].type , ctime(&print[i].stamp));
				strcpy(output, temp);
				output[strlen(output)] = '\0';
				if(i!=index1-1)
				{
					//write(listenfd , output , strlen(output));
					sendto(listenfd,output,strlen(output),0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
					memset(output,0,sizeof(output));
				}
				//printf("out = %s\n", print[i].type);
			}
			strcat(output,"~@~");
			//write(listenfd , output , strlen(output));
			sendto(listenfd,output,strlen(output),0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
			output[0] = '\0';
		}
		else if(type == FileHash)
		{
			int i;
			char temp[packetSize];
			for (i = 0 ; i < index1 ; i++)
			{
				sprintf(temp, "%-25s ", hashed_data[i].filename);
				strcat(output, temp);
				sprintf(temp, "%02x", hashed_data[i].hash);
				strcat(output, temp);
				sprintf(temp, "\t %s",ctime(&hashed_data[i].stamp));
				strcat(output, temp);
			}
			strcat(output,"~@~");
			//write(listenfd, output , strlen(output));
			sendto(listenfd,output,strlen(output),0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
			output[0] = '\0';
		}
		else if(type == FileDownload )
		{
			int i;
			char temp[packetSize];
			if(err_flag != -1)
			{
				printf("%d\n",err_flag);
				strcat( output, temp1 );
				err_flag = -1;
			}
			else
			{
				FILE* fp;
	            fp = fopen(temp1,"rb");
	            md5Hash(fp);
	            fp = fopen(temp1,"rb");
	            size_t bytes_read;
	            memcpy(output,tmpHash,MD5_DIGEST_LENGTH);
	            //write(listenfd , output , MD5_DIGEST_LENGTH);
	            sendto(listenfd,output,MD5_DIGEST_LENGTH,0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
	            memset(output, 0, sizeof(output));
	            while(!feof(fp))
	            {
	                bytes_read = fread(temp, 1, 1023, fp);
	                memcpy(output,temp,bytes_read);
	                //write(listenfd , output , bytes_read);
	                sendto(listenfd,output,strlen(output),0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
	                memset(output, 0, sizeof(output));
	                memset(temp, 0, sizeof(temp));
	            }
	            memcpy(output,"~@~",3); 
	            //write(listenfd , output , 3);
	            sendto(listenfd,output,3,0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
	            memset(output, 0, sizeof(output));
	            fclose(fp);
			}
		}
		else if(type == FileUpload)
        {	
        	FILE *fp1;
			fp1 = fopen("permission","r");
			char perm[100];
			fscanf(fp1, "%s", perm);
			if(strcmp(perm,"allow")!=0) {
				printf("\nFileUpload Rejected\n");
				memcpy(output,"FileUpload Deny\n",16);
	            sendto(listenfd,output,16,0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
			}
			else {
	            printf("\nFileUpload Accepted\n");
	            memcpy(output,"FileUpload Accept\n",18);
	            //write(listenfd , output , 18);
	            sendto(listenfd,output,18,0,(struct sockaddr *)&addr1,sizeof(struct sockaddr));
	            memset(output, 0,18);
	            char clone[1024];
	            memset(clone, 0,1024);
	            memcpy(clone,request,1024);
	            char *size = strtok(clone,"\n");
	            size = strtok(NULL,"\n");
	            long fsize = str_to_int(size, fsize);
	            char *request_data = NULL;
	            const char delim[] = " \n";
	            request_data = strtok(request,delim);
	            request_data = strtok(NULL,delim);
	            int file = open(request_data, O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600);
	            if (file == -1) {
	                //perror("Error opening file for writing:");
	                return 1;
	            }
	            int result = lseek(file,fsize-1, SEEK_SET);
	            result = write(file, "", 1);
	            if (result < 0) {
	                close(file);
	                //perror("Error opening file for writing:");
	                return 1;
	            }
	            close(file);
	            FILE *fp;
	            fp = fopen(request_data,"wb");
	            buff = recvfrom(listenfd,input,sizeof(input)-1,0,(struct sockaddr *)&addr1, &addr1_len);
	            puts(input);
	            memset(input,0,sizeof(input));
	            buff = recvfrom(listenfd,input,sizeof(input)-1,0,(struct sockaddr *)&addr1, &addr1_len);
	            while(1)
	            {
	                input[buff] = 0;
	                if(input[buff-1] == '~' && input[buff-3] == '~' && input[buff-2] == '@')
	                {
	                    input[buff-3] = 0;
	                    fwrite(input,1,buff-3,fp);
	                    fclose(fp);
	                    memset(input, 0,buff-3);
	                    break;
	                }
	                else
	                {
	                    fwrite(input,1,buff,fp);
	                    memset(input, 0, buff);
	                }
	                buff = recvfrom(listenfd,input,sizeof(input)-1,0,(struct sockaddr *)&addr1, &addr1_len);
	                if(buff < 0)
	                    break;
	            }
	        }
        }
        memset(output, 0,1024);
		memset(input, 0 , sizeof(input));
		while((buff = recvfrom(listenfd,input,sizeof(input),0,(struct sockaddr *)&addr1, &addr1_len))<=0)
			;
	}
	close(listenfd);
    wait(NULL);
	return 0;
}

int udpClient(char *ip, int port) {
	
	int sock = 0;
	struct sockaddr_in addr;
	char input[1024], output[1024];
	int i, j, k, buff;
	char downloadName[1024], uploadName[1024];
	/*creating array of 1024 initialized to 0*/
	memset(input, 0, sizeof(input));
	memset(output, 0, sizeof(output));
	
	//clearing addr
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);	//create socket

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if(inet_pton(AF_INET, ip, &addr.sin_addr)<=0)
	{
		puts("inet pton error occured");
		return 1;
	}
	/*while(1) {
		if( connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			continue;
		else {
			if(client)
				printf("Client Connection Success\n");
			break;
		}
	}*/
	
	int down_flag = 0, up_flag = 0, n = 0;
	int addr_len = sizeof(struct sockaddr);
	//sendto(sock, output, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));


	while(1)
	{
		printf("\nEnter Command Here: ");
		
		//printf("Enter Command: ");
		down_flag = 0;
		up_flag = 0;
		FILE *fp = NULL;
		int i;
		char *response = (char *)malloc(packetSize);
		unsigned char md5Check[MD5_DIGEST_LENGTH];
		memset(output,0,sizeof(output));
		fgets(output, sizeof(output), stdin);
		char *filename = (char *)malloc(1024);
		char clone[1024];
		strcpy(clone, output);

		filename = strtok(clone, " \n");
		//if(strcmp(filename, "quit") == 0)
		//	_exit(1);
		if(strcmp(filename, "FileDownload") == 0)
		{
			down_flag = 1;
			filename = strtok(NULL," \n");
			strcpy(downloadName, filename);
			fp = fopen(downloadName,"wb");
		}
		//int fail = 0;
		if(strcmp(filename,"FileUpload") == 0)
		{
			up_flag = 1;
			filename = strtok(NULL," \n");
			if( filename == NULL ) {
				strcat(output, "Error: No File given to Upload");
		//		fail = 1;
			}
			else
			{
				strcpy(uploadName, filename);
				FILE *file = fopen(uploadName, "r");
				fseek(file, 0, SEEK_END);
				unsigned long length = (unsigned long)ftell(file);
				char len[1024];
				memset(len, 0, sizeof(len));
				sprintf(len, "%ld\n", length);
				strcat(output, len);
				fclose(file);
			}
		}
		//puts("arrey");
		//write(sock, output , strlen(output));
		sendto(sock, output, strlen(output), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
		//puts(output);
 		//if(fail)
 		//	continue;
		//n = read(sock, input, sizeof(input) - 1);
		n = recvfrom(sock,input,sizeof(input) - 1,0,(struct sockaddr *)&addr, &addr_len);
		size_t readBytes;
		//int check_md5_flag = 0;
		if(strcmp(input, "FileUpload Accept\n") == 0)
		{
			int j, k;
			printf("\nUpload Accepted\n");
			
			strcat( output, checkFile(uploadName) );
			for (j = 0 ; j < 1 ; j++)
			{
				sprintf(response, "%s, ",hashed_data[j].filename);
				strcat(output, response);
				for (k = 0; k < MD5_DIGEST_LENGTH; k++) {
					sprintf(response, "%02x", hashed_data[j].hash[k]);
					strcat(output,response);
				}
				sprintf(response, ", %s",ctime(&hashed_data[j].stamp));
				strcat(output,response);
			}
			//write(sock , output , sizeof(output));
			sendto(sock, output, strlen(output), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
			printf("%s\n",output);
			memset(output, 0, sizeof(output));
			fp = fopen(uploadName, "rb");
	
			while( !feof(fp) )
			{
				readBytes = fread(response, 1, 1024, fp);
				response[1024] = 0;
				memcpy(output,response,readBytes);
				//write(sock , output , readBytes);
				sendto(sock, output, readBytes, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
				memset(output, 0, sizeof(output));
				memset(response, 0, sizeof(response));
			}
			memcpy(output,"~@~",3);
			//write(sock , output , 3);
			sendto(sock, output, 3, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
			memset(output, 0, sizeof(output));
			memset(input, 0, strlen(input));
			fclose(fp);
		}
		else if(strcmp(input, "FileUpload Failed\n") == 0)
		{
			puts("File Upload Failed, Try again.");
			memset(input, 0,sizeof(input));
			continue;			
		}
		else if(strcmp(input, "FileUpload Deny\n") == 0)
		{
			printf("\nUpload Denied\n");
			memset(input, 0,sizeof(input));
			continue;
		}
		else
		{
			if(down_flag == 1)
			{
				memcpy(md5Check,input,MD5_DIGEST_LENGTH);
				//n = read(sock, input, MD5_DIGEST_LENGTH);
				n = recvfrom(sock,input,MD5_DIGEST_LENGTH,0,(struct sockaddr *)&addr, &addr_len);
			}
			while(1)
			{
				input[n] = 0;
				if(input[n-1] == '~' && input[n-3] == '~' && input[n-2] == '@')
				{
					input[n-3] = 0;
					if(down_flag == 1)
					{
						fwrite(input,1,n-3,fp);
						fclose(fp);
						fp = fopen(downloadName,"rb");
						md5Hash(fp);
						int j;
						for(j=0;j<MD5_DIGEST_LENGTH;j++)
						{
							if(md5Check[j] != tmpHash[j])
							{
								puts("md5 did not match, try again......");
								goto mygoto;
							}
						}
					}
					else
						strcat(response,input);
					memset(input, 0,strlen(input));
					break;
				}
				else
				{
					if(down_flag == 1)
						fwrite(input,1,n,fp);
					else
						strcat(response,input);
					memset(input, 0,strlen(input));
				}
				//n = read(sock, input, sizeof(input)-1);
				n = recvfrom(sock,input,sizeof(input)-1,0,(struct sockaddr *)&addr, &addr_len);
				
				if(n < 0)
					break;
			}
		}

		if(down_flag == 0)
			printf("%s\n",response);
		else 
			printf("\nFile Downloaded\n");
mygoto:
		if(n < 0)
			printf("\n Read error \n");
		memset(input, 0,sizeof(input));
		memset(output, 0,sizeof(output));
	}
	return 0;
}


int main( int argc, char *argv[] ) {
	int i, j, k, lis_port, con_port;
	char *ip, *listen_port, *connect_port;
	if( argc != 5 ) {
		printf("Format:-   %s listen_port server_ip connect_port protocol(tcp/udp)\n", argv[0]);
		return 0;
	}

	//converting ip and ports to integer
	listen_port = argv[1];
	ip = argv[2];
	connect_port = argv[3];
	
	lis_port = str_to_int(listen_port, lis_port);
	con_port = str_to_int(connect_port, con_port);

	if( !strcmp("tcp", argv[4]) )
		client = 1;

	int pid = fork();
	if( pid < 0 ) {
		printf("Error:\nUnable to fork\n");
		return 0;
	}

	// setting up server
	if( pid == 0 ) {
		if( client ) {
			tcpServer(lis_port);
		}
		else {
			udpServer(lis_port);
			//udp
		}
	}
	
	//setting up client
	else {
		if( client ) {
			tcpClient(ip, con_port);
		}
		else {
			udpClient(ip, con_port);
			//udp
		}
	}
	return 0;
}