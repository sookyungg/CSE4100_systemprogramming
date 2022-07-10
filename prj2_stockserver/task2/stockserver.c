/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define NTHREADS 10
#define SBUFSIZE 15
#define MAX_CLIENT 15

typedef struct _node{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
}item;
item stock_table[MAX_CLIENT];
FILE* fp1;
typedef struct {
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
}sbuf_t;
sbuf_t sbuf;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);


int num = 0; 
static int cnt;
static sem_t mutex;

void echo_thread(int connfd);
void *thread(void *vargp);
static void init_echo_cnt(void);
int option_check(char* buf2, int n);


int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    
    FILE* fp = fopen("stock.txt", "r");
    if (!fp) {
        printf("file open error\n");
        exit(0);
    }
    else {
        int _id, _num, _price, res;
        for(int i=0;i<MAX_CLIENT;i++) {
            stock_table[i].ID = 0;
            stock_table[i].left_stock = 0;
            stock_table[i].price = 0;
            stock_table[i].readcnt = 0;
            Sem_init(&stock_table[i].mutex, 0, 1);
        }
        while (1) {
            res = fscanf(fp, "%d%d%d", &_id, &_num, &_price);
            if(res==EOF) break;
            num++;
            stock_table[_id].ID = _id;
            stock_table[_id].left_stock = _num;
            stock_table[_id].price = _price;
        }
    }
    fclose(fp);
    
    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    
    for(int i=0;i<NTHREADS;i++) {
        Pthread_create(&tid, NULL, thread, NULL);
    }
    while (1) {
	      clientlen = sizeof(struct sockaddr_storage); 
	      connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        sbuf_insert(&sbuf, connfd);
        
    }
}


void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    while(1) {
        int connfd = sbuf_remove(&sbuf);
        echo_thread(connfd);
        Close(connfd);
    }
}

static void init_echo_cnt(void) {
    Sem_init(&mutex, 0, 1);
    cnt=0;
}



void sbuf_init(sbuf_t *sp, int n) {
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n; 
    sp->front = sp->rear = 0; 
    Sem_init(&sp->mutex, 0, 1); 
    Sem_init(&sp->slots, 0, n); 
    Sem_init(&sp->items, 0, 0); 
}

void sbuf_deinit(sbuf_t *sp) {
  Free(sp->buf);
}



void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots); 
    P(&sp->mutex); 
    sp->buf[(++sp->rear)%(sp->n)] = item; 
    V(&sp->mutex); 
    V(&sp->items); 
}

int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items); 
    P(&sp->mutex); 
    item = sp->buf[(++sp->front)%(sp->n)]; 
    V(&sp->mutex); 
    V(&sp->slots); 
    return item;
}



int option_check(char* buf2, int n) {
    int i, k, buy_ID, buy_num, sell_ID, sell_num;
    int com_idx=0;
    char chkstock[3][MAXLINE];
    char buf4[20];
    
    //printf("%s", buf2);
    
    char *delimeter = strtok(buf2, " ");
    
    while(delimeter != NULL) {
        strcpy(chkstock[com_idx++], delimeter);
        //printf("%s\n", parse_command[com_idx-1]);
        delimeter = strtok(NULL, " ");
    }
    
    //printf("%s", parse_command[0]);
    
    if (!strcmp(chkstock[0], "buy")) {
        buy_ID = atoi(chkstock[1]);
        buy_num = atoi(chkstock[2]);
        P(&stock_table[buy_ID].mutex);
        if(stock_table[buy_ID].left_stock < buy_num ) {
            V(&stock_table[buy_ID].mutex);
            return 1;
        }
        else {
            
            buf2[0] = '\0';
            stock_table[buy_ID].left_stock -= buy_num;
            V(&stock_table[buy_ID].mutex);
            k = stock_table[buy_ID].ID;
            printf("%d", k);

            if (k == 0) {
                return 2;
            }
            else {
                sprintf(buf4, "buy %d %d \n", stock_table[buy_ID].ID, buy_num);

                strcat(buf2, buf4);
                //buf4[0] = '\0';

                strcpy(buf4, "[buy] success\n");
                strcat(buf2, buf4);
                return 2;
            }
        }
    }
    else if (!strcmp(chkstock[0], "sell")) {
        sell_ID = atoi(chkstock[1]);
        sell_num = atoi(chkstock[2]);
        P(&stock_table[sell_ID].mutex);
        stock_table[sell_ID].left_stock += sell_num;
        V(&stock_table[sell_ID].mutex);
        buf2[0] = '\0';
        k = stock_table[sell_ID].ID;

        if (k > 0) {
            sprintf(buf4, "sell %d %d \n", stock_table[sell_ID].ID, sell_num);

            strcat(buf2, buf4);

            strcpy(buf4, "[sell] success\n");

            strcat(buf2, buf4);
            return 2;
        }
        else {
            return 2;
        }
        
    }
    else if (!strcmp(chkstock[0], "show\n")) {
        fp1 = fopen("stock.txt", "w");
        buf2[0] = '\0';
        strcpy(buf4, "show\n");
        strcat(buf2, buf4);
        //buf2[0] = '\0';
        for(i=1;i<=num;i++) {
        sprintf(buf4, "%d %d %d\n", stock_table[i].ID, stock_table[i].left_stock, stock_table[i].price);
        strcat(buf2, buf4);
        buf4[0] = '\0';
        fprintf(fp1, "%d %d %d\n", stock_table[i].ID, stock_table[i].left_stock, stock_table[i].price);

        }
        fclose(fp1);
        return 2;
    }
    return 0;
}

void echo_thread(int connfd) {
    int n, m;
    char buf[MAXLINE];
    char buf2[MAXLINE];
    char buf3[MAXLINE];
    rio_t rio;
    static pthread_once_t once = PTHREAD_ONCE_INIT;

    Pthread_once(&once, init_echo_cnt);
    Rio_readinitb(&rio, connfd);

    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        P(&mutex);
        
        buf2[0] = '\0';
        cnt += n;
        printf("server received  %d  bytes\n", n);
        strcpy(buf2, buf);

        m = (option_check(buf2, n));///flagcheck
        //printf("%d", m);
        if (m == 1) {
            strcpy(buf3, "Not enough left stock\n");
            Rio_writen(connfd, buf3, MAXLINE);
            buf3[0] = '\0';

        }
        else if (m == 2) {
            Rio_writen(connfd, buf2, MAXLINE);
            buf3[0] = '\0';

        }


        V(&mutex);
    }
}



