/*
 * Simple disk I/O generator
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define SECTOR_SIZE	512
#define DISK_SZ		(4096 * 1024)
#define N_ACCESSES	50
#define N_FORKS 	10

int main()
{
	int ret, fd, pid, i, j;
	unsigned int pos;
	char buf[SECTOR_SIZE];

	printf("Starting sector read example...\n");

	/* limpa buffers e caches de disco */
	printf("Cleaning disk cache...\n");
	system("echo 3 > /proc/sys/vm/drop_caches"); 

	/* a) desabilita merges de i/o
	 * b) define 4Kb para o tamanho máximo de uma requisição 
	 * c) evita que o sistema faça a leitura de mais conteúdo do que o que foi requisitado
	 */
	printf("Configuring scheduling queues...\n");
	system("echo 2 > /sys/block/sda/queue/nomerges");
	system("echo 4 > /sys/block/sda/queue/max_sectors_kb");
	system("echo 0 > /sys/block/sda/queue/read_ahead_kb");

	srand(getpid());

	fd = open("/dev/sda", O_RDWR);
	
	if (fd < 0){
		perror("Failed to open the device...");
		return errno;
	}

	strcpy(buf, "C-Scan!");

	for(i = 0; i < N_FORKS; i++){
		pid = fork();
	
		if(pid < 0){
			perror("Failed to fork...");
			return errno;
		}

		if(pid == 0) {
			// Child process
			srand(getpid());

			for(j = 0; j < N_ACCESSES; j++) {
				// Random position
				pos = (rand() % (DISK_SZ / SECTOR_SIZE));
				//pid e bloco
				printf("%d, %d\n", getpid(), pos);
				
				/* Set position */
				lseek(fd, pos * SECTOR_SIZE, SEEK_SET);
				
				/* Peform read. */
				read(fd, buf, SECTOR_SIZE);
			}
			close(fd);
			exit(0);
		}
	}

	// Wait for all children to finish
	while(wait(NULL) > 0);

	// Close the father's file descriptor
	close(fd);

	printf("Finished sector read example...\n");
	return 0;
}