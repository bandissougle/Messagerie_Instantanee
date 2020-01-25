#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>


#define HELLO_PORT 12345
#define HELLO_GROUP "225.0.0.37"
#define MSGBUFSIZE 256
//----------------------------
//For chat code, les commandes
#define CHAT_TO_GROUP_CODE "zz1"
#define CHAT_TO_ONE_CODE "zz2"
#define GET_LIST_CODE "zz3"
#define KEEP_ALIVE_CODE "zz4"
#define QUIT_CODE "zz7"

#define MSG_SEPARER_CODE ":"

#define STATUS_OFFLINE "zz8"
#define STATUS_ONLINE "zz9"
//----------------------------
char userName[30] = "group1";
//liste d'utilisateur
char listUser[30][30];
int nombre = 0;

pthread_t *thread_id;

struct sockaddr_in addr1;
char msgSend[MSGBUFSIZE] = "";
char msgIn[MSGBUFSIZE] = "";
char msgAlive[MSGBUFSIZE] = "";

struct sockaddr_in addr;
int fd, nbytes, addrlen;
int fd1;
struct ip_mreq mreq;
char msgbuf[MSGBUFSIZE];
u_int yes = 1; /*** MODIFICATION TO ORIGINAL */
unsigned char ttl = 32;

//les fonctions
void *recvM(void * arg);
void *sendM(void * arg);
void *keepAlive(void * arg);
void *setZero(void * arg);
//Traiter les commandes, messages
int msgSendProcess(char *msg);
void msgRcvProcess(char *msg);
void strnCut(char *strDst, char *strSrc, int n);
void strcCut(char *strDst, char *strSrc, char c);
void strcCut2(char *strDst, char *strSrc, char c);

//Ajouter un utilisateur
void addUser(char *user);
//Delete un utilisateur
void delUser(char *user);
//Print list user
void printUser();
///send un message
void sendMsg();
//afficher le guide
void printGuide();

int main(int argc, char *argv[]) {
	strcpy(userName, argv[1]);
	/* create what looks like an ordinary UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return 1;
	}
	/**** MODIFICATION TO ORIGINAL */
	/* allow multiple sockets to use the same PORT number */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("Reusing ADDR failed");
		return 1;
	}
	/*** END OF MODIFICATION TO ORIGINAL */
	/* set up TTL */
	setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	/* set up destination address */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY ); /* N.B.: differs from sender */
	addr.sin_port = htons(HELLO_PORT);

	/* bind to receive address */
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	/* use setsockopt() to request that the kernel join a multicast group */
	mreq.imr_multiaddr.s_addr = inet_addr(HELLO_GROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY );
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))
			< 0) {
		perror("setsockopt");
		return 1;
	}
	//-----------------------------------------------------------------------------
	//struct sockaddr_in addr1;
	/* create what looks like an ordinary UDP socket */
	if ((fd1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return 1;
	}
	/* set up destination address */
	memset(&addr1, 0, sizeof(addr1));
	addr1.sin_family = AF_INET;
	addr1.sin_addr.s_addr = inet_addr(HELLO_GROUP);
	addr1.sin_port = htons(HELLO_PORT);
	//-----------------------------------------------------------------------------
	/* set up TTL */
	setsockopt(fd1, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
//------------------------------------------------------------------------------------
	//Afficher les commandes
	printGuide();	
	// Create thread for chat
	thread_id = (pthread_t *) malloc(3 * sizeof(pthread_t));

	// Pour text
	pthread_create(&thread_id[0], NULL, recvM, NULL);
	pthread_create(&thread_id[1], NULL, sendM, NULL);
	pthread_create(&thread_id[2], NULL, keepAlive, NULL);
	pthread_create(&thread_id[3], NULL, setZero, NULL);

	pthread_join(thread_id[0], NULL);
	pthread_join(thread_id[1], NULL);
	pthread_join(thread_id[2], NULL);
	pthread_join(thread_id[3], NULL);
//------------------------------------------------------------------------------------
}

//receiver Multicast
void *recvM(void * arg) {
	for (;;) {

		if ((nbytes = recv(fd, msgbuf, MSGBUFSIZE, 0)) < 0) {
			perror("recvfrom");
		}
		msgRcvProcess(msgbuf);
	}
	pthread_exit(0);
}

//sender Multicast
void *sendM(void * arg) {
	for (;;) {
		//printf("%s:",userName);
		gets(msgIn);
		int a = msgSendProcess(msgIn);
		if (a == 0) {
			if (sendto(fd1, msgSend, MSGBUFSIZE, 0, (struct sockaddr *) &addr1,
					sizeof(addr1)) < 0) {
				perror("sendto");
			}
		}
		else if(a == 2){
			if (sendto(fd1, msgSend, MSGBUFSIZE, 0, (struct sockaddr *) &addr1,
					sizeof(addr1)) < 0) {
				perror("sendto");
			}
			exit(0);
		}
	}
	pthread_exit(0);
}
//send un message dans le parametre
void sendMsg() {
	if (sendto(fd1, msgSend, MSGBUFSIZE, 0, (struct sockaddr *) &addr1,
					sizeof(addr1)) < 0) {
				perror("sendto");
			}
	
}
//sender Multicast
void *keepAlive(void * arg) {
	for (;;) {
		strcpy(msgAlive, "");
		strcpy(msgAlive, KEEP_ALIVE_CODE);
		strcat(msgAlive, userName);
		if (sendto(fd1, msgAlive, MSGBUFSIZE, 0, (struct sockaddr *) &addr1,
				sizeof(addr1)) < 0) {
			perror("sendto");
		}
		sleep(5);
		//set nombre des utilisateur =0
		//nombre = 0;
	}
	pthread_exit(0);
}
//Reset les onlines
void *setZero(void * arg){
	for (;;) {
		sleep(31);
		nombre=0;
	}
}

//Traiter les commandes
int msgSendProcess(char *msg) {
	char tmp[MSGBUFSIZE] = "";
	char rcveur[MSGBUFSIZE] = "";
	if (strncmp(msg, "/chatlist", 9) == 0) {
		//printf("\nChat avec tout le monde");
		//cut la tête
		strnCut(tmp, msg, 9);
		strcpy(msg, tmp);
		//Former le message pour envoyer
		strcpy(msgSend, "");
		strcpy(msgSend, CHAT_TO_GROUP_CODE);
		strcat(msgSend, userName);
		strcat(msgSend, ":");
		strcat(msgSend, msg);
		return 0;
	} else if (strncmp(msg, "/chatone:", 9) == 0) {
		//printf("\nChat avec une personne");
		strnCut(tmp, msg, 9);
		strcpy(msg, tmp);
		//Chercher qui est envoye
		strcCut(rcveur, msg, ':');
		//Former le message pour envoyer
		strcpy(msgSend, "");
		strcpy(msgSend, CHAT_TO_ONE_CODE);
		strcat(msgSend, userName);
		strcat(msgSend, ":");
		strcat(msgSend, msg);
		return 0;
	} else if (strncmp(msg, "/getlist", 8) == 0) {
		//Former le message pour envoyer
		strcpy(msgSend, "");
		strcpy(msgSend, GET_LIST_CODE);
		strcat(msgSend, userName);
		return 0;
	} else if (strncmp(msg, "/list", 5) == 0) {
		printUser();
		return 1;
	} else if (strncmp(msg, "/update", 7) == 0) {
		//Former le message pour envoyer
		strcpy(msgSend, "");
		strcpy(msgSend, KEEP_ALIVE_CODE);
		strcat(msgSend, userName);
		return 0;
	} else if (strncmp(msg, "/quit", 5) == 0) {
		//Former le message pour envoyer
		strcpy(msgSend, "");
		strcpy(msgSend, QUIT_CODE);
		strcat(msgSend, userName);
		return 2;
	} else {
		//le cas normal, chat avec tout le monde
		strcpy(msgSend, "");
		strcpy(msgSend, CHAT_TO_GROUP_CODE);
		strcat(msgSend, userName);
		strcat(msgSend, ":");
		strcat(msgSend, msg);
		return 0;
	}
}

//Pour recevoir les messages
void msgRcvProcess(char *msg) {
	char tmp[MSGBUFSIZE] = "";
	char tmp2[MSGBUFSIZE] = "";
	char tmp3[MSGBUFSIZE] = "";
	char sender[MSGBUFSIZE] = "";
	char rcveur[MSGBUFSIZE] = "";

	if (strncmp(msg, CHAT_TO_GROUP_CODE, 3) == 0) {
		strnCut(tmp, msg, 3);
		printf("Group:%s\n", tmp);
	} else if (strncmp(msg, CHAT_TO_ONE_CODE, 3) == 0) {
		strnCut(tmp, msg, 3);
		strcCut(sender, tmp, ':');
		strcCut2(tmp2, tmp, ':');
		strcCut(rcveur, tmp2, ':');
		strcCut2(tmp3, tmp2, ':');
		if (strcmp(userName, sender) == 0 || strcmp(userName, rcveur) == 0) {
			printf("%s:", sender);
			printf("to %s:", rcveur);
			printf("%s\n", tmp3);
		}
	} else if (strncmp(msg, GET_LIST_CODE, 3) == 0) {
		//quand il y a un nouveau, send alive		
		strcpy(msgSend, "");
		strcpy(msgSend, KEEP_ALIVE_CODE);
		strcat(msgSend, userName);
		sendMsg();
	} else if (strncmp(msg, KEEP_ALIVE_CODE, 3) == 0) {
		strnCut(tmp, msg, 3);
		addUser(tmp);
	} else if (strncmp(msg, QUIT_CODE, 3) == 0) {
		strnCut(tmp, msg, 3);
		strcpy(sender, tmp);
		delUser(sender);
	}
}

//Cut string depuis charatere n
void strnCut(char *strDst, char *strSrc, int n) {
	for (int i = n; i < strlen(strSrc); i++) {
		strDst[i - n] = strSrc[i];
	}
	strDst[strlen(strSrc) - n + 1] = '\0';
}
//Cut string avec une charatere
void strcCut(char *strDst, char *strSrc, char c) {
	char * pch = strchr(strSrc, c);
	//puts(strSrc);
	if (pch != NULL) {
		strncpy(strDst, strSrc, pch - strSrc);
		//puts(strDst);
	}
}
//get name of utilisateur
void strcCut2(char *strDst, char *strSrc, char c) {
	char * pch = strchr(strSrc, c);
	//puts(strSrc);
	if (pch != NULL) {
		strcpy(strDst, 1 + pch);
		//puts(strDst);
	}
}
//Ajouter un utilisateur
void addUser(char *user) {
if (strcmp(userName, user) != 0){
	if (nombre == 0) {
		strcpy(listUser[nombre], user);
		nombre++;
	} else {
		int tmp = -1;
		for (int i = 0; i < nombre; i++) {
			if ((strcmp(listUser[i], user) == 0)) {
				tmp = i;
			}
		}
		if (tmp == -1) {
			strcpy(listUser[nombre], user);
			nombre++;
		}
	}
}
}
//Delete un utilisateur
void delUser(char *user) {
	for (int i = 0; i < nombre; i++) {
		if ((strcmp(listUser[i], user) == 0)) {
			if (i == 0) {
				nombre--;
			} else {
				nombre--;
				for (int j = i; j < nombre; j++)
					strcpy(listUser[j], listUser[j + 1]);
			}
		}
	}
}
//Print list des utilisateurs
void printUser() {
	printf("List user:\n");
	for (int i = 0; i < nombre; i++) {
		printf("%d:", i+1 );
		puts(listUser[i]);
	}
}
//print guide
void printGuide(){
	printf("Chat Programme Group1\n");
	printf("Les commandes:\n");
	printf("/chatlist message POUR chatter avec le group\n");
	printf("/chatone:nom_autre:message POUR chatter avec l'autre\n");
	printf("/list POUR afficher la liste du group\n");
	printf("/getlist POUR mettre a jour la liste\n");
	printf("/update POUR annoncer au group\n");
	printf("/quit POUR quitter\n");
	printf("-----------------------------------------------------\n");
}

