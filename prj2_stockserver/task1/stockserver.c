/*
 * echoserveri.c - An iterative echo server
 */
 /* $begin echoserverimain */
#include "csapp.h"
#define MAX_CLIENT 15
#include <sys/select.h>
typedef struct _node {
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
}item;

item stock_table[MAX_CLIENT];

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool;


FILE* fp1;
void init_pool(int listenfd, pool* p);
void add_client(int connfd, pool* p);
void check_client(pool* p);
int optioncheck(char* buf, int n);
int stock_num = 0;
int cnt = 0;




void add_client(int connfd, pool* p) {
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++) {

        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd) p->maxfd = connfd;
            if (i > p->maxi) p->maxi = i;

            break;
        }
    }

    if (i == FD_SETSIZE) {
        app_error("add clinet error : Too many clients\n");
    }
}

void init_pool(int listenfd, pool* p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        p->clientfd[i] = -1;
    }

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}



void check_client(pool* p) {
    int connfd, n, k;
    char buf[MAXLINE];
    char buf2[MAXLINE];
    char buf3[MAXLINE];
    rio_t rio;

    for (int i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                
                
                cnt += n;
                strcpy(buf2, buf);
                printf("Server received %d bytes\n", n);
                
                k = optioncheck(buf2, n);


                if (k== 1) {
                    strcpy(buf3, "ERROR\n");
                    Rio_writen(connfd, buf3, MAXLINE);
                    buf3[0] = '\0';
                    break;

                }
                else if (k == 2) {
                    strcpy(buf3, "Not enough left stock\n");
                    Rio_writen(connfd, buf3, MAXLINE);
                    buf3[0] = '\0';
                    break;
                }
                else if (k == 3) {//write on client
                    Rio_writen(connfd, buf2, MAXLINE);
                    buf3[0] = '\0';
                    break;
                }

              
            }
            else {
                
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
}

int optioncheck(char* buf2, int n) {
    int i, buy_ID, buy_num, sell_ID, sell_num;
    int idx = 0;
    char chkstock[3][MAXLINE];
    char buf4[20];
    int k;
    

    char* delimeter = strtok(buf2, " ");

    while (delimeter != NULL) {
        strcpy(chkstock[idx++], delimeter);
        
        delimeter = strtok(NULL, " ");
    }

    

    if (!strcmp(chkstock[0], "buy")) {
        buy_ID = atoi(chkstock[1]);
        buy_num = atoi(chkstock[2]);
        if (stock_table[buy_ID].readcnt == 1) return 1;
        else {
            stock_table[buy_ID].readcnt = 1;
            if (stock_table[buy_ID].left_stock < buy_num) {
                stock_table[buy_ID].readcnt = 0;
                return 2;
            }
            else {
                k = stock_table[buy_ID].ID;
                if (k == 0) {
                    
                    return 3;

                }
                else {
                    printf("%d", k);
                    buf2[0] = '\0';
                    stock_table[buy_ID].left_stock -= buy_num;
                    stock_table[buy_ID].readcnt = 0;
                    sprintf(buf4, "buy %d %d \n", stock_table[buy_ID].ID, buy_num);

                    strcat(buf2, buf4);
                    //buf4[0] = '\0';

                    strcpy(buf4, "[buy] success\n");
                    strcat(buf2, buf4);
                    return 3;
                }
            }
        }
    }
    else if (!strcmp(chkstock[0], "sell")) {
        sell_ID = atoi(chkstock[1]);
        sell_num = atoi(chkstock[2]);
        stock_table[sell_ID].left_stock += sell_num;
        buf2[0] = '\0';
        k = stock_table[sell_ID].ID;
       
        if (k>0) {
            sprintf(buf4, "sell %d %d \n", stock_table[sell_ID].ID, sell_num);

            strcat(buf2, buf4);

            strcpy(buf4, "[sell] success\n");

            strcat(buf2, buf4);
            return 3;
        }
        else {
            return 3;
        }
    }
    else if (!strcmp(chkstock[0], "show\n")) {
        fp1 = fopen("stock.txt", "w");
        buf2[0] = '\0';
        strcpy(buf4, "show\n");

        strcat(buf2, buf4);
        for (i = 1; i <= stock_num; i++) {
            sprintf(buf4, "%d %d %d\n", stock_table[i].ID, stock_table[i].left_stock, stock_table[i].price);
            strcat(buf2, buf4);
            buf4[0] = '\0';
            
            fprintf(fp1, "%d %d %d\n", stock_table[i].ID, stock_table[i].left_stock, stock_table[i].price);

        }
        fclose(fp1);
        return 3;
    }
    else if (!strcmp(chkstock[0], "exit\n")) {
        //char ans[MAXLINE];
        buf2[0] = '\0';
        strcpy(buf4, "exit\n");
        strcat(buf2, buf4);
        //Rio_writen(connfd, ans, MAXLINE);
        return 3;
    }
    //char ans[MAXLINE];
    //strcpy(buf2, "exit\n");
    //Rio_writen(connfd, ans, MAXLINE);
    return 0;
}

int main(int argc, char** argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;
    //char ans[MAXLINE];
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
        for (int i = 0; i < MAX_CLIENT; i++) {
            stock_table[i].ID = 0;
            stock_table[i].left_stock = 0;
            stock_table[i].price = 0;
            stock_table[i].readcnt = 0;
        }
        while (1) {
            res = fscanf(fp, "%d%d%d", &_id, &_num, &_price);
            if (res == EOF) break;
            stock_num++;
            
            stock_table[_id].ID = _id;
            stock_table[_id].left_stock = _num;
            stock_table[_id].price = _price;
        }
    }
    fclose(fp);
    //printf("%d %d %d", stock_table[3].price, stock_table[2].left_stock, stock_table[1].ID );

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
            Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(connfd, &pool);
        }

        check_client(&pool);
    }
    //strcpy(ans, "exit\n");
    //Rio_writen(connfd, ans, MAXLINE);
    //printf("exit");
}
/* $end echoserverimain */
